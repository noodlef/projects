// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/poller/PollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>
#include <muduo/net/Channel.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

poll_poller::poll_poller(event_loop* loop)
	: muduo_poller(loop)
{ }

poll_poller::~poll_poller()
{ }

time_stamp poll_poller::poll(int timeout_ms, channel_list* active_channels)
{
	// XXX pollfds_ shouldn't change
	int num_events = ::poll(&*_pollfds.begin(), _pollfds.size(), timeout_ms);
	int saved_errno = errno;
	time_stamp now(time_stamp::now());
	if (num_events > 0)
	{
		LOG_TRACE << num_events << " events happened";
		fill_active_channels(num_events, active_channels);
	}
	else if (num_events == 0)
	{
		LOG_TRACE << " nothing happened";
	}
	else
	{
		if (savedErrno != EINTR)
		{
			errno = saved_errno;
			LOG_SYSERR << "PollPoller::poll()";
		}
	}
	return now;
}

void poll_poller::fill_active_channels(int num_events,
	channel_list* active_channels) const
{
	for (pollfd_list_t::const_iterator pfd = _pollfds.begin();
		pfd != _pollfds.end() && num_events > 0; ++pfd)
	{
		if (pfd->revents > 0)
		{
			--num_events;
			channel_map_t::const_iterator ch = _channels.find(pfd->fd);
			assert(ch != _channels.end());
			channel* chanl = ch->second;
			assert(chanl->fd() == pfd->fd);
			chanl->set_revents(pfd->revents);
			// pfd->revents = 0;
			active_channels->push_back(chanl);
		}
	}
}

void poll_poller::update_channel(channel* chl)
{
	muduo_poller::assert_in_loop_thread();
	LOG_TRACE << "fd = " << chl->fd() << " events = " << chl->events();
	if (chl->index() < 0)
	{
		// a new one, add to pollfds_
		assert(_channels.find(chl->fd()) == _channels.end());
		struct pollfd pfd;
		pfd.fd = chl->fd();
		pfd.events = static_cast<short>(chl->events());
		pfd.revents = 0;
		_pollfds.push_back(pfd);
		int idx = static_cast<int>(_pollfds.size()) - 1;
		chl->set_index(idx);
		_channels[pfd.fd] = chl;
	}
	else
	{
		// update existing one
		assert(_channels.find(chl->fd()) != _channels.end());
		assert(_channels[chl->fd()] == chl);
		int idx = chl->index();
		assert(0 <= idx && idx < static_cast<int>(_pollfds.size()));
		struct pollfd& pfd = _pollfds[idx];
		assert(pfd.fd == chl->fd() || pfd.fd == -chl->fd() - 1);
		pfd.fd = chl->fd();
		pfd.events = static_cast<short>(chl->events());
		pfd.revents = 0;
		if (chl->is_none_event())
		{
			// ignore this pollfd
			pfd.fd = -chl->fd() - 1;
		}
	}
}

void poll_poller::remove_channel(channel* chl)
{
	muduo_poller::assert_in_loop_thread();
	LOG_TRACE << "fd = " << chl->fd();
	assert(_channels.find(channel->fd()) != _channels.end());
	assert(_channels[chl->fd()] == chl);
	assert(chl->is_none_event());
	int idx = chl->index();
	assert(0 <= idx && idx < static_cast<int>(_pollfds.size()));
	const struct pollfd& pfd = _pollfds[idx]; (void)pfd;
	assert(pfd.fd == -chl->fd() - 1 && pfd.events == chl->events());
	size_t n = _channels.erase(channel->fd());
	assert(n == 1); (void)n;
	if (implicit_cast<size_t>(idx) == _pollfds.size() - 1)
	{
		_pollfds.pop_back();
	}
	else
	{
		int channel_at_end = _pollfds.back().fd;
		iter_swap(_pollfds.begin() + idx, _pollfds.end() - 1);
		if (channel_at_end < 0)
		{
			channel_at_end = -channel_at_end - 1;
		}
		_channels[channel_at_end]->set_index(idx);
		_pollfds.pop_back();
	}
}
