#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include <stdint.h>

namespace muduo
{
	namespace current_thread
	{
		// internal 线程局部变量
		extern __thread int             t_cached_tid;
		extern __thread char            t_tid_string[32];
		extern __thread int             t_tid_string_length;
		extern __thread const char*     t_thread_name;
		void cache_tid();
        
        // 返回线程 id
		inline int tid()
		{
            // _builtin_expect(expr, value) 告知编译器expr == value的概率很大
			if (__builtin_expect(t_cached_tid == 0, 0))
			{
				cache_tid();
			}
			return t_cached_tid;
		}

		inline const char* tid_string() 
		{
			return t_tid_string;
		}

		inline int tid_string_length() 
		{
			return t_tid_string_length;
		}

        // 返回线程的名字
		inline const char* name()
		{
			return t_thread_name;
		}

        // 判断是否主线程
		bool is_main_thread();

        // 进入可中断睡眠状态 @usec 单位微妙
		void sleep_usec(int64_t usec);
	}
}

#endif
