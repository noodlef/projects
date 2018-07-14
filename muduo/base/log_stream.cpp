#include "log_stream.h"

#include <algorithm>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::detail;

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wtautological-compare"
#else
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

namespace muduo
{
	namespace detail
	{

		const char digits[] = "9876543210123456789";
		const char* zero = digits + 9;
		BOOST_STATIC_ASSERT(sizeof(digits) == 20);

		const char digitsHex[] = "0123456789ABCDEF";
		BOOST_STATIC_ASSERT(sizeof digitsHex == 17);

		// Efficient Integer to String Conversions, by Matthew Wilson.
		template<typename T>
		size_t convert(char buf[], T value)
		{
			T i = value;
			char* p = buf;

			do
			{
				int lsd = static_cast<int>(i % 10);
				i /= 10;
				*p++ = zero[lsd];
			} while (i != 0);

			if (value < 0)
			{
				*p++ = '-';
			}
			*p = '\0';
			std::reverse(buf, p);

			return p - buf;
		}

		size_t convert_hex(char buf[], uintptr_t value)
		{
			uintptr_t i = value;
			char* p = buf;

			do
			{
				int lsd = static_cast<int>(i % 16);
				i /= 16;
				*p++ = digitsHex[lsd];
			} while (i != 0);

			*p = '\0';
			std::reverse(buf, p);

			return p - buf;
		}

		template class fixed_buffer<ksmall_buffer>;
		template class fixed_buffer<klarge_buffer>;

	}
}


/////////////////////////////////// fixed_buffer /////////////////////////////////
template<int SIZE>
const char* fixed_buffer<SIZE>::debug_string()
{
	*_cur = '\0';
	return _data;
}

template<int SIZE>
void fixed_buffer<SIZE>::cookie_start()
{ }

template<int SIZE>
void fixed_buffer<SIZE>::cookie_end()
{ }

//////////////////////////////////// log_stream ///////////////////////////////
void log_stream::static_check()
{
	BOOST_STATIC_ASSERT(kmax_numeric_size - 10 > std::numeric_limits<double>::digits10);
	BOOST_STATIC_ASSERT(kmax_numeric_size - 10 > std::numeric_limits<long double>::digits10);
	BOOST_STATIC_ASSERT(kmax_numeric_size - 10 > std::numeric_limits<long>::digits10);
	BOOST_STATIC_ASSERT(kmax_numeric_size - 10 > std::numeric_limits<long long>::digits10);
}

template<typename T>
void log_stream::format_integer(T v)
{
	if (_buffer.avail() >= kmax_numeric_size)
	{
		size_t len = convert(_buffer.current(), v);
		_buffer.add(len);
	}
}

log_stream& log_stream::operator<<(bool v)
{
	_buffer.append(v ? "1" : "0", 1);
	return *this;
}

log_stream& log_stream::operator<<(short v)
{
	*this << static_cast<int>(v);
	return *this;
}

log_stream& log_stream::operator<<(unsigned short v)
{
	*this << static_cast<unsigned int>(v);
	return *this;
}

log_stream& log_stream::operator<<(int v)
{
	format_integer(v);
	return *this;
}

log_stream& log_stream::operator<<(unsigned int v)
{
	format_integer(v);
	return *this;
}

log_stream& log_stream::operator<<(long v)
{
	format_integer(v);
	return *this;
}

log_stream& log_stream::operator<<(unsigned long v)
{
	format_integer(v);
	return *this;
}

log_stream& log_stream::operator<<(long long v)
{
	format_integer(v);
	return *this;
}

log_stream& log_stream::operator<<(unsigned long long v)
{
	format_integer(v);
	return *this;
}

log_stream& log_stream::operator<<(const void* p)
{
	uintptr_t v = reinterpret_cast<uintptr_t>(p);
	if (_buffer.avail() >= kmax_numeric_size)
	{
		char* buf = _buffer.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = convert_hex(buf + 2, v);
		_buffer.add(len + 2);
	}
	return *this;
}

log_stream& log_stream::operator<<(float v)
{
	*this << static_cast<double>(v);
	return *this;
}

log_stream& log_stream::operator<<(char v)
{
	_buffer.append(&v, 1);
	return *this;
}
// FIXME: replace this with Grisu3 by Florian Loitsch.
log_stream& log_stream::operator<<(double v)
{
	if (_buffer.avail() >= kmax_numeric_size)
	{
		int len = snprintf(_buffer.current(), kmax_numeric_size, "%.12g", v);
		_buffer.add(len);
	}
	return *this;
}

log_stream& log_stream::operator<<(const char* str)
{
	if (str)
	{
		_buffer.append(str, strlen(str));
	}
	else
	{
		_buffer.append("(null)", 6);
	}
	return *this;
}

log_stream& log_stream::operator<<(const unsigned char* str)
{
	return operator<<(reinterpret_cast<const char*>(str));
}

log_stream& log_stream::operator<<(const string& v)
{
	_buffer.append(v.c_str(), v.size());
	return *this;
}

log_stream& log_stream::operator<<(const string_piece& v)
{
	_buffer.append(v.data(), v.size());
	return *this;
}

log_stream& log_stream::operator<<(const buffer_t& v)
{
	*this << v.to_stringPiece();
	return *this;
}

/////////////////////////////////////// fmt /////////////////////////////////////
template<typename T>
fmt::fmt(const char* fmt, T val)
{
	BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value == true);

	_length = snprintf(_buf, sizeof _buf, fmt, val);
	assert(static_cast<size_t>(_length) < sizeof _buf);
}

// Explicit instantiations
template fmt::fmt(const char* fmt, char);
template fmt::fmt(const char* fmt, short);
template fmt::fmt(const char* fmt, unsigned short);
template fmt::fmt(const char* fmt, int);
template fmt::fmt(const char* fmt, unsigned int);
template fmt::fmt(const char* fmt, long);
template fmt::fmt(const char* fmt, unsigned long);
template fmt::fmt(const char* fmt, long long);
template fmt::fmt(const char* fmt, unsigned long long);
template fmt::fmt(const char* fmt, float);
template fmt::fmt(const char* fmt, double);
