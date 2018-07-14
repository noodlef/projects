#include"time_stamp.h"

#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h> // for PRID_64 ��ƽ̨��ӡ64λ����
#include<boost/static_assert.hpp>

using namespace muduo;

BOOST_STATIC_ASSERT(sizeof(time_stamp) == sizeof(int64_t));

string time_stamp::to_string() const
{
	char buf[32] = { 0 };
	int64_t seconds = _micro_seconds_sinceEpoch / kmicro_seconds_perSecond;
	int64_t microseconds = _micro_seconds_sinceEpoch % kmicro_seconds_perSecond;
	snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
	return buf;
}

string time_stamp::to_formatted_string(bool showMicroseconds) const
{
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(_micro_seconds_sinceEpoch / kmicro_seconds_perSecond);
	struct tm tm_time;
    // ������ʱ��ת��Ϊ�����׼ʱ�䣨��������ʱ�䣩
	::gmtime_r(&seconds, &tm_time);
    // ��ʾ΢��
	if (showMicroseconds)
	{
		int microseconds = static_cast<int>(_micro_seconds_sinceEpoch % kmicro_seconds_perSecond);
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
			microseconds);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
	}
	return buf;
}

time_stamp time_stamp::now()
{
	struct timeval tv;
    // �õ�epoch ʱ�䣬΢���
	::gettimeofday(&tv, NULL);
	int64_t seconds = tv.tv_sec;
	return time_stamp(seconds * kmicro_seconds_perSecond + tv.tv_usec);
}
