// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "process_info.h"
#include "current_thread.h"
#include "file_util.h"

#include <algorithm>

#include <assert.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h> // snprintf
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>

namespace muduo
{
	namespace detail
	{
		__thread int t_num_opened_files = 0;
		int fd_dir_filter(const struct dirent* d)
		{
			if (::isdigit(d->d_name[0]))
			{
				++t_num_opened_files;
			}
			return 0;
		}

		__thread std::vector<pid_t>* t_pids = NULL;
		int task_dir_filter(const struct dirent* d)
		{
			if (::isdigit(d->d_name[0]))
			{
				t_pids->push_back(atoi(d->d_name));
			}
			return 0;
		}

		int scan_dir(const char *dirpath, int(*filter)(const struct dirent *))
		{
			struct dirent** namelist = NULL;
			int result = ::scandir(dirpath, &namelist, filter, alphasort);
			assert(namelist == NULL);
			return result;
		}

		time_stamp g_startTime = time_stamp::now();
		// assume those won't change during the life time of a process.
		int g_clockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
		int g_pageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));
	}
}

using namespace muduo;
using namespace muduo::detail;

pid_t process_info::pid()
{
	return ::getpid();
}

string process_info::pid_string()
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", pid());
	return buf;
}

uid_t process_info::uid()
{
	return ::getuid();
}

string process_info::username()
{
	struct passwd pwd;
	struct passwd* result = NULL;
	char buf[8192];
	const char* name = "unknownuser";

	::getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
	if (result)
	{
		name = pwd.pw_name;
	}
	return name;
}

uid_t process_info::euid()
{
	return ::geteuid();
}

time_stamp process_info::start_time()
{
	return g_startTime;
}

int process_info::clockTicks_per_second()
{
	return g_clockTicks;
}

int process_info::page_size()
{
	return g_pageSize;
}

bool process_info::is_debug_build()
{
#ifdef NDEBUG
	return false;
#else
	return true;
#endif
}

string process_info::hostname()
{
	// HOST_NAME_MAX 64
	// _POSIX_HOST_NAME_MAX 255
	char buf[256];
	if (::gethostname(buf, sizeof buf) == 0)
	{
		buf[sizeof(buf) - 1] = '\0';
		return buf;
	}
	else
	{
		return "unknownhost";
	}
}

string process_info::procname()
{
	return procname(proc_stat()).as_string();
}

string_piece process_info::procname(const string& stat)
{
	string_piece name;
	size_t lp = stat.find('(');
	size_t rp = stat.rfind(')');
	if (lp != string::npos && rp != string::npos && lp < rp)
	{
		name.set(stat.data() + lp + 1, static_cast<int>(rp - lp - 1));
	}
	return name;
}

string process_info::proc_status()
{
	string result;
	file_util::read_file("/proc/self/status", 65536, &result);
	return result;
}

string process_info::proc_stat()
{
	string result;
	file_util::read_file("/proc/self/stat", 65536, &result,0,0,0);
	return result;
}

string process_info::thread_stat()
{
	char buf[64];
	snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", current_thread::tid());
	string result;
	file_util::read_file(buf, 65536, &result);
	return result;
}

string process_info::exe_path()
{
	string result;
	char buf[1024];
	size_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
	if (n > 0)
	{
		result.assign(buf, n);
	}
	return result;
}

int process_info::opened_files()
{
	t_num_opened_files = 0;
	scan_dir("/proc/self/fd", fd_dir_filter);
	return t_num_opened_files;
}

int process_info::max_open_files()
{
	struct rlimit rl;
	if (::getrlimit(RLIMIT_NOFILE, &rl))
	{
		return opened_files();
	}
	else
	{
		return static_cast<int>(rl.rlim_cur);
	}
}

process_info::cpu_time process_info::cpuTime()
{
	process_info::cpu_time t;
	struct tms tms;
	if (::times(&tms) >= 0)
	{
		const double hz = static_cast<double>(clockTicks_per_second());
		t.user_seconds = static_cast<double>(tms.tms_utime) / hz;
		t.system_seconds = static_cast<double>(tms.tms_stime) / hz;
	}
	return t;
}

int process_info::num_threads()
{
	int result = 0;
	string status = proc_status();
	size_t pos = status.find("Threads:");
	if (pos != string::npos)
	{
		result = ::atoi(status.c_str() + pos + 8);
	}
	return result;
}

std::vector<pid_t> process_info::threads()
{
	std::vector<pid_t> result;
	t_pids = &result;
	scan_dir("/proc/self/task", task_dir_filter);
	t_pids = NULL;
	std::sort(result.begin(), result.end());
	return result;
}
