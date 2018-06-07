#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_POLLPOLLER_H
#define MUDUO_NET_POLLER_POLLPOLLER_H

#include "../poller.h"

#include <vector>

struct pollfd;

namespace muduo
{
	namespace net
	{
		// IO Multiplexing with poll(2).
		class poll_poller : public muduo_poller
		{
		public:

			poll_poller(event_loop* loop);
			virtual ~poll_poller();

			virtual time_stamp poll(int timeout_ms, channel_list* active_channels);
			virtual void update_channel(channel* chl);
			virtual void remove_channel(channel* chl);

		private:
			void fill_active_channels(int num_events,
				channel_list* active_channels) const;

			typedef std::vector<struct pollfd> pollfd_list_t;
			pollfd_list_t  _pollfds;
		};

	}
}
#endif  // MUDUO_NET_POLLER_POLLPOLLER_H
