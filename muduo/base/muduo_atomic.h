#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace muduo
{

	namespace detail
	{
		template<typename T>
		class atomic_integer : boost::noncopyable
		{
		public:
			atomic_integer()
				: _value(0)
			{ }

			T get()
			{
				// in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
				return __sync_val_compare_and_swap(&_value, 0, 0);
			}

			T get_and_add(T x)
			{
				// in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
				return __sync_fetch_and_add(&_value, x);
			}

			T add_and_get(T x)
			{
				return get_and_add(x) + x;
			}

			T increment_and_get()
			{
				return add_and_get(1);
			}

			T decrement_and_get()
			{
				return add_and_get(-1);
			}

			void add(T x)
			{
				get_and_add(x);
			}

			void increment()
			{
				increment_and_get();
			}

			void decrement()
			{
				decrementAndGet();
			}

			T get_and_set(T newValue)
			{
				// in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
				return __sync_lock_test_and_set(&_value, newValue);
			}

		private:
			volatile T _value;
		};
	}

	typedef detail::atomic_integer<int32_t> atomic_int32;
	typedef detail::atomic_integer<int64_t> atomic_int64;
}

#endif  // MUDUO_BASE_ATOMIC_H
