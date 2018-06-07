#include"event_dispatch.h"
#include"Lock.h"
#include"socket_base.h"
#include<list>

event_dispatch* event_dispatch::m_pEventDispatch = NULL;

event_dispatch::~event_dispatch() { ::close(m_epfd); };

/* add event to the event dispatcher */
void event_dispatch::add_event(socket_t fd, uint8_t socket_event)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
	ev.data.fd = fd;
	if (::epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) != 0)
	{
		mix_log("epoll_ctl() failed, errno=%d", errno);
	}
}

/* remove event from the event dispatcher */
void event_dispatch::remove_event(SOCKET fd, uint8_t socket_event)
{
	if (::epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL) != 0)
	{
		mix_log("epoll_ctl failed, errno=%d", errno);
	}
}

void event_dispatch::add_timer(callback_t callback, void* user_data, uint64_t interval)
{
	list<TimerItem*>::iterator it;
	for (it = m_timer_list.begin(); it != m_timer_list.end(); it++)
	{
		TimerItem* pItem = *it;
		if (pItem->callback == callback && pItem->user_data == user_data)
		{
			pItem->interval = interval;
			pItem->next_tick = get_tick_count() + interval;
			return;
		}
	}

	TimerItem* pItem = new TimerItem;
	pItem->callback = callback;
	pItem->user_data = user_data;
	pItem->interval = interval;
	pItem->next_tick = get_tick_count() + interval;
	m_timer_list.push_back(pItem);
}

void event_dispatch::remove_timer(callback_t callback, void* user_data)
{
	list<TimerItem*>::iterator it;
	for (it = m_timer_list.begin(); it != m_timer_list.end(); it++)
	{
		TimerItem* pItem = *it;
		if (pItem->callback == callback && pItem->user_data == user_data)
		{
			m_timer_list.erase(it);
			delete pItem;
			return;
		}
	}
}

void event_dispatch::add_loop(callback_t callback, void* user_data)
{
	TimerItem* pItem = new TimerItem;
	pItem->callback = callback;
	pItem->user_data = user_data;
	m_loop_list.push_back(pItem);
}

/* dispatch */
void event_dispatch::start_dispatch(uint32_t wait_timeout)
{
	struct epoll_event *events = NULL;
	int nfds = 0;
	events = new struct epoll_event[MAX_EVENT_NUMS];
	if (!events) return;
	if (running) return;
	running = true;
	/* */
	while (running)
	{
		nfds = ::epoll_wait(m_epfd, events, MAX_EVENT_NUMS, wait_timeout);
		for (int i = 0; i < nfds; i++)
		{
			int ev_fd = events[i].data.fd;
			socket_base* pSocket = find_socket_base(ev_fd);
			if (!pSocket)
				continue;

			//Commit by zhfu @2015-02-28
			if (events[i].events & EPOLLRDHUP)
			{
				//log("On Peer Close, socket=%d, ev_fd);
				pSocket->on_close();
			}
			// Commit End

			if (events[i].events & EPOLLIN)
			{
				//log("OnRead, socket=%d\n", ev_fd);
				pSocket->on_read();
			}

			if (events[i].events & EPOLLOUT)
			{
				//log("OnWrite, socket=%d\n", ev_fd);
				pSocket->on_write();
			}

			if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
			{
				//log("OnClose, socket=%d\n", ev_fd);
				pSocket->on_close();
			}

			pSocket->release_ref();
		}

		_check_timer();
		_check_loop();
	}
	delete events;
}

void event_dispatch::stop_dispatch()
{
	running = false;
}

bool event_dispatch::is_running() { return running; }

event_dispatch* event_dispatch::instance()
{
	if (m_pEventDispatch == NULL)
	{
		m_pEventDispatch = new event_dispatch();
	}
	return m_pEventDispatch;
}

event_dispatch::event_dispatch()
	{
		m_epfd = ::epoll_create(MAX_EVENT_NUMS);
		if (m_epfd == -1)
		{
			mix_log("epoll_create failed");
		}
	}

	void event_dispatch::_check_timer()
	{
		uint64_t curr_tick = get_tick_count();
		list<TimerItem*>::iterator it;

		for (it = m_timer_list.begin(); it != m_timer_list.end(); )
		{
			TimerItem* pItem = *it;
			it++;		// iterator maybe deleted in the callback, so we should increment it before callback
			if (curr_tick >= pItem->next_tick)
			{
				pItem->next_tick += pItem->interval;
				pItem->callback(pItem->user_data, NETLIB_MSG_TIMER, 0, NULL);
			}
		}
	}

	void event_dispatch::_check_loop()
	{
		for (list<TimerItem*>::iterator it = m_loop_list.begin(); it != m_loop_list.end(); it++) {
			TimerItem* pItem = *it;
			pItem->callback(pItem->user_data, NETLIB_MSG_LOOP, 0, NULL);
		}
	}
