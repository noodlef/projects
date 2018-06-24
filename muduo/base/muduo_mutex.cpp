#include"muduo_mutex.h"
#include<pthread.h>
#include<assert.h>
#include"current_thread.h"

// 检测返回值是否为0
#define MCHECK(ret) ({                   \
        typeof(ret) errnum = (ret);      \
        assert(errnum == 0);             \
        (void) errnum;                   \
    })
//////////////////////////////  muduo_mutex /////////////////////////////////////
using namespace muduo;
muduo_mutex::muduo_mutex()
	: _holder(0)
{
	MCHECK(pthread_mutex_init(&_mutex, NULL));
}

muduo_mutex::~muduo_mutex()
{
	assert(_holder == 0);
	MCHECK(pthread_mutex_destroy(&_mutex));
}

// must be called when locked, i.e. for assertion
bool muduo_mutex::is_locked_by_this_thread() const
{
	return _holder == current_thread::tid();
}

void muduo_mutex::assert_locked() const
{
	assert(is_locked_by_this_thread());
}

// internal usage
void muduo_mutex::lock()
{
	MCHECK(pthread_mutex_lock(&_mutex));
	assign_holder();
}

void muduo_mutex::unlock()
{
	unassign_holder();
	MCHECK(pthread_mutex_unlock(&_mutex));
}

pthread_mutex_t* muduo_mutex::get_pthread_mutex() /* non-const */
{
	return &_mutex;
}

void muduo_mutex::unassign_holder()
{
	_holder = 0;
}

void muduo_mutex::assign_holder()
{
	_holder = current_thread::tid();
}
