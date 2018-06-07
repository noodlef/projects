#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "muduo_condition.h"
#include "muduo_mutex.h"
#include "muduo_thread.h"
#include "types.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{

	class thread_pool : boost::noncopyable
	{
	public:
		typedef boost::function<void()> task_t;

		explicit thread_pool(const string& nameArg = string("ThreadPool"));
		~thread_pool();

		// Must be called before start().
		void set_max_queueSize(int maxSize) { _max_queue_size = maxSize; }

		void set_thread_init_callback(const task_t& cb){ _thread_init_callback = cb;}

		void start(int numThreads);

		void stop();

		const string& name() const{ return _name;}

		size_t queue_size() const;

		// Could block if maxQueueSize > 0
		void run(const task_t& f);

	private:
		bool is_full() const;
		void run_in_thread();
		task_t take();

		mutable muduo_mutex                    _mutex;
		muduo_condition                        _not_empty;
		muduo_condition                        _not_full;
		string                                 _name;
		task_t                                 _thread_init_callback;
		boost::ptr_vector<muduo::muduo_thread> _threads;
		std::deque<task_t>                     _queue;
		size_t                                 _max_queue_size;
		bool                                   _running;
	};

}

#endif
