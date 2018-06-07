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
		// internal
		extern __thread int             t_cached_tid;
		extern __thread char            t_tid_string[32];
		extern __thread int             t_tid_string_length;
		extern __thread const char*     t_thread_name;
		void cache_tid();

		inline int tid()
		{
			if (__builtin_expect(t_cached_tid == 0, 0))
			{
				cache_tid();
			}
			return t_cached_tid;
		}

		inline const char* tid_string() // for logging
		{
			return t_tid_string;
		}

		inline int tid_string_length() // for logging
		{
			return t_tid_string_length;
		}

		inline const char* name()
		{
			return t_thread_name;
		}

		bool is_main_thread();

		void sleep_usec(int64_t usec);
	}
}

#endif
