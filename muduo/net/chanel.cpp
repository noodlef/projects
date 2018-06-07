// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "../logger.h"
#include "channel.h"
#include "event_loop.h"

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int channel::knone_event = 0;
const int channel::kread_event = POLLIN | POLLPRI;
const int channel::kwrite_event = POLLOUT;

channel::channel(event_loop* loop, int fd__)
	: _loop(loop),
	  _fd(fd__),
	  _events(0),
	  _revents(0),
	  _index(-1),
	  _log_hup(true),
	  _tied(false),
	  _event_handling(false),
	  _added_to_loop(false)
{ }

channel::~channel()
{
	assert(!_event_handling);
	assert(!_added_to_loop);
	if (_loop->is_in_loop_thread())
	{
		assert(!_loop->has_channel(this));
	}
}

void channel::tie(const boost::shared_ptr<void>& obj)
{
	_tie = obj;
	_tied = true;
}

void channel::update()
{
	_added_to_loop = true;
	_loop->update_channel(this);
}

void channel::remove()
{
	assert(is_none_event());
	_added_to_loop = false;
	_loop->remove_channel(this);
}

void channel::handle_event(time_stamp receiveTime)
{
	boost::shared_ptr<void> guard;
	if (_tied)
	{
		guard = _tie.lock();
		if (guard)
		{
			handle_event_with_guard(receiveTime);
		}
	}
	else
	{
		handle_event_with_guard(receiveTime);
	}
}

void channel::handle_event_with_guard(time_stamp receiveTime)
{
	_event_handling = true;
	LOG_TRACE << revents_to_string();
	// close
	if ((_revents & POLLHUP) && !(_revents & POLLIN))
	{
		if (_log_hup)
		{
			LOG_WARN << "fd = " << _fd << " Channel::handle_event() POLLHUP";
		}
		if (_close_callback) _close_callback();
	}

	if (_revents & POLLNVAL)
	{
		LOG_WARN << "fd = " << _fd << " Channel::handle_event() POLLNVAL";
	}
	// error
	if (_revents & (POLLERR | POLLNVAL))
	{
		if (_error_callback) _error_callback();
	}
	// read
	if (_revents & (POLLIN | POLLPRI | POLLRDHUP))
	{
		if (_read_callback) _read_callback(receiveTime);
	}
	// write
	if (_revents & POLLOUT)
	{
		if (_write_callback) _write_callback();
	}
	_event_handling = false;
}

string channel::revents_to_string() const
{
	return events_to_string(_fd, _revents);
}

string channel::events_to_string() const
{
	return events_to_string(_fd, _events);
}

string channel::events_to_string(int fd, int ev)
{
	std::ostringstream oss;
	oss << fd << ": ";
	if (ev & POLLIN)
		oss << "IN ";
	if (ev & POLLPRI)
		oss << "PRI ";
	if (ev & POLLOUT)
		oss << "OUT ";
	if (ev & POLLHUP)
		oss << "HUP ";
	if (ev & POLLRDHUP)
		oss << "RDHUP ";
	if (ev & POLLERR)
		oss << "ERR ";
	if (ev & POLLNVAL)
		oss << "NVAL ";

	return oss.str().c_str();
}
