#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <map>
#include <vector>
#include <boost/noncopyable.hpp>

#include "../time_stamp.h"
#include "event_loop.h"

namespace muduo
{
	namespace net
	{

		class channel;

	
		// Base class for IO Multiplexing
		// This class doesn't own the Channel objects.
		class muduo_poller //: boost::noncopyable
		{
		public:
			typedef std::vector<channel*> channel_list;

			muduo_poller(event_loop* loop)
				:_owner_loop(loop)
			{ }

			virtual ~muduo_poller()
			{ }

			// Polls the I/O events.
			// Must be called in the loop thread.
			virtual time_stamp poll(int timeout_ms, channel_list* active_channels) = 0;

			// Changes the interested I/O events.
			// Must be called in the loop thread.
			virtual void update_channel(channel* channel) = 0;

			// Remove the channel, when it destructs.
			// Must be called in the loop thread.
			virtual void remove_channel(channel* channel) = 0;

			virtual bool has_channel(channel* chl) const
			{
				assert_in_loop_thread();
				channel_map_t::const_iterator it = _channels.find(chl->fd());
				return it != _channels.end() && it->second == chl;
			}

			static muduo_poller* new_default_poller(event_loop* loop);

			void assert_in_loop_thread() const
			{
				_owner_loop->assert_in_loop_thread();
			}

		protected:
			typedef std::map<int, channel*> channel_map_t;
			channel_map_t _channels;

		private:
			event_loop*   _owner_loop;
		};

	}
}
#endif  // MUDUO_NET_POLLER_H
