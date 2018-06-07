#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{

	template<typename T>
	class thread_local_singleton : boost::noncopyable
	{
	public:

		static T& instance()
		{
			if (!_t_value)
			{
				_t_value = new T();
				_deleter.set(_t_value);
			}
			return *_t_value;
		}

		static T* pointer()
		{
			return _t_value;
		}

	private:
		thread_local_singleton();
		~thread_local_singleton();

		static void destructor(void* obj)
		{
			assert(obj == _t_value);
			typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
			T_must_be_complete_type dummy; (void)dummy;
			delete _t_value;
			_t_value = 0;
		}

		class deleter
		{
		public:
			deleter()
			{
				pthread_key_create(&_pkey, &thread_local_singleton::destructor);
			}

			~deleter()
			{
				pthread_key_delete(_pkey);
			}

			void set(T* newObj)
			{
				assert(pthread_getspecific(_pkey) == NULL);
				pthread_setspecific(_pkey, newObj);
			}

			pthread_key_t   _pkey;
		};

		static __thread T*   _t_value;
		static deleter       _deleter;
	};

	template<typename T>
	__thread T* thread_local_singleton<T>::_t_value = 0;

	template<typename T>
	typename thread_local_singleton<T>::deleter thread_local_singleton<T>::_deleter;

}
#endif
