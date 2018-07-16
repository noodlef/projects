#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h

// 检测返回值是否为0
#define MCHECK(ret) ({                   \
        typeof(ret) errnum = (ret);      \
        assert(errnum == 0);             \
        (void) errnum;                   \
})

namespace muduo
{

    /**
     * 线程局部对象
     */
	template<typename T>
	class thread_local : boost::noncopyable
	{
	public:
		thread_local()
		{
			MCHECK(pthread_key_create(&_pkey, &thread_local::destructor));
		}

		~thread_local()
		{
			MCHECK(pthread_key_delete(_pkey));
		}

		T& value()
		{
			T* per_thread_value = static_cast<T*>(pthread_getspecific(_pkey));
			if (!per_thread_value)
			{
				T* new_obj = new T();
				MCHECK(pthread_setspecific(_pkey, new_obj));
				per_thread_value = new_obj;
			}
			return *per_thread_value;
		}
	private:

		static void destructor(void *x)
		{
			T* obj = static_cast<T*>(x);
			typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
			T_must_be_complete_type dummy; (void)dummy;
			delete obj;
		}

	private:
		pthread_key_t _pkey;
	};

}
#endif
