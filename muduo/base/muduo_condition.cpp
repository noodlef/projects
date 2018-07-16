#include"muduo_condition.h"

#include<assert.h>
#include<time.h>

// 检测返回值是否为0
#define MCHECK(ret) ({                   \
        typeof(ret) errnum = (ret);      \
        assert(errnum == 0);             \
        (void) errnum;                   \
})


using namespace muduo;
muduo_condition::muduo_condition(muduo_mutex& mutex)
	: _mutex(mutex)
{
	MCHECK(pthread_cond_init(&_pcond, NULL));
}

muduo_condition::~muduo_condition()
{
	MCHECK(pthread_cond_destroy(&_pcond));
}

void muduo_condition::wait()
{
	_mutex.unassign_holder();
	MCHECK(pthread_cond_wait(&_pcond, _mutex.get_pthread_mutex()));
	_mutex.assign_holder();
}

// returns true if time out, false otherwise.
bool muduo_condition::wait_for_seconds(double seconds)
{
    this->wait();
    struct timespec abstime;
    ::clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += seconds; 

    _mutex.unassign_holder();
    int error = ::pthread_cond_timedwait(&_pcond, _mutex.get_pthread_mutex(), &abstime);
    _mutex.assign_holder();
    return error == ETIMEDOUT;
} 

// 唤醒一个等待该条件的线程
void muduo_condition::notify()
{
	MCHECK(pthread_cond_signal(&_pcond));
}

// 唤醒所有等待该条件的线程
void muduo_condition::notify_all()
{
	MCHECK(pthread_cond_broadcast(&_pcond));
}
