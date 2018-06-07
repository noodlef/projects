#pragma once
#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include "string_piece.h"
#include "types.h"
#include <assert.h>
#include <string.h> // memcpy
#include <string>
#include <boost/noncopyable.hpp>

namespace muduo
{

	namespace detail
	{

		const int ksmall_buffer = 4000;
		const int klarge_buffer = 4000 * 1000;

		template<int SIZE>
		class fixed_buffer : boost::noncopyable
		{
		public:
			fixed_buffer()
				: _cur(_data)
			{
				set_cookie(cookie_start);
			}

			~fixed_buffer()
			{
				set_cookie(cookie_end);
			}

			void append(const char* /*restrict*/ buf, size_t len)
			{
				// FIXME: append partially
				if (implicit_cast<size_t>(avail()) > len)
				{
					memcpy(_cur, buf, len);
					_cur += len;
				}
			}

			const char* data() const { return _data; }
			int length() const { return static_cast<int>(_cur - _data); }

			// write to data_ directly
			char* current() { return _cur; }
			int avail() const { return static_cast<int>(end() - _cur); }
			void add(size_t len) { _cur += len; }

			void reset() { _cur = _data; }
			void bzero() { ::bzero(_data, sizeof _data); }

			// for used by GDB
			const char* debug_string();
			void set_cookie(void(*cookie)()) { _cookie = cookie; }
			// for used by unit test
			string to_string() const { return string(_data, length()); }
			string_piece to_stringPiece() const { return string_piece(_data, length()); }

		private:
			const char* end() const { return _data + sizeof _data; }
			// Must be outline function for cookies.
			static void cookie_start();
			static void cookie_end();

			void(*_cookie)();
			char      _data[SIZE];
			char*     _cur;
		};

	}

	class log_stream : boost::noncopyable
	{
		typedef log_stream self;
	public:
		typedef detail::fixed_buffer<detail::ksmall_buffer> buffer_t;

		self& operator<<(bool v);

		self& operator<<(short);
		self& operator<<(unsigned short);
		self& operator<<(int);
		self& operator<<(unsigned int);
		self& operator<<(long);
		self& operator<<(unsigned long);
		self& operator<<(long long);
		self& operator<<(unsigned long long);

		self& operator<<(const void*);
		self& operator<<(float v);
		self& operator<<(double);
		self& operator<<(char v);
		self& operator<<(const char* str);
		self& operator<<(const unsigned char* str);
		self& operator<<(const string& v);
		self& operator<<(const string_piece& v);
		self& operator<<(const buffer_t& v);
		

		void append(const char* data, int len) { _buffer.append(data, len); }
		const buffer_t& buffer() const { return _buffer; }
		void reset_buffer() { _buffer.reset(); }

	private:
		void static_check();
		template<typename T>
		void format_integer(T);
		buffer_t _buffer;
		static const int kmax_numeric_size = 32;
	};

	class fmt 
	{
	public:
		template<typename T>
		fmt(const char* fmt, T val);

		const char* data() const { return _buf; }
		int length() const { return _length; }

	private:
		char   _buf[32];
		int    _length;
	};

	inline log_stream& operator<<(log_stream& s, const fmt& ft)
	{
		s.append(ft.data(), ft.length());
		return s;
	}

}
#endif  // MUDUO_BASE_LOGSTREAM_H
