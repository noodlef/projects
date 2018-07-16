#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include<boost/noncopyable.hpp>

namespace muduo
{
    // muduo_mutex 
	class muduo_mutex : boost::noncopyable
	{
	public:
		muduo_mutex();

		~muduo_mutex();

		// must be called when locked, i.e. for assertion
		bool is_locked_by_this_thread() const;

		void assert_locked() const;

		void lock();

		void unlock();

		pthread_mutex_t* get_pthread_mutex();

	private:
		friend class muduo_condition;

		void unassign_holder();

		void assign_holder();

		pthread_mutex_t     _mutex;
		pid_t               _holder;/* 当前获得互斥量的线程 tid */
	};

    // helper class used by muduo_mutex
	class mutex_lock_guard : boost::noncopyable
	{
	public:
		explicit mutex_lock_guard(muduo_mutex& mutex)
			: _mutex(mutex)
		{
			_mutex.lock();
		}

		~mutex_lock_guard()
		{
			_mutex.unlock();
		}

	private:

		muduo_mutex& _mutex;
	};

}

#endif  // MUDUO_BASE_MUTEX_H
