#include"current_thread.h"
#include"time_stamp.h"

#include<boost/static_assert.hpp>
#include<boost/type_traits/is_same.hpp>

#include<sys/syscall.h>
#include<time.h> // for nanosleep

/* namespace current_thread */
namespace muduo 
{
	namespace current_thread 
    {

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
        
        // @usec ΢�λ
		void sleep_usec(int64_t usec)
		{
			struct timespec ts = { 0, 0 };
			ts.tv_sec = static_cast<time_t>(usec /
				time_stamp::kmicro_seconds_perSecond);
			ts.tv_nsec = static_cast<long>(usec %
				time_stamp::kmicro_seconds_perSecond * 1000);
			::nanosleep(&ts, NULL);/* task_interrputible */
		} 
	}// end of current_thread 

}// end of muduo


