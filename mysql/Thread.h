#pragma once
#ifndef __THREAD_H__
#define __THREAD_H__
#include <pthread.h>
class CThreadNotify
{
public:
	CThreadNotify()
	{
		pthread_mutexattr_init(&m_mutexattr);
        // XXX recursive lock
		pthread_mutexattr_settype(&m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_mutex, &m_mutexattr);
		pthread_cond_init(&m_cond, NULL);
	}
	~CThreadNotify()
	{
		pthread_mutexattr_destroy(&m_mutexattr);
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}
	void Lock() { pthread_mutex_lock(&m_mutex); }
	void Unlock() { pthread_mutex_unlock(&m_mutex); }
	void Wait() { pthread_cond_wait(&m_cond, &m_mutex); }
	void Signal() { pthread_cond_signal(&m_cond); }
private:
	pthread_mutex_t 	m_mutex;
	pthread_mutexattr_t	m_mutexattr;

	pthread_cond_t 		m_cond;
};
#endif
