// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "event_loop.h"

#include "../logger.h"
#include "../muduo_mutex.h"
#include "channel.h"
#include "poller.h"
#include "sockets_ops.h"
#include "../time_stamp.h"

#include <boost/bind.hpp>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
	__thread event_loop* t_loop_in_this_thread = 0;

	const int kpoll_time_ms = 10000;

	int create_eventfd()
	{
		int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if (evtfd < 0)
		{
			LOG_SYSERR << "Failed in eventfd";
			abort();
		}
		return evtfd;
	}

#pragma GCC diagnostic ignored "-Wold-style-cast"
	class ignore_sig_pipe
	{
	public:
		ignore_sig_pipe()
		{
			::signal(SIGPIPE, SIG_IGN);
			// LOG_TRACE << "Ignore SIGPIPE";
		}
	};
#pragma GCC diagnostic error "-Wold-style-cast"

	ignore_sig_pipe initObj;
}



event_loop* event_loop::get_event_loop_of_currentThread()
{
	return t_loop_in_this_thread;
}

event_loop::event_loop()
	: _looping(false),
	  _quit(false),
	  _event_handling(false),
	  _calling_pending_functors(false),
	  _iteration(0),
	  _thread_id(current_thread::tid()),
	  _poller(poller::new_default_poller(this)),
	  _timer_queue(new timer_queue(this)),
	  _wakeup_fd(create_eventfd()),
	  _wakeup_channel(new channel(this, _wakeup_fd)),
	  _current_active_channel(NULL)
{
	LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
	if (t_loopInThisThread)
	{
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
			<< " exists in this thread " << threadId_;
	}
	else
	{
		t_loopInThisThread = this;
	}
	wakeupChannel_->setReadCallback(
		boost::bind(&EventLoop::handleRead, this));
	// we are always reading the wakeupfd
	wakeupChannel_->enableReading();
}

event_loop::~event_loop()
{
	LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
		<< " destructs in thread " << CurrentThread::tid();
	wakeupChannel_->disableAll();
	wakeupChannel_->remove();
	::close(wakeupFd_);
	t_loopInThisThread = NULL;
}

void event_loop::loop()
{
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;
	quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
	LOG_TRACE << "EventLoop " << this << " start looping";

	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
		++iteration_;
		if (Logger::logLevel() <= Logger::TRACE)
		{
			printActiveChannels();
		}
		// TODO sort channel by priority
		eventHandling_ = true;
		for (ChannelList::iterator it = activeChannels_.begin();
			it != activeChannels_.end(); ++it)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void event_loop::quit()
{
	quit_ = true;
	// There is a chance that loop() just executes while(!quit_) and exits,
	// then EventLoop destructs, then we are accessing an invalid object.
	// Can be fixed using mutex_ in both places.
	if (!isInLoopThread())
	{
		wakeup();
	}
}

void event_loop::run_in_loop(const Functor& cb)
{
	if (isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(cb);
	}
}

void event_loop::queue_in_loop(const Functor& cb)
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}

size_t event_loop::queue_size() const
{
	MutexLockGuard lock(mutex_);
	return pendingFunctors_.size();
}

timer_id event_loop::run_at(const time_stamp& time, const timer_callback& cb)
{
	return timerQueue_->addTimer(cb, time, 0.0);
}

timer_id event_loop::run_after(double delay, const timer_callback& cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, cb);
}

timer_id event_loop::run_every(double interval, const timer_callback& cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue_->addTimer(cb, time, interval);
}


void event_loop::cancel(timer_id timerId)
{
	return timerQueue_->cancel(timerId);
}

void event_loop::update_channel(channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(channel);
}

void event_loop::remove_channel(channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	if (eventHandling_)
	{
		assert(currentActiveChannel_ == channel ||
			std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
	}
	poller_->removeChannel(channel);
}

bool event_loop::has_channel(channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	return poller_->hasChannel(channel);
}

void event_loop::abort_not_in_loop_thread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< " was created in threadId_ = " << threadId_
		<< ", current thread id = " << CurrentThread::tid();
}

void event_loop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

void event_loop::handle_read()
{
	uint64_t one = 1;
	ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
	}
}

void event_loop::do_pending_functors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i)
	{
		functors[i]();
	}
	callingPendingFunctors_ = false;
}

void event_loop::print_active_channels() const
{
	for (ChannelList::const_iterator it = activeChannels_.begin();
		it != activeChannels_.end(); ++it)
	{
		const Channel* ch = *it;
		LOG_TRACE << "{" << ch->reventsToString() << "} ";
	}
}

