#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include "../poller.h"

#include <vector>

struct epoll_event;

namespace muduo
{
	namespace net
	{

		
		// IO Multiplexing with epoll(4).
		class epoll_poller : public muduo_poller
		{
		public:
			epoll_poller(event_loop* loop);
			virtual ~epoll_poller();

			virtual time_stamp poll(int timeout_ms, channel_list* active_channels);
			virtual void update_channel(channel* chl);
			virtual void remove_channel(channel* chl);

		private:
			static const int kinit_event_list_size = 16;

			static const char* operation_to_string(int op);

			void fill_active_channels(int num_events,
				channel_list* active_channels) const;
			void update(int operation, channel* channel);

			typedef std::vector<struct epoll_event> event_list_t;

			int               _epollfd;
			event_list_t      _events;
		};

	}
}
#endif  // MUDUO_NET_POLLER_EPOLLPOLLER_H
