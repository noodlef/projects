#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include "../time_stamp.h"

namespace muduo
{

	// Adapted from google-protobuf stubs/common.h
	// see License in muduo/base/Types.h
	template<typename To, typename From>
	inline ::boost::shared_ptr<To> down_pointer_cast(const ::boost::shared_ptr<From>& f)
	{
		if (false)
		{
			implicit_cast<From*, To*>(0);
		}

#ifndef NDEBUG
		assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
		return ::boost::static_pointer_cast<To>(f);
	}

	namespace net
	{

		// All client visible callbacks go here.

		class muduo_buffer;
		class tcp_connection;
		typedef boost::shared_ptr<tcp_connection> tcp_connection_ptr;
		typedef boost::function<void()> timer_callback;
		typedef boost::function<void(const tcp_connection_ptr&)> connection_callback;
		typedef boost::function<void(const tcp_connection_ptr&)> close_callback;
		typedef boost::function<void(const tcp_connection_ptr&)> write_complete_callback;
		typedef boost::function<void(const tcp_connection_ptr&, size_t)> high_water_mark_callback;

		// the data has been read to (buf, len)
		typedef boost::function<void(const tcp_connection_ptr&,
			muduo_buffer*,
			time_stamp)> message_callback;

		void default_connection_callback(const tcp_connection_ptr& conn);
		void default_message_callback(const tcp_connection_ptr& conn,
			muduo_buffer* buffer,
			time_stamp receive_time);

	}
}

#endif  // MUDUO_NET_CALLBACKS_H
