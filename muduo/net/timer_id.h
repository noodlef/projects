#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TIMERID_H
#define MUDUO_NET_TIMERID_H

#include "../copyable.h"

namespace muduo
{
	namespace net
	{

		class muduo_timer;

		// An opaque identifier, for canceling Timer.
		class timer_id : public muduo::copyable
		{
		public:
			timer_id()
				: _timer(NULL),
				  _sequence(0)
			{ }

			timer_id(muduo_timer* timer, int64_t seq)
				: _timer(timer),
				  _sequence(seq)
			{ }


			friend class timer_queue;

		private:
			muduo_timer*    _timer;
			int64_t         _sequence;
		};

	}
}

#endif  // MUDUO_NET_TIMERID_H
