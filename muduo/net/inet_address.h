#pragma once
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include "../copyable.h"
#include "../string_piece.h"

#include <netinet/in.h>

namespace muduo
{
	namespace net
	{
		namespace sockets
		{
			const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
		}

	
		// Wrapper of sockaddr_in.
		// This is an POD interface class.
		class inet_address : public muduo::copyable
		{
		public:
			// Constructs an endpoint with given port number.
			// Mostly used in TcpServer listening.
			explicit inet_address(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

			// Constructs an endpoint with given ip and port.
			// @c ip should be "1.2.3.4"
			inet_address(string_arg ip, uint16_t port, bool ipv6 = false);

			// Constructs an endpoint with given struct @c sockaddr_in
			// Mostly used when accepting new connections
			explicit inet_address(const struct sockaddr_in& addr)
				: _addr(addr)
			{ }

			explicit inet_address(const struct sockaddr_in6& addr)
				: _addr6(addr)
			{ }

			sa_family_t family() const { return _addr.sin_family; }

			string to_ip() const;
			string to_ip_port() const;
			uint16_t to_port() const;

			const struct sockaddr* get_sock_addr() const { return sockets::sockaddr_cast(&_addr6); }
			void set_sock_addrInet6(const struct sockaddr_in6& addr6) { _addr6 = addr6; }

			uint32_t ip_net_endian() const;
			uint16_t port_net_endian() const { return _addr.sin_port; }

			// resolve hostname to IP address, not changing port or sin_family
			// return true on success.
			// thread safe
			static bool resolve(string_arg hostname, inet_address* result);
			// static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

		private:
			union
			{
				struct sockaddr_in    _addr;
				struct sockaddr_in6   _addr6;
			};
		};

	}
}

#endif  // MUDUO_NET_INETADDRESS_H
