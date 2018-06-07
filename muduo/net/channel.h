#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "../time_stamp.h"

namespace muduo
{
	namespace net
	{

		class event_loop;

	
		// A selectable I/O channel.
		// This class doesn't own the file descriptor.
		// The file descriptor could be a socket,
		// an eventfd, a timerfd, or a signalfd
		class channel : boost::noncopyable
		{
		public:
			typedef boost::function<void()> event_callback;
			typedef boost::function<void(time_stamp)> read_event_callback;

			channel(event_loop* loop, int fd);

			~channel();

			void handle_event(time_stamp receiveTime);

			void remove();

			// set callback
			void set_read_callback(const read_event_callback& cb)
			{
				_read_callback = cb;
			}
			void set_write_callback(const event_callback& cb)
			{
				_write_callback = cb;
			}
			void set_close_callback(const event_callback& cb)
			{
				_close_callback = cb;
			}
			void set_error_callback(const event_callback& cb)
			{
				_error_callback = cb;
			}

			// set events
			void enable_reading() { _events |= kread_event; update(); }
			void disable_reading() { _events &= ~kread_event; update(); }
			void enable_writing() { _events |= kwrite_event; update(); }
			void disable_writing() { _events &= ~kwrite_event; update(); }
			void disable_all() { _events = knone_event; update(); }

			// Tie this channel to the owner object managed by shared_ptr,
			// prevent the owner object being destroyed in handleEvent.
			void tie(const boost::shared_ptr<void>& obj);

			void set_revents(int revt) { _revents = revt; } // used by pollers	

			void do_not_log_hup() { _log_hup = false; }

			event_loop* owner_loop() { return _loop; }

			int fd() const { return _fd; }
			int events() const { return _events; }									
			bool is_none_event() const { return _events == knone_event; }

			
			bool is_writing() const { return _events & kwrite_event; }
			bool is_reading() const { return _events & kread_event; }

			// for Poller
			int index() { return _index; }
			void set_index(int idx) { _index = idx; }

			// for debug
			string revents_to_string() const;
			string events_to_string() const;

		private:
			static string events_to_string(int fd, int ev);

			void update();

			void handle_event_with_guard(time_stamp receiveTime);

			static const int knone_event;
			static const int kread_event;
			static const int kwrite_event;

			event_loop*		_loop;
			const int		_fd;
			int             _events;
			int             _revents; // it's the received event types of epoll or poll

			int             _index;   // used by Poller.
			bool            _log_hup; // ???

			boost::weak_ptr<void> _tie;
			bool	_tied;
			bool	_event_handling;
			bool	_added_to_loop;     

			read_event_callback      _read_callback;
			event_callback           _write_callback;
			event_callback           _close_callback;
			event_callback           _error_callback;
		};

	}
}
#endif  // MUDUO_NET_CHANNEL_H
