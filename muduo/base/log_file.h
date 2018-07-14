#pragma once
#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include "muduo_mutex.h"
#include "types.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{   
    // 文件末尾添加日志，不是线程安全
    namespace file_util{
        class append_file;
    }

    // muduo 库内部的记录日志的类
	class log_file : boost::noncopyable
	{
	public:

        /**
         * @basename : 要写入日志的文件名
         * @rollSize : 当日志文件达到多大时进行回滚
         * @threadSafe : 是否线程安全，即是否加锁 
         * @flushInterval : 日志文件缓冲区刷新间隔(单位/秒) 
         * @checkEeveryN : 每写入多少次日志检查是否需要进行刷新 
         */
		log_file(const string& basename,
			off_t rollSize,
			bool threadSafe = true,
			int flushInterval = 3,
			int checkEveryN = 1024);

		~log_file();

        /**
         * append -  在日志文件尾部添加新的日志
         * @logline : 要添加的日志
         * @len : logline 的长度
         */
		void append(const char* logline, int len);
        
        /**
         * flush - 刷新文件缓冲区 
         */
		void flush();

        /**
         * roll_file - 回滚文件，关闭之前打开的日志文件，重新打开新的日志文件 
         */
		bool roll_file();

	private:
		void append_unlocked(const char* logline, int len);

		static string get_logFileName(const string& basename, time_t* now);

		const string       _basename;
		const off_t        _roll_size;
		const int          _flush_interval;
		const int          _check_everyN;

		int _count; /* 距离上次刷新写入日志的次数 */

		boost::scoped_ptr<muduo_mutex> _mutex;
		time_t _start_of_period;
		time_t _last_roll; /* 上次回滚文件的时刻 */
		time_t _last_flush; /* 上次刷新文件的时刻 */
		boost::scoped_ptr<file_util::append_file> _file;
		const static int _kroll_per_seconds = 60 * 60 * 24; /* 日志文件每24小时回滚一次 */
	};

}
#endif  // MUDUO_BASE_LOGFILE_H
