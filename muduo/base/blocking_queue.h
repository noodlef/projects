#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include "muduo_condition.h"
#include "muduo_mutex.h"

#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

namespace muduo
{

	template<typename T>
	class blocking_queue : boost::noncopyable
	{
	public:
		blocking_queue()
			: _mutex(),
			  _not_empty(_mutex),
			  _queue()
		{ }

		void put(const T& x)
		{
			mutex_lock_guard lock(_mutex);
			_queue.push_back(x);
			_not_empty.notify(); 
			// wait morphing saves us
		    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
		}

		T take()
		{
			mutex_lock_guard lock(_mutex);
			// always use a while-loop, due to spurious wakeup
			while (_queue.empty())
			{
				_not_empty.wait();
			}
			assert(!_queue.empty());
			T front(_queue.front());
			_queue.pop_front();
			return front;
		}

		size_t size() const
		{
			mutex_lock_guard lock(_mutex);
			return _queue.size();
		}

	private:
		mutable muduo_mutex      _mutex;
		muduo_condition          _not_empty;
		std::deque<T>            _queue;
	};

}

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H
