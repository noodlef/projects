#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H

#include <boost/noncopyable.hpp>

#include "../muduo_atomic.h"
#include "../time_stamp.h"
#include "callbacks.h"

namespace muduo
{
	namespace net
	{
		
		// Internal class for timer event.
		class muduo_timer : boost::noncopyable
		{
		public:
			muduo_timer(const timer_callback& cb, time_stamp when, double interval)
				: _callback(cb),
				  _expiration(when),
				  _interval(interval),
				  _repeat(interval > 0.0),
				  _sequence(_s_num_created.increment_and_get())
			{ }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
			Timer(TimerCallback&& cb, Timestamp when, double interval)
				: callback_(std::move(cb)),
				expiration_(when),
				interval_(interval),
				repeat_(interval > 0.0),
				sequence_(s_numCreated_.incrementAndGet())
			{ }
#endif

			void run() const
			{
				_callback();
			}

			time_stamp expiration() const { return _expiration; }
			bool repeat() const { return _repeat; }
			int64_t sequence() const { return _sequence; }

			void restart(time_stamp now)
			{
				if (_repeat)
				{
					_expiration = add_time(now, _interval);
				}
				else
				{
					_expiration = time_stamp::invalid();
				}
			}

			static int64_t num_created() { return _s_num_created.get(); }

		private:
			const timer_callback              _callback;
			time_stamp                        _expiration;
			const double                      _interval;
			const bool                        _repeat;
			const int64_t                     _sequence;

			static atomic_int64               _s_num_created;
		};
	}
}
#endif  // MUDUO_NET_TIMER_H
