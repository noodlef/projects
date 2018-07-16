#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "muduo_mutex.h"
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

	class muduo_condition : boost::noncopyable
	{
	public:
		explicit muduo_condition(muduo_mutex& mutex);

		~muduo_condition();

		void wait();

		// returns true if time out, false otherwise.
		bool wait_for_seconds(double seconds);

        /* 唤醒一个等待该条件的线程 */
		void notify();

        /* 唤醒所有等待该条件的线程 */
		void notify_all();

	private:
		muduo_mutex&    _mutex;
		pthread_cond_t  _pcond;
	};

}
#endif  // MUDUO_BASE_CONDITION_H
