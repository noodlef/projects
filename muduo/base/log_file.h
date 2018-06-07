#pragma once
#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include "muduo_mutex.h"
#include "types.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

	namespace file_util
	{
		class append_file;
	}

	class log_file : boost::noncopyable
	{
	public:
		log_file(const string& basename,
			off_t rollSize,
			bool threadSafe = true,
			int flushInterval = 3,
			int checkEveryN = 1024);
		~log_file();

		void append(const char* logline, int len);
		void flush();
		bool roll_file();

	private:
		void append_unlocked(const char* logline, int len);

		static string get_logFileName(const string& basename, time_t* now);

		const string       _basename;
		const off_t        _roll_size;
		const int          _flush_interval;
		const int          _check_everyN;

		int _count;

		boost::scoped_ptr<muduo_mutex>               _mutex;
		time_t                                       _start_of_period;
		time_t                                       _last_roll;
		time_t                                       _last_flush;
		boost::scoped_ptr<file_util::append_file>    _file;

		const static int _kroll_per_seconds = 60 * 60 * 24;
	};

}
#endif  // MUDUO_BASE_LOGFILE_H
