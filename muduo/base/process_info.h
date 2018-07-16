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
        // pid 
		pid_t pid();

        // pid_string
		string pid_string();

        // uid_t
		uid_t uid();

        // 返回当前登陆的用户名
		string username();

        // euid
		uid_t euid();

        // 返回系统启动时间
		time_stamp start_time();

        // 每秒钟系统时钟滴答次数
		int clockTicks_per_second();
        
        // 页大小
		int page_size();

        // 是否处于debug 模式
		bool is_debug_build();  // constexpr

        // 主机名
		string hostname();

        // 处理器名字
		string procname();
		string_piece procname(const string& stat);

		// read /proc/self/status
        // 进程状态相关的信息
		string proc_status();

		// read /proc/self/stat
        // 进程相关的信息
		string proc_stat();

		// read /proc/self/task/tid/stat
        // 线程相关的信息
		string thread_stat();

		// readlink /proc/self/exe
        // 进程执行路径
		string exe_path();
            
        // 打开的文件个数
		int opened_files();

        // 最大能打开的文件个数
		int max_open_files();

		struct cpu_time
		{
			double user_seconds;
			double system_seconds;

			cpu_time() : user_seconds(0.0), system_seconds(0.0)
			{ }
		};

        // cpu 时间
		cpu_time cpuTime();

        // 进程包含的线程个数
		int num_threads();

        // 返回本进程的所有线程id
		std::vector<pid_t> threads();
	}

}

#endif  // MUDUO_BASE_PROCESSINFO_H
