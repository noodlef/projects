// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "buffer.h"

#include "sockets_ops.h"

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const char muduo_buffer::kCRLF[] = "\r\n";

const size_t muduo_buffer::kcheap_prepend;
const size_t muduo_buffer::kinitial_size;

ssize_t muduo_buffer::read_fd(int fd, int* savedErrno)
{
	// saved an ioctl()/FIONREAD call to tell how much to read
	char extrabuf[65536];
	struct iovec vec[2];
	const size_t writable = writable_bytes();
	vec[0].iov_base = begin() + _writer_index;
	vec[0].iov_len = writable;
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;
	// when there is enough space in this buffer, don't read into extrabuf.
	// when extrabuf is used, we read 128k-1 bytes at most.
	const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
	const ssize_t n = sockets::readv(fd, vec, iovcnt);
	if (n < 0)
	{
		*savedErrno = errno;
	}
	else if (implicit_cast<size_t>(n) <= writable)
	{
		_writer_index += n;
	}
	else
	{
		_writer_index = _buffer.size();
		append(extrabuf, n - writable);
	}
	return n;
}
