#pragma once
#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include "copyable.h"
#include "types.h"
#include <sys/time.h>

#include <boost/operators.hpp>

namespace muduo
{

	
	// Time stamp in UTC, in microseconds resolution.
	// This class is immutable.
	// It's recommended to pass it by value, since it's passed in register on x64.
	
	class time_stamp : public muduo::copyable,
		public boost::equality_comparable<time_stamp>,
		public boost::less_than_comparable<time_stamp>
	{
	public:
		
		time_stamp()
			: _micro_seconds_sinceEpoch(0)
		{ }

		// Constucts a Timestamp at specific time
		explicit time_stamp(int64_t microSecondsSinceEpochArg)
			: _micro_seconds_sinceEpoch(microSecondsSinceEpochArg)
		{ }

		void swap(time_stamp& that)
		{
			std::swap(_micro_seconds_sinceEpoch, that._micro_seconds_sinceEpoch);
		}

		// default copy/assignment/dtor are Okay
        // second.micro_second
		string to_string() const;

		string to_formatted_string(bool showMicroseconds = true) const;

		bool valid() const { return _micro_seconds_sinceEpoch > 0; }

		// for internal usage.
		int64_t micro_seconds_sinceEpoch() const { return _micro_seconds_sinceEpoch; }

		time_t seconds_since_epoch() const
		{
			return static_cast<time_t>(_micro_seconds_sinceEpoch / kmicro_seconds_perSecond);
		}



		// Get time of now.
		static time_stamp now();

		static time_stamp invalid()
		{
			return time_stamp();
		}

		static time_stamp from_unix_time(time_t t)
		{
			return from_unix_time(t, 0);
		}

		static time_stamp from_unix_time(time_t t, int microseconds)
		{
			return time_stamp(static_cast<int64_t>(t) * kmicro_seconds_perSecond + microseconds);
		}

		static const int kmicro_seconds_perSecond = 1000 * 1000;

	private:
		int64_t _micro_seconds_sinceEpoch;
	};





	inline bool operator<(time_stamp lhs, time_stamp rhs)
	{
		return lhs.micro_seconds_sinceEpoch() < rhs.micro_seconds_sinceEpoch();
	}

	inline bool operator==(time_stamp lhs, time_stamp rhs)
	{
		return lhs.micro_seconds_sinceEpoch() == rhs.micro_seconds_sinceEpoch();
	}

	// Gets time difference of two timestamps, result in seconds.
	// @param high, low
	// @return (high-low) in seconds
	// @c double has 52-bit precision, enough for one-microsecond
	// resolution for next 100 years.
	inline double time_difference(time_stamp high, time_stamp low)
	{
		int64_t diff = high.micro_seconds_sinceEpoch() - low.micro_seconds_sinceEpoch();
		return static_cast<double>(diff) / time_stamp::kmicro_seconds_perSecond;
	}

	
	// Add @c seconds to given timestamp.
	// @return timestamp+seconds as Timestamp
	inline time_stamp add_time(time_stamp timestamp, double seconds)
	{
		int64_t delta = static_cast<int64_t>(seconds * time_stamp::kmicro_seconds_perSecond);
		return time_stamp(timestamp.micro_seconds_sinceEpoch() + delta);
	}

}
#endif  // MUDUO_BASE_TIMESTAMP_H
