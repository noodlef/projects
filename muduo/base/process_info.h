#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_BASE_PROCESSINFO_H
#define MUDUO_BASE_PROCESSINFO_H

#include "string_piece.h"
#include "types.h"
#include "time_stamp.h"
#include <vector>
#include <sys/types.h>

namespace muduo
{

	namespace process_info
	{
		pid_t pid();
		string pid_string();
		uid_t uid();
		string username();
		uid_t euid();
		time_stamp start_time();
		int clockTicks_per_second();
		int page_size();
		bool is_debug_build();  // constexpr

		string hostname();
		string procname();
		string_piece procname(const string& stat);

		// read /proc/self/status
		string proc_status();

		// read /proc/self/stat
		string proc_stat();

		// read /proc/self/task/tid/stat
		string thread_stat();

		// readlink /proc/self/exe
		string exe_path();

		int opened_files();
		int max_open_files();

		struct cpu_time
		{
			double user_seconds;
			double system_seconds;

			cpu_time() : user_seconds(0.0), system_seconds(0.0)
			{ }
		};
		cpu_time cpuTime();

		int num_threads();
		std::vector<pid_t> threads();
	}

}

#endif  // MUDUO_BASE_PROCESSINFO_H
