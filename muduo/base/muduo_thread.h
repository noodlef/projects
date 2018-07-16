#pragma once
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include"muduo_latch.h"
#include"types.h" // string
#include"muduo_latch.h"
#include"muduo_atomic.h"

#include<boost/noncopyable.hpp>
#include<boost/function.hpp>

#include<pthread.h>

namespace muduo
{
    /* 
     * �߳� mudou_thread - ����һ���߳�
     * ����ʱ��Ҫ����һ���޲��޷���ָ�Ŀɵ��ö���
     * ��һ����ѡ���߳����������ָ��������Ĭ�ϵ�
     * ��Ĭ�ϵ��߳���Ϊ��Thread%d
     */
    class muduo_thread : boost::noncopyable
	{
	public:
		typedef boost::function<void()> thread_func;

		explicit muduo_thread(const thread_func&, const string& name = string());

        /* call pthread_datch() if the thread is not joined */
		~muduo_thread();

        /* start the thread */
		void start();

        /* join the thread */
		int join(); 

		bool started() const { return _started; }

		pthread_t pthread_id() const { return _pthread_id; }
        
		pid_t tid() const { return _tid; }

        /* return thread name */
		const string& name() const { return _name; }

		static int num() { return _num.get(); }

	private:
		void _set_default_name();
		
		bool    _started;
		bool    _joined;
		thread_func     _func;
		count_down_latch    _latch; /* �ݼ�������������ͬ�� */
		pthread_t   _pthread_id;
		string  _name; /* thread name */
		pid_t   _tid;

		static atomic_int32  _num; /* �����߳�Ĭ������ʱ�ı�� */
    }; // end of mudou_thread
} // end of muduo
#endif
