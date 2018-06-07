// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "epoll_poller.h"

#include "../../logger.h"
#include "../channel.h"

#include <boost/static_assert.hpp>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

/*
 * struct epool_event
 *
typedef union epoll_data {
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
} epoll_data_t;

struct epoll_event {
	uint32_t events;   // Epoll events 
	epoll_data_t data; // User data variable 
};
*/

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
	const int kNew = -1;
	const int kAdded = 1;
	const int kDeleted = 2;
}

epoll_poller::epoll_poller(event_loop* loop)
	: muduo_poller(loop),
	  _epollfd(::epoll_create1(EPOLL_CLOEXEC)),
	  _events(kinit_event_list_size)
{
	if (_epollfd < 0)
	{
		LOG_SYSFATAL << "EPollPoller::EPollPoller";
	}
}

epoll_poller::~epoll_poller()
{
	::close(_epollfd);
}

time_stamp epoll_poller::poll(int timeout_ms, channel_list* active_channels)
{
	LOG_TRACE << "fd total count " << _channels.size();
	int num_events = ::epoll_wait(_epollfd,
		&*_events.begin(),
		static_cast<int>(_events.size()),
		timeout_ms);
	int saved_errno = errno;
	time_stamp now(time_stamp::now());
	if (num_events > 0)
	{
		LOG_TRACE << num_events << " events happened";
		fill_active_channels(num_events, active_channels);
		if (implicit_cast<size_t>(num_events) == _events.size())
		{
			_events.resize(_events.size() * 2);
		}
	}
	else if (num_events == 0)
	{
		LOG_TRACE << "nothing happened";
	}
	else
	{
		// error happens, log uncommon ones
		if (saved_errno != EINTR)
		{
			errno = saved_errno;
			LOG_SYSERR << "EPollPoller::poll()";
		}
	}
	return now;
}

void epoll_poller::fill_active_channels(int num_events,
	channel_list* active_channels) const
{
	assert(implicit_cast<size_t>(num_events) <= _events.size());
	for (int i = 0; i < num_events; ++i)
	{
		channel* chl = static_cast<channel*>(_events[i].data.ptr);
#ifndef NDEBUG
		int fd = chl->fd();
		channel_map_t::const_iterator it = _channels.find(fd);
		assert(it != _channels.end());
		assert(it->second == chl);
#endif
		chl->set_revents(_events[i].events);
		active_channels->push_back(chl);
	}
}

void epoll_poller::update_channel(channel* chl)
{
	muduo_poller::assert_in_loop_thread();
	const int index = chl->index();
	LOG_TRACE << "fd = " << chl->fd()
		<< " events = " << chl->events() << " index = " << index;
	if (index == kNew || index == kDeleted)
	{
		// a new one, add with EPOLL_CTL_ADD
		int fd = chl->fd();
		if (index == kNew)
		{
			assert(_channels.find(fd) == _channels.end());
			_channels[fd] = chl;
		}
		else // index == kDeleted
		{
			assert(_channels.find(fd) != _channels.end());
			assert(_channels[fd] == chl);
		}

		chl->set_index(kAdded);
		update(EPOLL_CTL_ADD, chl);
	}
	else
	{
		// update existing one with EPOLL_CTL_MOD/DEL
		int fd = chl->fd();
		(void)fd;
		assert(_channels.find(fd) != _channels.end());
		assert(_channels[fd] == chl);
		assert(index == kAdded);
		if (chl->is_none_event())
		{
			update(EPOLL_CTL_DEL, chl);
			chl->set_index(kDeleted);
		}
		else
		{
			update(EPOLL_CTL_MOD, chl);
		}
	}
}

void epoll_poller::remove_channel(channel* chl)
{
	muduo_poller::assert_in_loop_thread();
	int fd = chl->fd();
	LOG_TRACE << "fd = " << fd;
	assert(_channels.find(fd) != channels_.end());
	assert(_channels[fd] == chl);
	assert(chl->is_none_event());
	int index = chl->index();
	assert(index == kAdded || index == kDeleted);
	size_t n = _channels.erase(fd);
	(void)n;
	assert(n == 1);

	if (index == kAdded)
	{
		update(EPOLL_CTL_DEL, chl);
	}
	chl->set_index(kNew);
}

void epoll_poller::update(int operation, channel* chl)
{
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = chl->events();
	event.data.ptr = chl;
	int fd = chl->fd();
	LOG_TRACE << "epoll_ctl op = " << operation_to_string(operation)
		<< " fd = " << fd << " event = { " << chl->events_to_string() << " }";
	if (::epoll_ctl(_epollfd, operation, fd, &event) < 0)
	{
		if (operation == EPOLL_CTL_DEL)
		{
			LOG_SYSERR << "epoll_ctl op =" << operation_to_string(operation) << " fd =" << fd;
		}
		else
		{
			LOG_SYSFATAL << "epoll_ctl op =" << operation_to_string(operation) << " fd =" << fd;
		}
	}
}

const char* epoll_poller::operation_to_string(int op)
{
	switch (op)
	{
	case EPOLL_CTL_ADD:
		return "ADD";
	case EPOLL_CTL_DEL:
		return "DEL";
	case EPOLL_CTL_MOD:
		return "MOD";
	default:
		assert(false && "ERROR op");
		return "Unknown Operation";
	}
}
