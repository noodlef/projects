#pragma once
#ifndef __UTIL_H__
#define __UTIL_H__



#include"ostype.h"
#include"Lock.h"
#define mix_log  printf

using namespace std;

void write_pid();

class ref_object
{
public:
	ref_object()
	{
		m_lock = NULL;
		m_refCount = 1;
	}
	virtual ~ref_object()
	{
		/* do nothing */
	}
	void set_lock(CLock* lock) { m_lock = lock; }
	void add_ref()
	{
		if (m_lock)
		{
			m_lock->lock();
			m_refCount++;
			m_lock->unlock();
		}
		else
		{
			m_refCount++;
		}
	}
	void release_ref()
	{
		if (m_lock)
		{
			m_lock->lock();
			m_refCount--;
			if (m_refCount == 0)
			{
				delete this;
				return;
			}
			m_lock->unlock();
		}
		else
		{
			m_refCount--;
			if (m_refCount == 0)
			{
				delete this;
				return;
			}
		}
	}
private:
	int	    m_refCount;
	CLock*	m_lock;
};





uint64_t get_tick_count();


#endif

