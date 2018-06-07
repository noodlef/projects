// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "inet_address.h"

#include "../logger.h"
#include "endian.h"
#include "sockets_ops.h"

#include <netdb.h>
#include <strings.h>  // bzero
#include <netinet/in.h>

#include <boost/static_assert.hpp>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace muduo;
using namespace muduo::net;

BOOST_STATIC_ASSERT(sizeof(inet_address) == sizeof(struct sockaddr_in6));
BOOST_STATIC_ASSERT(offsetof(sockaddr_in, sin_family) == 0);
BOOST_STATIC_ASSERT(offsetof(sockaddr_in6, sin6_family) == 0);
BOOST_STATIC_ASSERT(offsetof(sockaddr_in, sin_port) == 2);
BOOST_STATIC_ASSERT(offsetof(sockaddr_in6, sin6_port) == 2);

#if !(__GNUC_PREREQ (4,6))
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
inet_address::inet_address(uint16_t port, bool loopbackOnly, bool ipv6)
{
	BOOST_STATIC_ASSERT(offsetof(inet_address, _addr6) == 0);
	BOOST_STATIC_ASSERT(offsetof(inet_address, _addr) == 0);
	if (ipv6)
	{
		bzero(&_addr6, sizeof _addr6);
		_addr6.sin6_family = AF_INET6;
		in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
		_addr6.sin6_addr = ip;
		_addr6.sin6_port = sockets::host_to_network16(port);
	}
	else
	{
		bzero(&_addr, sizeof _addr);
		_addr.sin_family = AF_INET;
		in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
		_addr.sin_addr.s_addr = sockets::host_to_network32(ip);
		_addr.sin_port = sockets::host_to_network16(port);
	}
}

inet_address::inet_address(string_arg ip, uint16_t port, bool ipv6)
{
	if (ipv6)
	{
		bzero(&_addr6, sizeof _addr6);
		sockets::from_ip_port(ip.c_str(), port, &_addr6);
	}
	else
	{
		bzero(&_addr, sizeof _addr);
		sockets::from_ip_port(ip.c_str(), port, &_addr);
	}
}

string inet_address::to_ip_port() const
{
	char buf[64] = "";
	sockets::to_ip_port(buf, sizeof buf, get_sock_addr());
	return buf;
}

string inet_address::to_ip() const
{
	char buf[64] = "";
	sockets::to_ip(buf, sizeof buf, get_sock_addr());
	return buf;
}

uint32_t inet_address::ip_net_endian() const
{
	assert(family() == AF_INET);
	return _addr.sin_addr.s_addr;
}

uint16_t inet_address::to_port() const
{
	return sockets::network_to_host16(port_net_endian());
}

static __thread char t_resolve_buffer[64 * 1024];

bool inet_address::resolve(string_arg hostname, inet_address* out)
{
	assert(out != NULL);
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	bzero(&hent, sizeof(hent));

	int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolve_buffer, sizeof t_resolve_buffer, &he, &herrno);
	if (ret == 0 && he != NULL)
	{
		assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		out->_addr.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	else
	{
		if (ret)
		{
			LOG_SYSERR << "InetAddress::resolve";
		}
		return false;
	}
}
