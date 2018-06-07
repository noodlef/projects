#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_EXCEPTION_H
#define MUDUO_BASE_EXCEPTION_H

#include "types.h"
#include <exception>

namespace muduo
{

	class muduo_exception : public std::exception
	{
	public:
		explicit muduo_exception(const char* what);
		explicit muduo_exception(const string& what);
		virtual ~muduo_exception() throw();
		virtual const char* what() const throw();
		const char* stack_trace() const throw();

	private:
		void _fill_stack_trace();

		string   _message;
		string   _stack;
	};

}

#endif  // MUDUO_BASE_EXCEPTION_H
