#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include"muduo_latch.h"

namespace muduo
{

	class muduo_thread : boost::noncopyable
	{
	public:
		typedef boost::function<void()> thread_func;

		explicit muduo_thread(const thread_func&, const string& name = string());
		~muduo_thread();

		void start();
		int join(); 

		bool started() const { return _started; }
		pthread_t pthread_Id() const { return _pthread_Id; }
		pid_t tid() const { return _tid; }
		const string& name() const { return _name; }

		static int num() { return _num.get(); }

	private:
		void _set_default_name();
		

		bool                  _started;
		bool                  _joined;
		pthread_t             _pthread_Id;
		pid_t                 _tid;
		thread_func           _func;
		string                _name;
		count_down_latch      _latch;

		static atomic_Int32   _num;
	};

}
#endif