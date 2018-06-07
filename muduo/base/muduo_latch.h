#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include"muduo_mutex.h"
#include "muduo_condition.h"
#include <boost/noncopyable.hpp>

namespace muduo
{

	class count_down_latch : boost::noncopyable
	{
	public:

		explicit count_down_latch(int count);

		void wait();

		void count_down();

		int get_count() const;

	private:
		mutable muduo_mutex  _mutex;
		muduo_condition      _condition;
		int                  _count;
	};

}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
