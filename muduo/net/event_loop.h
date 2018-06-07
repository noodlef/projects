#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "../muduo_mutex.h"
#include "../current_thread.h"
#include "../time_stamp.h"
#include "callbacks.h"
#include "timer_id.h"

namespace muduo
{
	namespace net
	{

		class channel;
		class muduo_poller;
		class timer_queue;

		// Reactor, at most one per thread.
		// This is an interface class, so don't expose too much details.
		class event_loop : boost::noncopyable
		{
		public:
			typedef boost::function<void()> functor;

			event_loop();
			~event_loop();  // force out-line dtor, for scoped_ptr members.

		    // Loops forever.
		    // Must be called in the same thread as creation of the object.
			void loop();

			// Quits loop.
			// This is not 100% thread safe, if you call through a raw pointer,
			// better to call through shared_ptr<EventLoop> for 100% safety.
			void quit();

	
			// Time when poll returns, usually means data arrival.
			time_stamp poll_return_time() const { return _poll_return_time; }

			int64_t iteration() const { return _iteration; }

			// Runs callback immediately in the loop thread.
			// It wakes up the loop, and run the cb.
			// If in the same loop thread, cb is run within the function.
			// Safe to call from other threads.
			void run_in_loop(const functor& cb);

			// Queues callback in the loop thread.
			// Runs after finish pooling.
			// Safe to call from other threads.
			void queue_in_loop(const functor& cb);

			size_t queue_size() const;

			// Runs callback at 'time'.
			// Safe to call from other threads.
			timer_id run_at(const time_stamp& time, const timer_callback& cb);
			
			// Runs callback after @c delay seconds.
			// Safe to call from other threads.
			timer_id run_after(double delay, const timer_callback& cb);
			
			// Runs callback every @c interval seconds.
			// Safe to call from other threads.
			timer_id run_every(double interval, const timer_callback& cb);
			
			// Cancels the timer.
			// Safe to call from other threads.
			void cancel(timer_id timerId);

			// internal usage
			void wakeup();
			void update_channel(channel* channel);
			void remove_channel(channel* channel);
			bool has_channel(channel* channel);

			void assert_in_loop_thread()
			{
				if (!is_in_loop_thread())
				{
					abort_not_in_loop_thread();
				}
			}

			bool is_in_loop_thread() const { return _thread_id == current_thread::tid(); }

			bool event_handling() const { return _event_handling; }

			void set_context(const boost::any& context)
			{
				_context = context;
			}

			const boost::any& get_context() const
			{
				return _context;
			}

			boost::any* get_mutable_context()
			{
				return &_context;
			}

			static event_loop* get_event_loop_of_currenThread();

		private:
			void abort_not_in_loop_thread();

			void handle_read();  // waked up

			void do_pending_functors();

			void print_active_channels() const; // for DEBUG

			typedef std::vector<channel*> channel_list_t;

			
			bool  _looping; /* atomic */
			bool  _quit; /* atomic and shared between threads, okay on x86, I guess. */
			bool  _event_handling; /* atomic */
			bool  _calling_pending_functors; /* atomic */

			int64_t                                  _iteration;
			const pid_t                              _thread_id;
			time_stamp                               _poll_return_time;
			boost::scoped_ptr<poller>                _poller;
			boost::scoped_ptr<timer_queue>           _timer_queue;
			
			int                                      _wakeup_fd;
			boost::scoped_ptr<channel>               _wakeup_channel;
			boost::any                               _context;

			// scratch variables
			channel_list_t                           _active_channels;
			channel*                                 _current_active_channel;

			mutable muduo_mutex                      _mutex;
			std::vector<functor>                     _pending_functors; // @GuardedBy _mutex
		};

	}
}
#endif  // MUDUO_NET_EVENTLOOP_H
