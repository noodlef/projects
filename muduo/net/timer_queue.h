#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "../muduo_mutex.h"
#include "../time_stamp.h"
#include "callbacks.h"
#include "channel.h"

namespace muduo
{
	namespace net
	{

		class event_loop;
		class muduo_timer;
		class timer_id;

		
		// A best efforts timer queue.
		// No guarantee that the callback will be on time.
		class timer_queue : boost::noncopyable
		{
		public:
			explicit timer_queue(event_loop* loop);
			~timer_queue();

			// Schedules the callback to be run at given time,
			// repeats if @c interval > 0.0.
		    // Must be thread safe. Usually be called from other threads.
			timer_id add_timer(const timer_callback& cb,
				               time_stamp when,
				               double interval);

			void cancel(timer_id timerId);

		private:

			// FIXME: use unique_ptr<Timer> instead of raw pointers.
			// This requires heterogeneous comparison lookup (N3465) from C++14
			// so that we can find an T* in a set<unique_ptr<T>>.
			typedef std::pair<time_stamp, muduo_timer*> entry;
			typedef std::set<entry> timer_list;
			typedef std::pair<muduo_timer*, int64_t> active_timer;
			typedef std::set<active_timer> active_timer_set;

			void add_timer_in_loop(muduo_timer* timer);

			void cancel_in_loop(timer_id timerId);

			// called when timerfd alarms
			void handle_read();

			// move out all expired timers
			std::vector<entry> get_expired(time_stamp now);

			void reset(const std::vector<entry>& expired, time_stamp now);

			bool insert(muduo_timer* timer);

			event_loop*                      _loop;
			const int                        _timerfd;
			channel                          _timerfd_channel;

			// Timer list sorted by expiration
			timer_list                       _timers;

			// for cancel()
			active_timer_set                 _active_timers;
			bool                             _calling_expired_timers; /* atomic */
			active_timer_set                 _canceling_timers;
		};

	}
}
#endif  // MUDUO_NET_TIMERQUEUE_H
