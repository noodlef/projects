#pragma once
#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include "ostype.h"
#include "Thread.h"
#include "Task.h"

#include <pthread.h>
#include <list>
using namespace std;

// 工作线程
class worker_thread {
public:
	worker_thread()
	{
		m_task_cnt = 0;
	}
	~worker_thread() { }
	static void* start_routine(void* arg)
	{
		worker_thread* thread = (worker_thread*)arg;
		thread->execute();
		return NULL;
	}
	void start()
	{
		pthread_create(&m_thread_id, NULL, start_routine, this);
		return;
	}
	void execute()
	{
		while (true) {
			// 从 task_list 中取出任务
			m_thread_notify.lock();
			while (m_task_list.empty())
				m_thread_notify.wait();
			mix_task* task = m_task_list.front();
			m_task_list.pop_front();
			m_thread_notify.unlock();
			// do stuff
			task->run();
			delete task;
			m_task_cnt++;
		}
		return;
	}
	void push_task(mix_task* task)
	{
		m_thread_notify.lock();
		m_task_list.push_back(task);
		m_thread_notify.signal();
		m_thread_notify.unlock();
	}

	void set_thread_idx(uint32_t idx) { m_thread_idx = idx; }
private:

	uint32_t		m_thread_idx;
	uint32_t		m_task_cnt;
    pthread_t		m_thread_id;
	thread_notify	m_thread_notify;
	list<mix_task*>	m_task_list;
};


// 线程池
class thread_pool {
public:
	thread_pool()
	{
		m_worker_size = 0;
		m_worker_list = NULL;
	}
	virtual ~thread_pool(){ }

	int  init(uint32_t worker_size)
	{
		m_worker_size = worker_size;
		m_worker_list = new worker_thread[m_worker_size];
		if (!m_worker_list) {
			return 1;
		}
		for (uint32_t i = 0; i < m_worker_size; i++) {
			m_worker_list[i].set_thread_idx(i);
			m_worker_list[i].start();
		}
		return 0;
	}
	void add_task(mix_task* task)
	{
		/*
		* select a random thread to push task
		* we can also select a thread that has less task to do
		* but that will scan the whole thread list and use thread lock to get each task size
		*/
		uint32_t thread_idx = random() % m_worker_size;
		m_worker_list[thread_idx].push_task(task);
	}
	void destory()
	{
		if (m_worker_list)
			delete[] m_worker_list;
	}
private:
	uint32_t 		m_worker_size;
	worker_thread* 	m_worker_list;
};


#endif /* THREADPOOL_H_ */
