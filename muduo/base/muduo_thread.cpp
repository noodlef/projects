#include"muduo_thread.h"
#include"current_thread.h"

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
			muduo_latch*        _latch;

			thread_data(const thread_func& func, const string& name,
				pid_t* tid, muduo_latch* latch)
				: _func(func), _name(name), _tid(tid), _latch(latch)
			{ }
		};
	}
}

static void run_in_thread(thread_func func)
{
	try
	{
		func();
		muduo::current_thread::t_thread_name = "finished";
	}
    catch (const muduo::exception& ex)
	{
		muduo::current_thread::t_thread_name = "crashed";
		fprintf(stderr, "exception caught in Thread %s\n", _name.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		fprintf(stderr, "stack trace: %s\n", ex.stack_trace());
		abort();/* kill the process */
	}
	catch (const std::exception& ex)
	{
		muduo::current_thread::t_thread_name = "crashed";
		fprintf(stderr, "exception caught in Thread %s\n", _name.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		abort();
	}
	catch (...)
	{
		muduo::current_thread::t_thread_name = "crashed";
		fprintf(stderr, "unknown exception caught in Thread %s\n", _name.c_str());
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
	delete obj;
	return NULL;
}






/* namespace current_thread */
namespace muduo {
	namespace current_thread {

		/* definitions */
		__thread int t_cached_tid = 0;
		__thread char t_tid_string[32];
		__thread int t_tid_string_length = 6;
		__thread const char* t_thread_name = "unknown";
		const bool same_type = boost::is_same<int, pid_t>::value;
		BOOST_STATIC_ASSERT(same_type);

		pid_t get_tid()
		{
			return static_cast<pid_t>(::syscall(SYS_gettid));
		}

		void cache_tid()
		{
			if (t_cached_tid == 0) {
				t_cached_tid = get_tid();
				t_tid_string_length = snprintf(t_tid_string,
					sizeof t_tid_string, "%5d ", t_cached_tid);
			}
		}

		bool is_main_thread()
		{
			return tid() == ::getpid();
		}

		void sleep_usec(int64_t usec)
		{
			struct timespec ts = { 0, 0 };
			ts.tv_sec = static_cast<time_t>(usec /
				times_tamp::kMicro_seconds_perSecond);
			ts.tv_nsec = static_cast<long>(usec %
				time_stamp::kMicro_seconds_perSecond * 1000);
			::nanosleep(&ts, NULL);/* task_interrputible */
		}

	}
}





/*
 *    muduo_thread
 */

using namespace muduo;

atomic_Int32 muduo_thread::_num;

muduo_thread::muduo_thread(const thread_func& func, const string& n)
	: _started(false), _joined(false), _pthread_Id(0),
	  _tid(0), _func(func), _name(n), _latch(1)
{
	_set_default_name();
}


muduo_thread::~muduo_thread()
{
	if (_started && !_joined)
	{
		pthread_detach(_pthread_Id);
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
	if (pthread_create(&_pthread_Id, NULL, start_thread, data))
	{
		_started = false;
		delete data; // or no delete?
		LOG_SYSFATAL << "Failed in pthread_create";
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
	return ::pthread_join(pthread_Id, NULL);
}
