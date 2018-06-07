#include "async_logging.h"
#include "log_file.h"
#include "time_stamp.h"

#include <stdio.h>

using namespace muduo;

async_logging::async_logging(const string& basename,
	off_t rollSize,
	int flushInterval)
	: _flush_interval(flushInterval),
	  _running(false),
	  _basename(basename),
	  _roll_size(rollSize),
	  _thread(boost::bind(&async_logging::thread_func, this), "Logging"),
	  _latch(1),
	  _mutex(),
	  _cond(_mutex),
	  _current_buffer(new buffer_t),
	  _next_buffer(new buffer_t),
	  _buffers()
{
	_current_buffer->bzero();
	_next_buffer->bzero();
	_buffers.reserve(16);
}


async_logging::~async_logging()
{
	if (_running)
	{
		stop();
	}
}


void async_logging::start()
{
	_running = true;
	_thread.start();
	_latch.wait();
}

void async_logging::stop()
{
	_running = false;
	_cond.notify();
	_thread.join();
}

void async_logging::append(const char* logline, int len)
{
	muduo::mutex_lock_guard lock(_mutex);
	if (_current_buffer->avail() > len)
	{
		_current_buffer->append(logline, len);
	}
	else
	{
		_buffers.push_back(_current_buffer.release());

		if (_next_buffer)
		{
			_current_buffer = boost::ptr_container::move(_next_buffer);
		}
		else
		{
			_current_buffer.reset(new buffer_t); // Rarely happens
		}
		_current_buffer->append(logline, len);
		_cond.notify();
	}
}

void async_logging::thread_func()
{
	assert(_running == true);
	_latch.count_down();

	log_file output(_basename, _roll_size, false);
	buffer_ptr new_buffer1(new buffer_t);
	buffer_ptr new_buffer2(new buffer_t);
	new_buffer1->bzero();
	new_buffer2->bzero();

	buffer_vector buffers_to_write;
	buffers_to_write.reserve(16);

	while (_running)
	{
		assert(new_buffer1 && new_buffer1->length() == 0);
		assert(new_buffer2 && new_buffer2->length() == 0);
		assert(buffers_to_write.empty());

		{
			muduo::mutex_lock_guard lock(_mutex);
			if (_buffers.empty())  // unusual usage!
			{
				_cond.wait_for_seconds(_flush_interval);
			}
			_buffers.push_back(_current_buffer.release());
			_current_buffer = boost::ptr_container::move(new_buffer1);
			buffers_to_write.swap(_buffers);
			if (!_next_buffer)
			{
				_next_buffer = boost::ptr_container::move(new_buffer2);
			}
		}

		assert(!buffers_to_write.empty());

		if (buffers_to_write.size() > 25)
		{
			char buf[256];
			snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
				time_stamp::now().to_formatted_string().c_str(),
				buffers_to_write.size() - 2);
			fputs(buf, stderr);
			output.append(buf, static_cast<int>(strlen(buf)));
			buffers_to_write.erase(buffers_to_write.begin() + 2, buffers_to_write.end());
		}

		for (size_t i = 0; i < buffers_to_write.size(); ++i)
		{
			// FIXME: use unbuffered stdio FILE ? or use ::writev ?
			output.append(buffers_to_write[i].data(), buffers_to_write[i].length());
		}

		if (buffers_to_write.size() > 2)
		{
			// drop non-bzero-ed buffers, avoid trashing
			buffers_to_write.resize(2);
		}

		if (!new_buffer1)
		{
			assert(!buffers_to_write.empty());
			new_buffer1 = buffers_to_write.pop_back();
			new_buffer1->reset();
		}

		if (!new_buffer2)
		{
			assert(!buffers_to_write.empty());
			new_buffer2 = buffers_to_write.pop_back();
			new_buffer2->reset();
		}

		buffers_to_write.clear();
		output.flush();
	}
	output.flush();
}
