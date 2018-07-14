#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_BASE_FILEUTIL_H
#define MUDUO_BASE_FILEUTIL_H

#include "string_piece.h" // for string_arg
#include "boost/noncopyable.hpp"
#include <sys/types.h>  // for off_t

namespace muduo
{

	namespace file_util
	{

		// read small file < 64KB
		class read_small_file : boost::noncopyable
		{
		public:
			read_small_file(string_arg filename);
			~read_small_file();

			// return errno
			template<typename String>
			int read_to_string(int      maxSize,
				               String*  content,
				               int64_t* fileSize,
				               int64_t* modifyTime,
				               int64_t* createTime);

			// Read at maxium kBufferSize into buf_
			// return errno
			int read_to_buffer(int* size);

			const char* buffer() const { return _buf; }

			static const int kbuffer_size = 64 * 1024;

		private:
			int         _fd;
            bool        _load_ok;
            int64_t     _file_size;
			char        _buf[kbuffer_size];
		};

		// read the file content, returns errno if error happens.
		template<typename String>
		int read_file(string_arg filename,
			int maxSize,
			String* content,
			int64_t* fileSize = NULL,
			int64_t* modifyTime = NULL,
			int64_t* createTime = NULL)
		{
			read_small_file file(filename);
			return file.read_to_string(maxSize, content, fileSize, modifyTime, createTime);
		}

		// not thread safe
		class append_file : boost::noncopyable
		{
		public:
			explicit append_file(string_arg filename);

			~append_file();

			void append(const char* logline, const size_t len);

			void flush();

			off_t written_bytes() const { return _written_bytes; }

		private:

			size_t write(const char* logline, size_t len);

			FILE*           _fp;
			char            _buffer[64 * 1024];
			off_t           _written_bytes;
		};
	}

}

#endif  // MUDUO_BASE_FILEUTIL_H

