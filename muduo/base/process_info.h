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

        // ���ص�ǰ��½���û���
		string username();

        // euid
		uid_t euid();

        // ����ϵͳ����ʱ��
		time_stamp start_time();

        // ÿ����ϵͳʱ�ӵδ����
		int clockTicks_per_second();
        
        // ҳ��С
		int page_size();

        // �Ƿ���debug ģʽ
		bool is_debug_build();  // constexpr

        // ������
		string hostname();

        // ����������
		string procname();
		string_piece procname(const string& stat);

		// read /proc/self/status
        // ����״̬��ص���Ϣ
		string proc_status();

		// read /proc/self/stat
        // ������ص���Ϣ
		string proc_stat();

		// read /proc/self/task/tid/stat
        // �߳���ص���Ϣ
		string thread_stat();

		// readlink /proc/self/exe
        // ����ִ��·��
		string exe_path();
            
        // �򿪵��ļ�����
		int opened_files();

        // ����ܴ򿪵��ļ�����
		int max_open_files();

		struct cpu_time
		{
			double user_seconds;
			double system_seconds;

			cpu_time() : user_seconds(0.0), system_seconds(0.0)
			{ }
		};

        // cpu ʱ��
		cpu_time cpuTime();

        // ���̰������̸߳���
		int num_threads();

        // ���ر����̵������߳�id
		std::vector<pid_t> threads();
	}

}

#endif  // MUDUO_BASE_PROCESSINFO_H
