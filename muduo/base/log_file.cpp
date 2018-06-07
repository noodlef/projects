#include "log_file.h"

#include"file_util.h"
#include "process_info.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>


using namespace muduo;

log_file::log_file(const string&  basename,
	               off_t rollSize,
	               bool  threadSafe,
	               int   flushInterval,
	               int   checkEveryN)
	: _basename(basename),
	  _roll_size(rollSize),
	  _flush_interval(flushInterval),
	  _check_everyN(checkEveryN),
	  _count(0),
	  _mutex(threadSafe ? new muduo_lock : NULL),
	  _start_of_period(0),
	  _last_roll(0),
	  _last_flush(0)
{
	assert(basename.find('/') == string::npos);
	roll_file();
}

log_file::~log_file()
{ }

void log_file::append(const char* logline, int len)
{
	if (_mutex)
	{
		mutex_lock_guard lock(*_mutex);
		append_unlocked(logline, len);
	}
	else
	{
		append_unlocked(logline, len);
	}
}

void log_file::flush()
{
	if (_mutex)
	{
		mutex_lock_guard lock(*_mutex);
		_file->flush();
	}
	else
	{
		_file->flush();
	}
}

void log_file::append_unlocked(const char* logline, int len)
{
	_file->append(logline, len);

	if (_file->written_bytes() > _roll_size)
	{
		roll_file();
	}
	else
	{
		++_count;
		if (_count >= _check_everyN)
		{
			_count = 0;
			time_t now = ::time(NULL);
			time_t thisPeriod = now / _kroll_per_seconds * _kroll_per_seconds;
			if (thisPeriod != _start_of_period)
			{
				roll_file();
			}
			else if (now - _last_flush > _flush_interval)
			{
				_last_flush = now;
				_file->flush();
			}
		}
	}
}

bool log_file::roll_file()
{
	time_t now = 0;
	string filename = get_logFileName(_basename, &now);
	time_t start = now / _kroll_per_seconds * _kroll_per_seconds;

	if (now > _last_roll)
	{
		_last_roll = now;
		_last_flush = now;
		_start_of_period = start;
		_file.reset(new file_util::append_file(filename));
		return true;
	}
	return false;
}

string log_file::get_logFileName(const string& basename, time_t* now)
{
	string filename;
	filename.reserve(basename.size() + 64);
	filename = basename;

	char timebuf[32];
	struct tm tm;
	*now = time(NULL);
	::gmtime_r(now, &tm); // FIXME: localtime_r ?
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
	filename += timebuf;

	filename += process_info::hostname();

	char pidbuf[32];
	snprintf(pidbuf, sizeof pidbuf, ".%d", process_info::pid());
	filename += pidbuf;

	filename += ".log";

	return filename;
}
