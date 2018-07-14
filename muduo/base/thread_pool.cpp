// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "thread_pool.h"

#include "exception.h"

#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>

using namespace muduo;

thread_pool::thread_pool(const string& nameArg)
	: _mutex(),
	  _not_empty(_mutex),
	  _not_full(_mutex),
	  _name(nameArg),
	  _max_queue_size(0),
	  _running(false)
{ }

thread_pool::~thread_pool()
{
	if (_running)
	{
		stop();
	}
}

void thread_pool::start(int numThreads)
{
	assert(_threads.empty());
    assert(_running == false);
	_running = true;
	_threads.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		char id[32];
		snprintf(id, sizeof id, "%d", i + 1);
		_threads.push_back(new muduo::muduo_thread(
			           boost::bind(&thread_pool::run_in_thread, this), _name + id));
		_threads[i].start();
	}
	if (numThreads == 0 && _thread_init_callback)
	{
		_thread_init_callback();
	}
}

void thread_pool::stop()
{
	{
		mutex_lock_guard lock(_mutex);
		_running = false;
		_not_empty.notify_all();
	}
	for_each(_threads.begin(),
		     _threads.end(),
		     boost::bind(&muduo::muduo_thread::join, _1));
}

size_t thread_pool::queue_size() const
{
	mutex_lock_guard lock(_mutex);
	return _queue.size();
}

void thread_pool::run(const task_t& task)
{
	if (_threads.empty())
	{
		task();
	}
	else
	{
		mutex_lock_guard lock(_mutex);
		while (is_full())
		{
			_not_full.wait();
		}
		assert(!is_full());
		_queue.push_back(task);
		_not_empty.notify();
	}
}



thread_pool::task_t thread_pool::take()
{
	mutex_lock_guard lock(_mutex);
	// always use a while-loop, due to spurious wakeup
	while (_queue.empty() && _running)
	{
		_not_empty.wait();
	}
	task_t task;
	if (!_queue.empty())
	{
		task = _queue.front();
		_queue.pop_front();
		if (_max_queue_size > 0)
		{
			_not_full.notify();
		}
	}
	return task;
}

bool thread_pool::is_full() const
{
	_mutex.assert_locked();
	return _max_queue_size > 0 && _queue.size() >= _max_queue_size;
}

void thread_pool::run_in_thread()
{
	try
	{
		if (_thread_init_callback)
		{
			_thread_init_callback();
		}
		while (_running)
		{
			task_t task(take());
			if (task)
			{
				task();
			}
		}
	}
	catch (const muduo_exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", _name.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		fprintf(stderr, "stack trace: %s\n", ex.stack_trace());
		abort();
	}
	catch (const std::exception& ex)
	{
		fprintf(stderr, "exception caught in ThreadPool %s\n", _name.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		abort();
	}
	catch (...)
	{
		fprintf(stderr, "unknown exception caught in ThreadPool %s\n", _name.c_str());
		throw; // rethrow
	}
}
