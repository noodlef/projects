#pragma once
/*
* A socket event dispatcher, features include:
* 1. portable: worked both on Windows, MAC OS X,  LINUX platform
* 2. a singleton pattern: only one instance of this class can exist
*/
#ifndef __EVENT_DISPATCH_H__
#define __EVENT_DISPATCH_H__

#include"ostype.h"
#include"util.h"
#include<list>

#define MIN_TIMER_DURATION	100	// 100 miliseconds
#define MAX_EVENT_NUMS 1024*10*10*5



enum {
	SOCKET_READ = 0x1,
	SOCKET_WRITE = 0x2,
	SOCKET_EXCEP = 0x4,
	SOCKET_ALL = 0x7
};

class event_dispatch
{
public:
	virtual ~event_dispatch();

	/* add event to the event dispatcher */
	void add_event(socket_t fd, uint8_t socket_event);
	

	/* remove event from the event dispatcher */
	void remove_event(SOCKET fd, uint8_t socket_event);
	

	void add_timer(callback_t callback, void* user_data, uint64_t interval);
	
	void remove_timer(callback_t callback, void* user_data);
	
	void add_loop(callback_t callback, void* user_data);
	

	/* dispatch */
	void start_dispatch(uint32_t wait_timeout = 100);
	

	void stop_dispatch();


	bool is_running();

	static event_dispatch* instance();
	
protected:
	event_dispatch();
	
private:
	void _check_timer();
	
	void _check_loop();
	
	typedef struct {
		callback_t	callback;
		void*		user_data;
		uint64_t	interval;
		uint64_t	next_tick;
	} TimerItem;

private:

	int		m_epfd;
	CLock			m_lock;
	list<TimerItem*>	m_timer_list;
	list<TimerItem*>	m_loop_list;

	static event_dispatch* m_pEventDispatch;
	/* stat */
	bool running;
};

#endif
