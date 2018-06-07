#pragma once
#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include "blocking_queue.h"
#include "bounded_blocking_queue.h"
#include "muduo_latch.h"
#include "muduo_mutex.h"
#include "muduo_thread.h"
#include "log_stream.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

	class async_logging : boost::noncopyable
	{
	public:

		async_logging(const string& basename,
			off_t rollSize,
			int flushInterval = 3);

		~async_logging();

		void append(const char* logline, int len);

		void start();

		void stop();

	private:

		// declare but not define, prevent compiler-synthesized functions
		async_logging(const async_logging&);  // ptr_container
		void operator=(const async_logging&);  // ptr_container

		void thread_func();

		typedef muduo::detail::fixed_buffer<muduo::detail::klarge_buffer> buffer_t;
		typedef boost::ptr_vector<buffer_t> buffer_vector;
		typedef buffer_vector::auto_type buffer_ptr;

		const int                   _flush_interval;
		bool                        _running;
		string                      _basename;
		off_t                       _roll_size;
		muduo::muduo_thread         _thread;
		muduo::count_down_latch     _latch;
		muduo::muduo_mutex          _mutex;
		muduo::muduo_condition      _cond;
		buffer_ptr                  _current_buffer;
		buffer_ptr                  _next_buffer;
		buffer_vector               _buffers;
	};

}
#endif  // MUDUO_BASE_ASYNCLOGGING_H
