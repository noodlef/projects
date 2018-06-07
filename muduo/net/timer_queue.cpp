// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "timer_queue.h"

#include "../logger.h"
#include "event_loop.h"
#include "timer.h"
#include "timer_id.h"

#include <boost/bind.hpp>

#include <sys/timerfd.h>
#include <unistd.h>

namespace muduo
{
	namespace net
	{
		namespace detail
		{

			int create_timerfd()
			{
				int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
					TFD_NONBLOCK | TFD_CLOEXEC);
				if (timerfd < 0)
				{
					LOG_SYSFATAL << "Failed in timerfd_create";
				}
				return timerfd;
			}

			struct timespec how_much_time_from_now(time_stamp when)
			{
				int64_t microseconds = when.micro_seconds_sinceEpoch()
					- time_stamp::now().micro_seconds_sinceEpoch();
				if (microseconds < 100)
				{
					microseconds = 100;
				}
				struct timespec ts;
				ts.tv_sec = static_cast<time_t>(
					microseconds / time_stamp::kmicro_seconds_perSecond);
				ts.tv_nsec = static_cast<long>(
					(microseconds % time_stamp::kmicro_seconds_perSecond) * 1000);
				return ts;
			}

			void read_timerfd(int timerfd, time_stamp now)
			{
				uint64_t howmany;
				ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
				LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.to_string();
				if (n != sizeof howmany)
				{
					LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
				}
			}

			void reset_timerfd(int timerfd, time_stamp expiration)
			{
				// wake up loop by timerfd_settime()
				struct itimerspec new_value;
				struct itimerspec old_value;
				bzero(&new_value, sizeof new_value);
				bzero(&old_value, sizeof old_value);
				new_value.it_value = how_much_time_from_now(expiration);
				int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
				if (ret)
				{
					LOG_SYSERR << "timerfd_settime()";
				}
			}

		}
	}
}

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

timer_queue::timer_queue(event_loop* loop)
	: _loop(loop),
	  _timerfd(create_timerfd()),
	  _timerfd_channel(_loop, _timerfd),
	  _timers(),
	  _calling_expired_timers(false)
{
	_timerfd_channel.set_read_callback(
		boost::bind(&timer_queue::handle_read, this));
	// we are always reading the timerfd, we disarm it with timerfd_settime.
	_timerfd_channel.enable_reading();
}

timer_queue::~timer_queue()
{
	_timerfd_channel.disable_all();
	_timerfd_channel.remove();
	::close(_timerfd);
	// do not remove channel, since we're in EventLoop::dtor();
	for (timer_list::iterator it = _timers.begin();
		it != _timers.end(); ++it)
	{
		delete it->second;
	}
}

timer_id timer_queue::add_timer(const timer_callback& cb,
	time_stamp when,
	double interval)
{
	muduo_timer* timer = new muduo_timer(cb, when, interval);
	_loop->run_in_loop(
		boost::bind(&timer_queue::add_timer_in_loop, this, timer));
	return timer_id(timer, timer->sequence());
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
TimerId TimerQueue::addTimer(TimerCallback&& cb,
	Timestamp when,
	double interval)
{
	Timer* timer = new Timer(std::move(cb), when, interval);
	loop_->runInLoop(
		boost::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}
#endif

void timer_queue::cancel(timer_id timerId)
{
	_loop->run_in_loop(
		boost::bind(&timer_queue::cancel_in_loop, this, timerId));
}

void timer_queue::add_timer_in_loop(muduo_timer* timer)
{
	_loop->assert_in_loop_thread();
	bool earliest_changed = insert(timer);

	if (earliest_changed)
	{
		reset_timerfd(_timerfd, timer->expiration());
	}
}

void timer_queue::cancel_in_loop(timer_id timerId)
{
	_loop->assert_in_loop_thread();
	assert(_timers.size() == _active_timers.size());
	active_timer timer(timerId._timer, timerId._sequence);
	active_timer_set::iterator it = _active_timers.find(timer);
	if (it != _active_timers.end())
	{
		size_t n = _timers.erase(entry(it->first->expiration(), it->first));
		assert(n == 1); (void)n;
		delete it->first; // FIXME: no delete please
		_active_timers.erase(it);
	}
	else if (_calling_expired_timers)
	{
		_canceling_timers.insert(timer);
	}
	assert(_timers.size() == _active_timers.size());
}

void timer_queue::handle_read()
{
	_loop->assert_in_loop_thread();
	time_stamp now(time_stamp::now());
	read_timerfd(_timerfd, now);

	std::vector<entry> expired = get_expired(now);

	_calling_expired_timers = true;
	_canceling_timers.clear();
	// safe to callback outside critical section
	for (std::vector<entry>::iterator it = expired.begin();
		it != expired.end(); ++it)
	{
		it->second->run();
	}
	_calling_expired_timers = false;

	reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
	assert(timers_.size() == activeTimers_.size());
	std::vector<Entry> expired;
	Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	TimerList::iterator end = timers_.lower_bound(sentry);
	assert(end == timers_.end() || now < end->first);
	std::copy(timers_.begin(), end, back_inserter(expired));
	timers_.erase(timers_.begin(), end);

	for (std::vector<Entry>::iterator it = expired.begin();
		it != expired.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		size_t n = activeTimers_.erase(timer);
		assert(n == 1); (void)n;
	}

	assert(timers_.size() == activeTimers_.size());
	return expired;
}

void timer_queue::reset(const std::vector<Entry>& expired, Timestamp now)
{
	Timestamp nextExpire;

	for (std::vector<Entry>::const_iterator it = expired.begin();
		it != expired.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		if (it->second->repeat()
			&& cancelingTimers_.find(timer) == cancelingTimers_.end())
		{
			it->second->restart(now);
			insert(it->second);
		}
		else
		{
			// FIXME move to a free list
			delete it->second; // FIXME: no delete please
		}
	}

	if (!timers_.empty())
	{
		nextExpire = timers_.begin()->second->expiration();
	}

	if (nextExpire.valid())
	{
		resetTimerfd(timerfd_, nextExpire);
	}
}

bool timer_queue::insert(muduo_timer* timer)
{
	_loop->assert_in_loop_thread();
	assert(_timers.size() == _active_timers.size());
	bool earliest_changed = false;
	time_stamp when = timer->expiration();
	timer_list::iterator it = _timers.begin();
	if (it == _timers.end() || when < it->first)
	{
		earliest_changed = true;
	}
	{
		std::pair<timer_list::iterator, bool> result
			= _timers.insert(entry(when, timer));
		assert(result.second); (void)result;
	}
	{
		std::pair<active_timer_set::iterator, bool> result
			= _active_timers.insert(active_timer(timer, timer->sequence()));
		assert(result.second); (void)result;
	}

	assert(_timers.size() == _active_timers.size());
	return earliest_changed;
}

