#pragma once
/*================================================================
*   Copyright (C) 2014 All rights reserved.
*
*   文件名称：Thread.h
*   创 建 者：Zhang Yuanhao
*   邮    箱：bluefoxah@gmail.com
*   创建日期：2014年09月10日
*   描    述：
*
#pragma once
================================================================*/

#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>


class mix_thread
{
public:
	// XXX
	mix_thread() { m_thread_id = 0; }
	virtual ~mix_thread() { }
	static void* start_routine(void* arg)
	{
		mix_thread* m_thread = (mix_thread*)arg;
		m_thread->on_thread_run();
		return NULL;
	}
	virtual void start_thread(void)
	{
		pthread_create(&m_thread_id, NULL, start_routine, this);
		return ;
	}
	virtual void on_thread_run(void) = 0;
protected:
    pthread_t	m_thread_id;
};



class event_thread : public mix_thread
{
public:
	event_thread() { running = false; }
	virtual ~event_thread()
	{
		stop_thread();
	}
	// XXX
	virtual void on_thread_tick(void) = 0;
	void on_thread_run(void)
	{
		while (running)
		{
			on_thread_tick();
		}
		return;
	}
	void start_thread()
	{
		running = true;
		mix_thread::start_thread();
	}
	virtual void stop_thread() { running = false; }
	bool is_running() { return running; }
private:
	bool 	running;
};


class thread_notify
{
public:
	thread_notify()
	{
		// 设置互斥量属性
		pthread_mutexattr_init(&m_mutexattr);
		pthread_mutexattr_settype(&m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
		// init mutex
		pthread_mutex_init(&m_mutex, &m_mutexattr);
		// init cond
		pthread_cond_init(&m_cond, NULL);
	}
	~thread_notify()
	{

		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}

	void lock() { pthread_mutex_lock(&m_mutex); }
	void unlock() { pthread_mutex_unlock(&m_mutex); }
	void wait() { pthread_cond_wait(&m_cond, &m_mutex); }
	void signal() { pthread_cond_signal(&m_cond); }
private:
	pthread_mutex_t 	m_mutex;
	pthread_mutexattr_t	m_mutexattr;
	pthread_cond_t 		m_cond;
};

#endif
