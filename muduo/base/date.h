#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_DATE_H
#define MUDUO_BASE_DATE_H

#include"copyable.h"
#include "type.h"

struct tm;

namespace muduo
{

	
	class date : public muduo::copyable
	{
	public:

		struct year_month_day_t
		{
			int year;   // [1900..2500]
			int month;  // [1..12]
			int day;    // [1..31]
		};

		static const int kdays_per_week = 7;
		static const int kJulianDayOf1970_01_01;

		date()
			: _julian_day_number(0)
		{ }

		date(int year, int month, int day);

		explicit date(int julianDayNum)
			: _julian_day_number(julianDayNum)
		{ }

		explicit date(const struct tm&);

		// default copy/assignment/dtor are Okay

		void swap(date& that)
		{
			std::swap(_julian_day_number, that._julian_day_number);
		}

		bool valid() const { return _julian_day_number > 0; }

		
		string to_iso_string() const;

		year_month_day_t year_month_day() const;

		int year() const
		{
			return year_month_day().year;
		}

		int month() const
		{
			return year_month_day().month;
		}

		int day() const
		{
			return year_month_day().day;
		}

		// [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
		int week_day() const
		{
			return (_julian_day_number + 1) % kdays_per_week;
		}

		int julian_day_number() const { return _julian_day_number; }

	private:
		int _julian_day_number;
	};

	inline bool operator<(date x, date y)
	{
		return x.julian_day_number() < y.julian_day_number();
	}

	inline bool operator==(date x, date y)
	{
		return x.julian_day_number() == y.julian_day_number();
	}

}
#endif  // MUDUO_BASE_DATE_H

