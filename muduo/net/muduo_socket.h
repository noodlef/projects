#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include <boost/noncopyable.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo
{
	// TCP networking.
	namespace net
	{

		class inet_address;

		
		// Wrapper of socket file descriptor.
		// It closes the sockfd when desctructs.
		// It's thread safe, all operations are delegated to OS.
		class muduo_socket : boost::noncopyable
		{
		public:
			explicit muduo_socket(int sockfd)
				: _sockfd(sockfd)
			{ }

			// Socket(Socket&&) // move constructor in C++11
			~muduo_socket();

			int fd() const { return _sockfd; }

			// return true if success.
			bool get_tcp_info(struct tcp_info*) const;

			bool get_tcp_infoString(char* buf, int len) const;

			// abort if address in use
			void bind_address(const inet_address& localaddr);

			// abort if address in use
			void listen();

			// On success, returns a non-negative integer that is
			// a descriptor for the accepted socket, which has been
			// set to non-blocking and close-on-exec. *peeraddr is assigned.
			// On error, -1 is returned, and *peeraddr is untouched.
			int accept(inet_address* peeraddr);

			void shutdown_write();

			// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
			void set_tcp_noDelay(bool on);

			// Enable/disable SO_REUSEADDR
			void set_reuse_addr(bool on);

			// Enable/disable SO_REUSEPORT
			void set_reuse_port(bool on);

			// Enable/disable SO_KEEPALIVE
			void set_keep_alive(bool on);

		private:
			const int _sockfd;
		};

	}
}
#endif  // MUDUO_NET_SOCKET_H
