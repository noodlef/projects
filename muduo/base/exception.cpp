// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "exception.h"

//#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

using namespace muduo;

muduo_exception::muduo_exception(const char* msg)
	: _message(msg)
{
	_fill_stack_trace();
}

muduo_exception::muduo_exception(const string& msg)
	: _message(msg)
{
	_fill_stack_trace();
}

muduo_exception::~muduo_exception() throw ()
{ }

const char* muduo_exception::what() const throw()
{
	return _message.c_str();
}

const char* muduo_exception::stack_trace() const throw()
{
	return _stack.c_str();
}

/*
  #include <execinfo.h>

  int backtrace(void **buffer, int size);

  backtrace() returns a backtrace for the calling program, in the array
  pointed to by buffer.A backtrace is the series of currently active
  function calls for the program.Each item in the array pointed to by
  buffer is of type void *, and is the return address from the
  corresponding stack frame.The size argument specifies the maximum
  number of addresses that can be stored in buffer.If the backtrace
  is larger than size, then the addresses corresponding to the size
  most recent function calls are returned; to obtain the complete
  backtrace, make sure that buffer and size are large enough.

  char **backtrace_symbols(void *const *buffer, int size);

  Given the set of addresses returned by backtrace() in buffer,
  backtrace_symbols() translates the addresses into an array of strings
  that describe the addresses symbolically.  The size argument
  specifies the number of addresses in buffer.  The symbolic
  representation of each address consists of the function name (if this
  can be determined), a hexadecimal offset into the function, and the
  actual return address (in hexadecimal).  The address of the array of
  string pointers is returned as the function result of
  backtrace_symbols().  This array is malloc(3)ed by
  backtrace_symbols(), and must be freed by the caller.  (The strings
  pointed to by the array of pointers need not and should not be
  freed.)

  void backtrace_symbols_fd(void *const *buffer, int size, int fd);

  backtrace_symbols_fd() takes the same buffer and size arguments as
  backtrace_symbols(), but instead of returning an array of strings to
  the caller, it writes the strings, one per line, to the file
  descriptor fd.  backtrace_symbols_fd() does not call malloc(3), and
  so can be employed in situations where the latter function might
  fail, but see NOTES.
*/
void muduo_exception::_fill_stack_trace()
{
	const int len = 200;
	void* buffer[len];
	int nptrs = ::backtrace(buffer, len);
	char** strings = ::backtrace_symbols(buffer, nptrs);
	if (strings)
	{
		for (int i = 0; i < nptrs; ++i)
		{
			// TODO demangle funcion name with abi::__cxa_demangle
			_stack.append(strings[i]);
			_stack.push_back('\n');
		}
		free(strings);
	}
}

