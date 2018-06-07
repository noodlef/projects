// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo_socket.h"

#include "../logger.h"
#include "inet_address.h"
#include "sockets_ops.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero
#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

muduo_socket::~muduo_socket()
{
	sockets::close(_sockfd);
}

bool muduo_socket::get_tcp_info(struct tcp_info* tcpi) const
{
	socklen_t len = sizeof(*tcpi);
	bzero(tcpi, len);
	return ::getsockopt(_sockfd, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool muduo_socket::get_tcp_infoString(char* buf, int len) const
{
	struct tcp_info tcpi;
	bool ok = get_tcp_info(&tcpi);
	if (ok)
	{
		snprintf(buf, len, "unrecovered=%u "
			"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
			"lost=%u retrans=%u rtt=%u rttvar=%u "
			"sshthresh=%u cwnd=%u total_retrans=%u",
			tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
			tcpi.tcpi_rto,          // Retransmit timeout in usec
			tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
			tcpi.tcpi_snd_mss,
			tcpi.tcpi_rcv_mss,
			tcpi.tcpi_lost,         // Lost packets
			tcpi.tcpi_retrans,      // Retransmitted packets out
			tcpi.tcpi_rtt,          // Smoothed round trip time in usec
			tcpi.tcpi_rttvar,       // Medium deviation
			tcpi.tcpi_snd_ssthresh,
			tcpi.tcpi_snd_cwnd,
			tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
	}
	return ok;
}

void muduo_socket::bind_address(const inet_address& addr)
{
	sockets::bind_or_die(_sockfd, addr.get_sock_addr());
}

void muduo_socket::listen()
{
	sockets::listen_or_die(_sockfd);
}

int muduo_socket::accept(inet_address* peeraddr)
{
	struct sockaddr_in6 addr;
	bzero(&addr, sizeof addr);
	int connfd = sockets::accept(_sockfd, &addr);
	if (connfd >= 0)
	{
		peeraddr->set_sock_addrInet6(addr);
	}
	return connfd;
}

void muduo_socket::shutdown_write()
{
	sockets::shutdown_write(_sockfd);
}

void muduo_socket::set_tcp_noDelay(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY,
		&optval, static_cast<socklen_t>(sizeof optval));
	// FIXME CHECK
}

void muduo_socket::set_reuse_addr(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
		&optval, static_cast<socklen_t>(sizeof optval));
	// FIXME CHECK
}

void muduo_socket::set_reuse_port(bool on)
{
#ifdef SO_REUSEPORT
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
		&optval, static_cast<socklen_t>(sizeof optval));
	if (ret < 0 && on)
	{
		LOG_SYSERR << "SO_REUSEPORT failed.";
	}
#else
	if (on)
	{
		LOG_ERROR << "SO_REUSEPORT is not supported.";
	}
#endif
}

void muduo_socket::set_keep_alive(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE,
		&optval, static_cast<socklen_t>(sizeof optval));
	// FIXME CHECK
}
