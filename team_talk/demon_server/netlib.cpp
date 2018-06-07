#include "stdafx.h"
#include "netlib.h"
#include "socket_base.h"
#include "event_dispatch.h"

int netlib_init()
{
	int ret = NETLIB_OK;
#ifdef _WIN32
	WSADATA wsaData;
	WORD wReqest = MAKEWORD(1, 1);
	if (WSAStartup(wReqest, &wsaData) != 0)
	{
		ret = NETLIB_ERROR;
	}
#endif

	return ret;
}

int netlib_destroy()
{
	int ret = NETLIB_OK;
#ifdef _WIN32
	if (WSACleanup() != 0)
	{
		ret = NETLIB_ERROR;
	}
#endif

	return ret;
}

// 创建服务器监听socket
int netlib_listen(
	const char*	server_ip,
	uint16_t	port,
	callback_t	callback,
	void*		callback_data)
{
	socket_base* pSocket = new socket_base();
	if (!pSocket)
		return NETLIB_ERROR;

	int ret = pSocket->listen(server_ip, port, callback, callback_data);
	if (ret == NETLIB_ERROR)
		delete pSocket;
	return ret;
}

net_handle_t netlib_connect(
	const char* server_ip,
	uint16_t	port,
	callback_t	callback,
	void*		callback_data)
{
	socket_base* pSocket = new socket_base();
	if (!pSocket)
		return NETLIB_INVALID_HANDLE;

	net_handle_t handle = pSocket->connect(server_ip, port, callback, callback_data);
	if (handle == NETLIB_INVALID_HANDLE)
		delete pSocket;
	return handle;
}

int netlib_send(net_handle_t handle, void* buf, int len)
{
	socket_base* pSocket = find_socket_base(handle);
	if (!pSocket)
	{
		return NETLIB_ERROR;
	}
	int ret = pSocket->send(buf, len);
	pSocket->release_ref();
	return ret;
}

int netlib_recv(net_handle_t handle, void* buf, int len)
{
	socket_base* pSocket = find_socket_base(handle);
	if (!pSocket)
		return NETLIB_ERROR;

	int ret = pSocket->recv(buf, len);
	pSocket->release_ref();
	return ret;
}

int netlib_close(net_handle_t handle)
{
	socket_base* pSocket = find_socket_base(handle);
	if (!pSocket)
		return NETLIB_ERROR;

	int ret = pSocket->close();
	pSocket->release_ref();
	return ret;
}

int netlib_option(net_handle_t handle, int opt, void* optval)
{
	socket_base* socket = find_socket_base(handle);
	if (!socket)
		return NETLIB_ERROR;

	if ((opt >= NETLIB_OPT_GET_REMOTE_IP) && !optval)
		return NETLIB_ERROR;

	switch (opt)
	{
	case NETLIB_OPT_SET_CALLBACK:
		socket->set_callback((callback_t)optval);
		break;
	case NETLIB_OPT_SET_CALLBACK_DATA:
		socket->set_callback_data(optval);
		break;
	case NETLIB_OPT_GET_REMOTE_IP:
		*(string*)optval = socket->get_remote_IP();
		break;
	case NETLIB_OPT_GET_REMOTE_PORT:
		*(uint16_t*)optval = socket->get_remote_port();
		break;
	case NETLIB_OPT_GET_LOCAL_IP:
		*(string*)optval = socket->get_local_IP();
		break;
	case NETLIB_OPT_GET_LOCAL_PORT:
		*(uint16_t*)optval = socket->get_local_port();
		break;
	case NETLIB_OPT_SET_SEND_BUF_SIZE:
		socket->set_send_bufSize(*(uint32_t*)optval);
		break;
	case NETLIB_OPT_SET_RECV_BUF_SIZE:
		socket->set_recv_bufSize(*(uint32_t*)optval);
		break;
	}

	socket->release_ref();
	return NETLIB_OK;
}

int netlib_register_timer(callback_t callback, void* user_data, uint64_t interval)
{
	event_dispatch::instance()->add_timer(callback, user_data, interval);
	return 0;
}

int netlib_delete_timer(callback_t callback, void* user_data)
{
	event_dispatch::instance()->remove_timer(callback, user_data);
	return 0;
}

int netlib_add_loop(callback_t callback, void* user_data)
{
	event_dispatch::instance()->add_loop(callback, user_data);
	return 0;
}

void netlib_eventloop(uint32_t wait_timeout)
{
	event_dispatch::instance()->start_dispatch(wait_timeout);
}

void netlib_stop_event()
{
	event_dispatch::instance()->stop_dispatch();
}

bool netlib_is_running()
{
	return event_dispatch::instance()->is_running();
}
