#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include "../copyable.h"
#include "../string_piece.h"
#include "../types.h"

#include "endian.h"

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>
//#include <unistd.h>  // ssize_t

namespace muduo
{
	namespace net
	{

		// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
		//
		// @code
		// +-------------------+------------------+------------------+
		// | prependable bytes |  readable bytes  |  writable bytes  |
		// |                   |     (CONTENT)    |                  |
		// +-------------------+------------------+------------------+
		// |                   |                  |                  |
		// 0      <=      readerIndex   <=   writerIndex    <=     size
		// @endcode
		class muduo_buffer : public muduo::copyable
		{
		public:
			static const size_t kcheap_prepend = 8;
			static const size_t kinitial_size = 1024;

			explicit muduo_buffer(size_t initialSize = kinitial_size)
				: _buffer(kcheap_prepend + initialSize),
				  _reader_index(kcheap_prepend),
				  _writer_index(kcheap_prepend)
			{
				assert(readable_bytes() == 0);
				assert(writable_bytes() == initialSize);
				assert(prependable_bytes() == kcheap_prepend);
			}

			// implicit copy-ctor, move-ctor, dtor and assignment are fine
			// NOTE: implicit move-ctor is added in g++ 4.6

			void swap(muduo_buffer& rhs)
			{
				_buffer.swap(rhs._buffer);
				std::swap(_reader_index, rhs._reader_index);
				std::swap(_writer_index, rhs._writer_index);
			}

			size_t readable_bytes() const
			{
				return _writer_index - _reader_index;
			}

			size_t writable_bytes() const
			{
				return _buffer.size() - _writer_index;
			}

			size_t prependable_bytes() const
			{
				return _reader_index;
			}

			const char* peek() const
			{
				return begin() + _reader_index;
			}

			const char* find_CRLF() const
			{
				// FIXME: replace with memmem()?
				const char* crlf = std::search(peek(), begin_write(), kCRLF, kCRLF + 2);
				return crlf == begin_write() ? NULL : crlf;
			}

			const char* find_CRLF(const char* start) const
			{
				assert(peek() <= start);
				assert(start <= begin_write());
				// FIXME: replace with memmem()?
				const char* crlf = std::search(start, begin_write(), kCRLF, kCRLF + 2);
				return crlf == begin_write() ? NULL : crlf;
			}

			const char* find_EOL() const
			{
				const void* eol = memchr(peek(), '\n', readable_bytes());
				return static_cast<const char*>(eol);
			}

			const char* find_EOL(const char* start) const
			{
				assert(peek() <= start);
				assert(start <= begin_write());
				const void* eol = memchr(start, '\n', begin_write() - start);
				return static_cast<const char*>(eol);
			}

			// retrieve returns void, to prevent
			// string str(retrieve(readableBytes()), readableBytes());
			// the evaluation of two functions are unspecified
			void retrieve(size_t len)
			{
				assert(len <= readable_bytes());
				if (len < readable_bytes())
				{
					_reader_index += len;
				}
				else
				{
					retrieve_all();
				}
			}

			void retrieve_until(const char* end)
			{
				assert(peek() <= end);
				assert(end <= begin_write());
				retrieve(end - peek());
			}

			void retrieve_int64()
			{
				retrieve(sizeof(int64_t));
			}

			void retrieve_int32()
			{
				retrieve(sizeof(int32_t));
			}

			void retrieve_int16()
			{
				retrieve(sizeof(int16_t));
			}

			void retrieve_int8()
			{
				retrieve(sizeof(int8_t));
			}

			void retrieve_all()
			{
				_reader_index = kcheap_prepend;
				_writer_index = kcheap_prepend;
			}

			string retrieve_all_as_string()
			{
				return retrieve_as_string(readable_bytes());
			}

			string retrieve_as_string(size_t len)
			{
				assert(len <= readable_bytes());
				string result(peek(), len);
				retrieve(len);
				return result;
			}

			string_piece to_string_piece() const
			{
				return string_piece(peek(), static_cast<int>(readable_bytes()));
			}

			void append(const string_piece& str)
			{
				append(str.data(), str.size());
			}

			void append(const char* /*restrict*/ data, size_t len)
			{
				ensure_writable_bytes(len);
				std::copy(data, data + len, begin_write());
				has_written(len);
			}

			void append(const void* /*restrict*/ data, size_t len)
			{
				append(static_cast<const char*>(data), len);
			}

			void ensure_writable_bytes(size_t len)
			{
				if (writable_bytes() < len)
				{
					make_space(len);
				}
				assert(writable_bytes() >= len);
			}

			char* begin_write()
			{
				return begin() + _writer_index;
			}

			const char* begin_write() const
			{
				return begin() + _writer_index;
			}

			void has_written(size_t len)
			{
				assert(len <= writable_bytes());
				_writer_index += len;
			}

			void unwrite(size_t len)
			{
				assert(len <= readable_bytes());
				_writer_index -= len;
			}

			
			// Append int64_t using network endian
			void append_int64(int64_t x)
			{
				int64_t be64 = sockets::host_to_network64(x);
				append(&be64, sizeof be64);
			}

			// Append int32_t using network endian
			void append_int32(int32_t x)
			{
				int32_t be32 = sockets::host_to_network32(x);
				append(&be32, sizeof be32);
			}

			void append_int16(int16_t x)
			{
				int16_t be16 = sockets::host_to_network32(x);
				append(&be16, sizeof be16);
			}

			void append_int8(int8_t x)
			{
				append(&x, sizeof x);
			}

			
			// Read int64_t from network endian
			// Require: buf->readableBytes() >= sizeof(int32_t)
			int64_t read_int64()
			{
				int64_t result = peek_int64();
				retrieve_int64();
				return result;
			}

			///
			/// Read int32_t from network endian
			///
			/// Require: buf->readableBytes() >= sizeof(int32_t)
			int32_t read_int32()
			{
				int32_t result = peek_int32();
				retrieve_int32();
				return result;
			}

			int16_t read_int16()
			{
				int16_t result = peek_int16();
				retrieve_int16();
				return result;
			}

			int8_t readInt8()
			{
				int8_t result = peek_int8();
				retrieve_int8();
				return result;
			}

		
			// Peek int64_t from network endian
			// Require: buf->readableBytes() >= sizeof(int64_t)
			int64_t peek_int64() const
			{
				assert(readable_bytes() >= sizeof(int64_t));
				int64_t be64 = 0;
				::memcpy(&be64, peek(), sizeof be64);
				return sockets::network_to_host64(be64);
			}

			
			// Peek int32_t from network endian
			// Require: buf->readableBytes() >= sizeof(int32_t)
			int32_t peek_int32() const
			{
				assert(readable_bytes() >= sizeof(int32_t));
				int32_t be32 = 0;
				::memcpy(&be32, peek(), sizeof be32);
				return sockets::network_to_host32(be32);
			}

			int16_t peek_int16() const
			{
				assert(readable_bytes() >= sizeof(int16_t));
				int16_t be16 = 0;
				::memcpy(&be16, peek(), sizeof be16);
				return sockets::network_to_host16(be16);
			}

			int8_t peek_Int8() const
			{
				assert(readable_bytes() >= sizeof(int8_t));
				int8_t x = *peek();
				return x;
			}

			
			// Prepend int64_t using network endian
			void prepend_int64(int64_t x)
			{
				int64_t be64 = sockets::host_to_network64(x);
				prepend(&be64, sizeof be64);
			}

			// Prepend int32_t using network endian
			void prepend_int32(int32_t x)
			{
				int32_t be32 = sockets::host_to_network32(x);
				prepend(&be32, sizeof be32);
			}

			void prepend_int16(int16_t x)
			{
				int16_t be16 = sockets::host_to_network16(x);
				prepend(&be16, sizeof be16);
			}

			void prepend_int8(int8_t x)
			{
				prepend(&x, sizeof x);
			}

			void prepend(const void* /*restrict*/ data, size_t len)
			{
				assert(len <= prependable_bytes());
				_reader_index -= len;
				const char* d = static_cast<const char*>(data);
				std::copy(d, d + len, begin() + _reader_index);
			}

			void shrink(size_t reserve)
			{
				// FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
				muduo_buffer other;
				other.ensure_writable_bytes(readable_bytes() + reserve);
				other.append(to_string_piece());
				swap(other);
			}

			size_t internal_capacity() const
			{
				return _buffer.capacity();
			}

			// Read data directly into buffer.
			// It may implement with readv(2)
			// @return result of read(2), @c errno is saved
			ssize_t read_fd(int fd, int* savedErrno);

		private:

			char* begin()
			{
				return &*_buffer.begin();
			}

			const char* begin() const
			{
				return &*_buffer.begin();
			}

			void make_space(size_t len)
			{
				if (writable_bytes() + prependable_bytes() < len + kcheap_prepend)
				{
					// FIXME: move readable data
					_buffer.resize(_writer_index + len);
				}
				else
				{
					// move readable data to the front, make space inside buffer
					assert(kcheap_prepend < _reader_index);
					size_t readable = readable_bytes();
					std::copy(begin() + _reader_index,
						begin() + _writer_index,
						begin() + kcheap_prepend);
					_reader_index = kcheap_prepend;
					_writer_index = _reader_index + readable;
					assert(readable == readable_bytes());
				}
			}

		private:
			std::vector<char>               _buffer;
			size_t                          _reader_index;
			size_t                          _writer_index;

			static const char               kCRLF[];
		};

	}
}

#endif  // MUDUO_NET_BUFFER_H
