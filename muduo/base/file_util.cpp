// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "file_util.h"
//#include "logger.h" // strerror_tl

#include <boost/static_assert.hpp>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace muduo;
///////////////////////////////////////// append_file /////////////////////////////////
file_util::append_file::append_file(string_arg filename)
	: _fp(::fopen(filename.c_str(), "ae")),  // 'e' for O_CLOEXEC
	  _written_bytes(0)
{
	assert(_fp);
	// 设置流的缓冲区
	::setbuffer(_fp, _buffer, sizeof _buffer);
	// posix_fadvise POSIX_FADV_DONTNEED ?
}

file_util::append_file::~append_file()
{
	::fclose(_fp);
}

void file_util::append_file::append(const char* logline, const size_t len)
{
	size_t n = write(logline, len);
	size_t remain = len - n;
	while (remain > 0)
	{
		size_t x = write(logline + n, remain);
		if (x == 0)
		{
			int err = ferror(_fp);
			if (err)
			{
				//fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
			}
			break;
		}
		n += x;
		remain = len - n; 
	}

	_written_bytes += len;
}

void file_util::append_file::flush()
{
	::fflush(_fp);
}

size_t file_util::append_file::write(const char* logline, size_t len)
{
	// #undef fwrite_unlocked
    // linux 环境中fwrite 是加锁的 
	return ::fwrite_unlocked(logline, 1, len, _fp);
}

/////////////////////////////////// read_small_file ///////////////////////////////////////
file_util::read_small_file::read_small_file(string_arg filename)
	: _fd(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
      _load_ok(false),
      _file_size(0)
{
	//_buf[0] = '\0';
    if(_fd < 0){
        printf("file_util: cant't open %s", filename.c_str());
    }
    read_to_buffer(NULL);
}

file_util::read_small_file::~read_small_file()
{
	if (_fd >= 0)
	{
		::close(_fd); // FIXME: check EINTR
	}
}

// return errno
template<typename String>
int file_util::read_small_file::read_to_string(int maxSize,
	String* content,
	int64_t* fileSize,
	int64_t* modifyTime,
	int64_t* createTime)
{
	BOOST_STATIC_ASSERT(sizeof(off_t) == 8);
	assert(content != NULL);
    if(!_load_ok)
    {
        return -1;
    }
	content->clear();
	if (_file_size)
	{
		struct stat statbuf;
		if (::fstat(_fd, &statbuf) == 0)
		{
			content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize),_file_size)));
			if (modifyTime)
			{
				*modifyTime = statbuf.st_mtime;
			}
			if (createTime)
			{
				*createTime = statbuf.st_ctime;
			}
		}
    }

    content->append(_buf, _file_size);
	return 0; 
}

int file_util::read_small_file::read_to_buffer(int* size)
{
    if(_fd < 0)
        return -1; 
    if(_load_ok)
    {
        if(size)
        {
            *size = _file_size; 
        }
        return 0;
    }
    
    struct stat statbuf;

    if(::fstat(_fd, &statbuf) == 0)
    {
        if(S_ISREG(statbuf.st_mode))
        {
            _file_size = statbuf.st_size;
        }
        else
            return EISDIR;
    }
    else
        return -1;

    int64_t to_read = std::min(_file_size, static_cast<int64_t>(sizeof(_buf)));
    int64_t cnt = 0;
    while(to_read > 0)
    {
        ssize_t n = ::read(_fd, _buf + cnt, to_read);
        if(n < 0)
        {
            break;
        }
        cnt += n;
        to_read -= n;
    }
    
    if(to_read == 0)
    {
        _load_ok = true;
        if(size) *size = _file_size;
    }
    return to_read ? -1 : 0;
}

template int file_util::read_file(string_arg filename,
	int maxSize,
	string* content,
	int64_t*, int64_t*, int64_t*);

template int file_util::read_small_file::read_to_string(
	int maxSize,
	string* content,
	int64_t*, int64_t*, int64_t*);

//template int file_util::read_file(string_arg filename,
//	int maxSize,
//	std::string* content,
//	int64_t*, int64_t*, int64_t*);

