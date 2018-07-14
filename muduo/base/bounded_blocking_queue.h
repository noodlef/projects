#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
#define MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H

#include "muduo_condition.h"
#include "muduo_mutex.h"

#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#include <assert.h>

namespace muduo
{
    // 有界队列，当超过队列设定的最大长度时
    // 队列前面的元素会被后面添加的元素覆盖
	template<typename T>
	class bounded_blocking_queue : boost::noncopyable
	{
	public:
		explicit bounded_blocking_queue(int maxSize)
			: _mutex(),
			  _not_empty(_mutex),
			  _not_full(_mutex),
			  _queue(maxSize)
		{ }

		void put(const T& x)
		{
			mutex_lock_guard lock(_mutex);
			while (_queue.full())
			{
				_not_full.wait();
			}
			assert(!_queue.full());
			_queue.push_back(x);
			_not_empty.notify();
		}

		T take()
		{
			mutex_lock_guard lock(_mutex);
			while (_queue.empty())
			{
				_not_empty.wait();
			}
			assert(!_queue.empty());
			T front(_queue.front());
			_queue.pop_front();
			_not_full.notify();
			return front;
		}

		bool empty() const
		{
			mutex_lock_guard lock(_mutex);
			return _queue.empty();
		}

		bool full() const
		{
			mutex_lock_guard lock(_mutex);
			return _queue.full();
		}

		size_t size() const
		{
			mutex_lock_guard lock(_mutex);
			return _queue.size();
		}

		size_t capacity() const
		{
			mutex_lock_guard lock(_mutex);
			return _queue.capacity();
		}

	private:
		mutable muduo_mutex          _mutex;
		muduo_condition              _not_empty;
		muduo_condition              _not_full;
		boost::circular_buffer<T>    _queue;
	};

}

#endif  // MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
