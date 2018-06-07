#pragma once
// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

#ifndef MUDUO_BASE_STRINGPIECE_H
#define MUDUO_BASE_STRINGPIECE_H

#include <string.h>
#include <iosfwd>    // for ostream forward-declaration

#include "types.h"
#ifndef MUDUO_STD_STRING
#include <string>
#endif

namespace muduo {

	// For passing C-style string argument to a function.
	class string_arg // copyable
	{
	public:
		string_arg(const char* str)
			: _str(str)
		{ }

		string_arg(const string& str)
			: _str(str.c_str())
		{ }

		const char* c_str() const { return _str; }

	private:
		const char* _str;
	};

	class string_piece {
	private:
		const char*      _ptr;
		int              _length;

	public:
		// We provide non-explicit singleton constructors so users can pass
		// in a "const char*" or a "string" wherever a "StringPiece" is
		// expected.
		string_piece()
			: _ptr(NULL), _length(0) 
		{ }

		string_piece(const char* str)
			: _ptr(str), _length(static_cast<int>(strlen(_ptr)))
		{ }

		string_piece(const unsigned char* str)
			: _ptr(reinterpret_cast<const char*>(str)),
			  _length(static_cast<int>(strlen(_ptr))) 
		{ }

		string_piece(const string& str)
			: _ptr(str.data()), _length(static_cast<int>(str.size())) 
		{ }

		string_piece(const char* offset, int len)
			: _ptr(offset), _length(len) 
		{ }

		// data() may return a pointer to a buffer with embedded NULs, and the
		// returned buffer may or may not be null terminated.  Therefore it is
		// typically a mistake to pass data() to a routine that expects a NUL
		// terminated string.  Use "as_string().c_str()" if you really need to do
		// this.  Or better yet, change your routine so it does not rely on NUL
		// termination.
		const char* data() const { return _ptr; }
		int size() const { return _length; }
		bool empty() const { return _length == 0; }
		const char* begin() const { return _ptr; }
		const char* end() const { return _ptr + _length; }

		void clear() { _ptr = NULL; _length = 0; }
		void set(const char* buffer, int len) { _ptr = buffer; _length = len; }
		void set(const char* str) 
		{
			_ptr = str;
			_length = static_cast<int>(strlen(str));
		}

		void set(const void* buffer, int len) 
		{
			_ptr = reinterpret_cast<const char*>(buffer);
			_length = len;
		}

		char operator[](int i) const { return _ptr[i]; }

		void remove_prefix(int n) {
			_ptr += n;
			_length -= n;
		}

		void remove_suffix(int n) {
			_length -= n;
		}

		bool operator==(const string_piece& x) const {
			return ((_length == x._length) &&
				(memcmp(_ptr, x._ptr, _length) == 0));
		}
		bool operator!=(const string_piece& x) const {
			return !(*this == x);
		}

#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)                             \
  bool operator cmp (const string_piece& x) const {                           \
    int r = memcmp(_ptr, x._ptr, _length < x._length ? _length : x._length); \
    return ((r auxcmp 0) || ((r == 0) && (_length cmp x._length)));          \
  }
		STRINGPIECE_BINARY_PREDICATE(<, <);
		STRINGPIECE_BINARY_PREDICATE(<= , <);
		STRINGPIECE_BINARY_PREDICATE(>= , >);
		STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

		int compare(const string_piece& x) const {
			int r = memcmp(_ptr, x._ptr, _length < x._length ? _length : x._length);
			if (r == 0) {
				if (_length < x._length) r = -1;
				else if (_length > x._length) r = +1;
			}
			return r;
		}

		string as_string() const {
			return string(data(), size());
		}

		void copy_to_string(string* target) const {
			target->assign(_ptr, _length);
		}

		// Does "this" start with "x"
		bool starts_with(const string_piece& x) const {
			return ((_length >= x._length) && (memcmp(_ptr, x._ptr, x._length) == 0));
		}
	};

}   // namespace muduo

	// ------------------------------------------------------------------
	// Functions used to create STL containers that use StringPiece
	//  Remember that a StringPiece's lifetime had better be less than
	//  that of the underlying string or char*.  If it is not, then you
	//  cannot safely store a StringPiece into an STL container
	// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
	// This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<muduo::string_piece> {
	typedef __true_type    has_trivial_default_constructor;
	typedef __true_type    has_trivial_copy_constructor;
	typedef __true_type    has_trivial_assignment_operator;
	typedef __true_type    has_trivial_destructor;
	typedef __true_type    is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const muduo::string_piece& piece);

#endif  // MUDUO_BASE_STRINGPIECE_H

