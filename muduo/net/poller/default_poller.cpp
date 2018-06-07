// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "../poller.h"
#include "poll_poller.h"
#include "epoll_poller.h"

#include <stdlib.h>

using namespace muduo::net;
muduo_poller* muduo_poller::new_default_poller(event_loop* loop)
{
	if (::getenv("MUDUO_USE_POLL"))
	{
		return new poll_poller(loop);
	}
	else
	{
		return new epoll_poller(loop);
	}
}
