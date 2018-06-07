#include"logger.h"

#include "current_thread.h"
#include "time_stamp.h"
#include "time_zone.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace muduo
{

	

	__thread char    t_errno_buf[512];
	__thread char    t_time[32];
	__thread time_t  t_last_second;

	const char* strerror_tl(int savedErrno)
	{
		// strerror_r function returns a string describing 
		// the error code passed in the argument errnum
		return strerror_r(savedErrno, t_errno_buf, sizeof t_errno_buf);
	}

	logger::LOG_LEVEL init_LogLevel()
	{
		if (::getenv("MUDUO_LOG_TRACE"))
			return logger::TRACE;
		else if (::getenv("MUDUO_LOG_DEBUG"))
			return logger::DEBUG;
		else
			return logger::INFO;
	}

	logger::LOG_LEVEL g_logLevel = init_logLevel();

	const char* log_level_name[logger::NUM_LOG_LEVELS] =
	{
		"TRACE ",
		"DEBUG ",
		"INFO  ",
		"WARN  ",
		"ERROR ",
		"FATAL ",
	};

	// helper class for known string length at compile time
	class T
	{
	public:
		T(const char* str, unsigned len)
			:_str(str),
			 _len(len)
		{
			assert(strlen(str) == _len);
		}

		const char*    _str;
		const unsigned _len;
	};

	inline log_stream& operator<<(log_stream& s, T v)
	{
		s.append(v._str, v._len);
		return s;
	}

	inline log_stream& operator<<(log_stream& s, const logger::source_file& v)
	{
		s.append(v._data, v._size);
		return s;
	}

	void default_output(const char* msg, int len)
	{
		size_t n = fwrite(msg, 1, len, stdout);
		//FIXME check n
		(void)n;
	}

	void default_flush()
	{
		fflush(stdout);
	}

	logger::output_func g_output = defaultOutput;
	logger::flush_func  g_flush = defaultFlush;
	time_zone g_logTimeZone;

}



using namespace muduo;
/////////////////////////////////////// logger::Impl //////////////////////////////////////////
logger::Impl::Impl(LOG_LEVEL level, int savedErrno, const source_file& file, int line)
	: _time(time_stamp::now()),
	  _stream(),
	  _level(level),
	  _line(line),
	  _basename(file)
{
	format_time();
	current_thread::tid();
	_stream << T(current_thread::tid_string(), current_thread::tid_string_length());
	_stream << T(log_level_name[level], 6);
	if (savedErrno != 0)
	{
		_stream << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
	}
}

void logger::Impl::format_time()
{
	int64_t micro_seconds_sinceEpoch = _time.micro_seconds_sinceEpoch();
	time_t seconds = static_cast<time_t>(micro_seconds_sinceEpoch / time_stamp::kmicro_seconds_perSecond);
	int microseconds = static_cast<int>(micro_seconds_sinceEpoch % time_stamp::kmicro_seconds_perSecond);
	if (seconds != t_last_second)
	{
		t_lastSecond = seconds;
		struct tm tm_time;
		if (g_logTimeZone.valid())
		{
			tm_time = g_log_timeZone.to_local_time(seconds);
		}
		else
		{
			::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
		}

		int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
		assert(len == 17); (void)len;
	}

	if (g_logTimeZone.valid())
	{
		fmt us(".%06d ", microseconds);
		assert(us.length() == 8);
		_stream << T(t_time, 17) << T(us.data(), 8);
	}
	else
	{
		fmt us(".%06dZ ", microseconds);
		assert(us.length() == 9);
		_stream << T(t_time, 17) << T(us.data(), 9);
	}
}

void logger::Impl::finish()
{
	_stream << " - " << _basename << ':' << _line << '\n';
}


/////////////////////////////// logger ////////////////////////////////////
logger::logger(source_file file, int line)
	: _impl(INFO, 0, file, line)
{ }

logger::logger(source_file file, int line, LOG_LEVEL level, const char* func)
	: _impl(level, 0, file, line)
{
	_impl._stream << func << ' ';
}

logger::logger(source_file file, int line, LOG_LEVEL level)
	: _impl(level, 0, file, line)
{ }

logger::logger(source_file file, int line, bool toAbort)
	: _impl(toAbort ? FATAL : ERROR, errno, file, line)
{ }

logger::~logger()
{
	_impl.finish();
	const log_stream::buffer_t& buf(stream().buffer());
	g_output(buf.data(), buf.length());
	if (_impl._level == FATAL)
	{
		g_flush();
		abort();
	}
}

void logger::set_log_level(logger::LOG_LEVEL level)
{
	g_logLevel = level;
}

void logger::set_output(output_func out)
{
	g_output = out;
}

void logger::set_flush(flush_func flush)
{
	g_flush = flush;
}

void logger::set_time_zone(const time_zone& tz)
{
	g_logTimeZone = tz;
}
