#pragma once
#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include "log_stream.h"
#include "time_stamp.h"

namespace muduo
{

	class time_zone;

	class logger
	{
	public:
		enum LOG_LEVEL
		{
			TRACE,
			DEBUG,
			INFO,
			WARN,
			ERROR,
			FATAL,
			NUM_LOG_LEVELS,
		};

		// compile time calculation of basename of source file
		// 获取文件名 eg filename = "/etc/test.log" --> source_file = "test.log"
		class source_file
		{
		public:
			template<int N>
			inline source_file(const char(&arr)[N])
				: _data(arr),
				  _size(N - 1)
			{
				const char* slash = strrchr(data_, '/'); // builtin function
				if (slash)
				{
					_data = slash + 1;
					_size -= static_cast<int>(_data - arr);
				}
			}

			explicit source_file(const char* filename)
				: _data(filename)
			{
				const char* slash = strrchr(filename, '/');
				if (slash)
				{
					_data = slash + 1;
				}
				_size = static_cast<int>(strlen(_data));
			}

			const char*    _data;
			int            _size;
		};

		logger(source_file file, int line);
		logger(source_file file, int line, LOG_LEVEL level);
		logger(source_file file, int line, LOG_LEVEL level, const char* func);
		logger(source_file file, int line, bool toAbort);
		~logger();

		log_stream& stream() { return _impl._stream; }

		static LOG_LEVEL log_level();
		static void set_log_level(LOG_LEVEL level);

		typedef void(*output_func)(const char* msg, int len);
		typedef void(*flush_func)();
		static void set_output(output_func);
		static void set_flush(flush_func);
		static void set_time_zone(const time_zone& tz);

	private:

		class Impl
		{
		public:
			typedef logger::LOG_LEVEL LOG_LEVEL;
			Impl(LOG_LEVEL level, int old_errno, const source_file& file, int line);
			void format_time();
			void finish();

			time_stamp     _time;
			log_stream     _stream;
			LOG_LEVEL      _level;
			int            _line;
			source_file    _basename;
		};

		Impl _impl;

	};

	extern logger::LOG_LEVEL g_logLevel;

	inline logger::LOG_LEVEL logger::log_level()
	{
		return g_logLevel;
	}

	//
	// CAUTION: do not write:
	//
	// if (good)
	//   LOG_INFO << "Good news";
	// else
	//   LOG_WARN << "Bad news";
	//
	// this expends to
	//
	// if (good)
	//   if (logging_INFO)
	//     logInfoStream << "Good news";
	//   else
	//     logWarnStream << "Bad news";
	//
#define LOG_TRACE if (muduo::logger::log_level() <= muduo::logger::TRACE) \
  muduo::logger(__FILE__, __LINE__, muduo::logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::logger::log_level() <= muduo::logger::DEBUG) \
  muduo::logger(__FILE__, __LINE__, muduo::logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::logger::log_level() <= muduo::logger::INFO) \
  muduo::logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::logger(__FILE__, __LINE__, muduo::logger::WARN).stream()
#define LOG_ERROR muduo::logger(__FILE__, __LINE__, muduo::logger::ERROR).stream()
#define LOG_FATAL muduo::logger(__FILE__, __LINE__, muduo::logger::FATAL).stream()
#define LOG_SYSERR muduo::logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::logger(__FILE__, __LINE__, true).stream()

	const char* strerror_tl(int savedErrno);

	// Taken from glog/logging.h
	// Check that the input is non NULL.  This very useful in constructor
	// initializer lists.

#define CHECK_NOTNULL(val) \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

	// A small helper for CHECK_NOTNULL().
	template <typename T>
	T* check_not_null(logger::source_file file, int line, const char *names, T* ptr)
	{
		if (ptr == NULL)
		{
			logger(file, line, logger::FATAL).stream() << names;
		}
		return ptr;
	}

}

#endif  // MUDUO_BASE_LOGGING_H
