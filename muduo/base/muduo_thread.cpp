#include"muduo_thread.h"
#include"current_thread.h"
#include<assert.h>
//#include"logger.h"
#include"exception.h"

#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/prctl.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<linux/unistd.h>

#include<boost/static_assert.hpp>
#include<boost/type_traits/is_same.hpp>

typedef muduo::muduo_thread::thread_func thread_func;

namespace muduo {

	namespace detail
	{
		void after_fork()
		{
			muduo::current_thread::t_cached_tid = 0;
			muduo::current_thread::t_thread_name = "main";
			muduo::current_thread::tid();
		}

		class thread_name_initializer
		{
		public:
			thread_name_initializer()
			{
				muduo::current_thread::t_cached_tid = 0;
				muduo::current_thread::t_thread_name = "main";
				muduo::current_thread::tid();
				::pthread_atfork(NULL, NULL, &after_fork);
			}
		};

		thread_name_initializer init;

		struct thread_data
		{
			thread_func         _func;
			string              _name;
			pid_t*              _tid;
			count_down_latch*   _latch;


			thread_data(const thread_func& func, const string& name,
				pid_t* tid, count_down_latch* latch)
				: _func(func), _name(name), _tid(tid), _latch(latch)
			{ }
		};
	}
}

static void run_in_thread(thread_func func)
{
	using namespace muduo::current_thread;
	try
	{
		func();
		muduo::current_thread::t_thread_name = "finished";
	}
    catch (const muduo::muduo_exception& ex)
	{
		fprintf(stderr, "exception caught in Thread %s\n",t_thread_name);
	    fprintf(stderr, "reason: %s\n", ex.what());
		fprintf(stderr, "stack trace: %s\n", ex.stack_trace());
        muduo::current_thread::t_thread_name = "crashed";
		abort();/* kill the process */
	}
	catch (const std::exception& ex)
	{
		fprintf(stderr, "exception caught in Thread %s\n",t_thread_name); 
		fprintf(stderr, "reason: %s\n", ex.what());
		muduo::current_thread::t_thread_name = "crashed";
		abort();
	}
	catch (...)
	{
		fprintf(stderr, "unknown exception caught in Thread %s\n",t_thread_name); 
		muduo::current_thread::t_thread_name = "crashed";
		throw; // rethrow
	}
}

static void* start_thread(void* obj)
{
	using namespace muduo::current_thread;
	using namespace muduo::detail;
	thread_data* data = static_cast<thread_data*>(obj);
	*(data->_tid) = tid();
	data->_tid = NULL;
	data->_latch->count_down();
	data->_latch = NULL;

	t_thread_name =
		data->_name.empty() ? "muduo_thread" : data->_name.c_str();
	::prctl(PR_SET_NAME, muduo::current_thread::t_thread_name);/* syscall -> set thread_name */

	run_in_thread(data->_func);
	delete data; 
	return NULL;
}


/*
 *    muduo_thread
 */

using namespace muduo;

atomic_int32 muduo_thread::_num;

muduo_thread::muduo_thread(const thread_func& func, const string& n)
	: _started(false), _joined(false), _pthread_id(0),
	  _tid(0), _func(func), _name(n), _latch(1)
{
	_set_default_name();
}

muduo_thread::~muduo_thread()
{
	if (_started && !_joined)
	{
		pthread_detach(_pthread_id);
	}
}

void muduo_thread::_set_default_name()
{
	int num = _num.increment_and_get();
	if (_name.empty())
	{
		char buf[32];
		snprintf(buf, sizeof buf, "Thread%d", num);
		_name = buf;
	}
}

void muduo_thread::start()
{
	assert(!_started);
	_started = true;
	// FIXME: move(func_)
	detail::thread_data* data = new detail::thread_data(_func, _name, &_tid, &_latch);
	if (pthread_create(&_pthread_id, NULL, start_thread, data))
	{
		_started = false;
		delete data; // or no delete?
		//LOG_SYSFATAL << "Failed in pthread_create";
	}
	else
	{
		_latch.wait();
		assert(_tid > 0);
	}
}

int muduo_thread::join()
{
	assert(_started);
	assert(!_joined);
	_joined = true;
	return ::pthread_join(_pthread_id, NULL);
}
