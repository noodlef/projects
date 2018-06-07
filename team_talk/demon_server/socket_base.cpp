#include"socket_base.h"
#include<string.h>


/* all socket_base objetcs are stored in a hashmap */
typedef hash_map<net_handle_t, socket_base*> socket_map;
socket_map	g_socket_map;

/* 添加进socket_base */
void insert_socket_base(socket_base* pSocket)
{
	g_socket_map.insert(make_pair((net_handle_t)pSocket->get_socket(), pSocket));
}

/* 删除socket_base */
void remove_socket_base(socket_base* pSocket)
{
	g_socket_map.erase((net_handle_t)pSocket->get_socket());
}

/* 根据fd从hashmap中找到socket_base */
socket_base* find_socket_base(net_handle_t fd)
{
	socket_base* pSocket = NULL;
	socket_map::iterator iter = g_socket_map.find(fd);
	if (iter != g_socket_map.end())
	{
		pSocket = iter->second;
		pSocket->add_ref();
	}
	return pSocket;
}


/* member functions of socket_base */

socket_base::socket_base()
{
	m_socket = INVALID_SOCKET;
	m_state = SOCKET_STATE_IDLE;
}

socket_base::~socket_base()
{
	/* do nothing */
	mix_log("socket_base::~socket_base, socket=%d\n", m_socket);
}

void socket_base::set_send_bufSize(uint32_t send_size)
{
#if 0
	int ret = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &send_size, 4);
	if (ret == SOCKET_ERROR) {
		log("set SO_SNDBUF failed for fd=%d", m_socket);
	}

	socklen_t len = 4;
	int size = 0;
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &size, &len);
	log("socket=%d send_buf_size=%d", m_socket, size);
#endif
}

void socket_base::set_recv_bufSize(uint32_t recv_size)
{
#if 0
	int ret = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &recv_size, 4);
	if (ret == SOCKET_ERROR) {
		log("set SO_RCVBUF failed for fd=%d", m_socket);
	}

	socklen_t len = 4;
	int size = 0;
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &size, &len);
	log("socket=%d recv_buf_size=%d", m_socket, size);
#endif
}

int socket_base::listen(
	const char*		server_ip,
	uint16_t		port,
	callback_t		callback,
	void*			callback_data)
{
	m_local_ip = server_ip;
	m_local_port = port;
	m_callback = callback;
	m_callback_data = callback_data;
	// create a tcp socketfd
	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		printf("socket failed, err_code=%d\n", _get_error_code());
		return NETLIB_ERROR;
	}
	/* set socketfd noblock, reusable */
	_set_reuseAddr(m_socket);
	_set_nonblock(m_socket);

	sockaddr_in serv_addr;
	_set_addr(server_ip, port, &serv_addr);
	/* bind */
	int ret = ::bind(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == SOCKET_ERROR)
	{
		mix_log("bind failed, err_code=%d", _get_error_code());
		::close(m_socket);
		return NETLIB_ERROR;
	}
	/* listen */
	ret = ::listen(m_socket, 64);
	if (ret == SOCKET_ERROR)
	{
		mix_log("listen failed, err_code=%d", _get_error_code());
		::close(m_socket);
		return NETLIB_ERROR;
	}
	/* set state of the socketfd */
	m_state = SOCKET_STATE_LISTENING;
	mix_log("CBaseSocket::Listen on %s:%d", server_ip, port);
	/* insert the sock_base object into hashmap */
	insert_socket_base(this);
	/* insert the socketfd into event_dispatch */
	event_dispatch::instance()->add_event(m_socket, SOCKET_READ | SOCKET_EXCEP);
	return NETLIB_OK;
}

net_handle_t socket_base::connect(
	const char*		server_ip,
	uint16_t		port,
	callback_t		callback,
	void*			callback_data)
{
	mix_log("base_socket::Connect, server_ip=%s, port=%d", server_ip, port);
	/* set remote IP.port, callback function */
	m_remote_ip = server_ip;
	m_remote_port = port;
	m_callback = callback;
	m_callback_data = callback_data;
	/* creat a new socketfd */
	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		mix_log("socket failed, err_code=%d", _get_error_code());
		return NETLIB_INVALID_HANDLE;
	}
	/* set socketfd noblock, nodelay */
	_set_nonblock(m_socket);
	_set_no_delay(m_socket);
	sockaddr_in serv_addr;
	/* connect */
	_set_addr(server_ip, port, &serv_addr);
	int ret = ::connect(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if ((ret == SOCKET_ERROR) && (!_is_block(_get_error_code())))
	{
		mix_log("connect failed, err_code=%d", _get_error_code());
		::close(m_socket);
		return NETLIB_INVALID_HANDLE;
	}
	m_state = SOCKET_STATE_CONNECTING;
	/*insert the socket_base object into hashmap*/
	insert_socket_base(this);
	event_dispatch::instance()->add_event(m_socket, SOCKET_ALL);           // XXX SOCKET_ALL
	return (net_handle_t)m_socket;
}


int socket_base::send(void* buf, int len)
{
	if (m_state != SOCKET_STATE_CONNECTED)
		return NETLIB_ERROR;

	int ret = ::send(m_socket, (char*)buf, len, 0);
	if (ret == SOCKET_ERROR)
	{
		int err_code = _get_error_code();
		if (_is_block(err_code))
		{
			ret = 0;
			mix_log("socket send block fd=%d", m_socket);
		}
		else
		{
			mix_log("!!!send failed, error code: %d", err_code);
		}
	}
	return ret;
}

int socket_base::recv(void* buf, int len)
{
	return ::recv(m_socket, (char*)buf, len, 0);
}

int socket_base::close()
{
	event_dispatch::instance()->remove_event(m_socket, SOCKET_ALL);
	remove_socket_base(this);
	::close(m_socket);
	release_ref();
	return 0;
}

void socket_base::on_read()
{
	if (m_state == SOCKET_STATE_LISTENING)
	{
		/* server goes here */
		_accept_new_socket();
	}
	else
	{
		/* client goes here */
		u_long avail = 0;
		if ((ioctlsocket(m_socket, FIONREAD, &avail) == SOCKET_ERROR) || (avail == 0))
		{
			/* close socketfd */
			m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t)m_socket, NULL);
		}
		else
		{   /* read */
			m_callback(m_callback_data, NETLIB_MSG_READ, (net_handle_t)m_socket, NULL);
		}
	}
}

void socket_base::on_write()
{
	if (m_state == SOCKET_STATE_CONNECTING)
	{
		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (void*)&error, &len);
		if (error) {
			m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t)m_socket, NULL);
		}
		else {
			/* connction established */
			m_state = SOCKET_STATE_CONNECTED;
			m_callback(m_callback_data, NETLIB_MSG_CONFIRM, (net_handle_t)m_socket, NULL);
		}
	}
	else
	{
		m_callback(m_callback_data, NETLIB_MSG_WRITE, (net_handle_t)m_socket, NULL);
	}
}


void socket_base::on_close()
{
	m_state = SOCKET_STATE_CLOSING;
	m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t)m_socket, NULL);
}


int socket_base::_get_error_code() 
{ 
	return errno; 
}


bool socket_base::_is_block(int error_code)
{
	return ((error_code == EINPROGRESS) || (error_code == EWOULDBLOCK));
}

void socket_base::_set_nonblock(SOCKET fd)
{
	int ret = fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL));
	if (ret == SOCKET_ERROR)
	{
		mix_log("_SetNonblock failed, err_code=%d", _get_error_code());
	}
}

void socket_base::_set_reuseAddr(SOCKET fd)
{
	int reuse = 1;
	int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
	if (ret == SOCKET_ERROR)
	{
		mix_log("set_reuseAddr failed, err_code=%d", _get_error_code());
	}
}

void socket_base::_set_no_delay(SOCKET fd)
{
	int nodelay = 1;
	int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
	if (ret == SOCKET_ERROR)
	{
		mix_log("_set_no_delay failed, err_code=%d", _get_error_code());
	}
}

void socket_base::_set_addr(const char* ip, const uint16_t port, sockaddr_in* pAddr)
{
	memset(pAddr, 0, sizeof(sockaddr_in));
	pAddr->sin_family = AF_INET;
	pAddr->sin_port = htons(port);
	pAddr->sin_addr.s_addr = inet_addr(ip);
	if (pAddr->sin_addr.s_addr == INADDR_NONE)
	{
		hostent* host = gethostbyname(ip);
		if (host == NULL)
		{
			mix_log("gethostbyname failed, ip=%s", ip);
			return;
		}
		pAddr->sin_addr.s_addr = *(uint32_t*)host->h_addr;
	}
}

void socket_base::_accept_new_socket()
{
	socket_t socketfd = 0;
	sockaddr_in peer_addr;
	socklen_t addr_len = sizeof(sockaddr_in);
	char ip_str[64];
	while ((socketfd = accept(m_socket, (sockaddr*)&peer_addr, &addr_len)) != INVALID_SOCKET)
	{
		socket_base* socket = new socket_base();
		uint32_t ip = ntohl(peer_addr.sin_addr.s_addr);
		uint16_t port = ntohs(peer_addr.sin_port);
		snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
			ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
		mix_log("AcceptNewSocket, socket=%d from %s:%d\n", socketfd, ip_str, port);
		/* set IP.port , callback function, socket stat */
		socket->set_socket(socketfd);
		socket->set_callback(m_callback);
		socket->set_callback_data(m_callback_data);
		socket->set_state(SOCKET_STATE_CONNECTED);
		socket->set_remote_IP(ip_str);
		socket->set_remote_port(port);
		/* set socketfd noblock, nodelay*/
		_set_no_delay(socketfd);
		_set_nonblock(socketfd);
		/* insert the new socket_base object into hashmap */
		insert_socket_base(socket);
		/* insert the new socket_base object into event_dispatch */
		event_dispatch::instance()->add_event(socketfd, SOCKET_READ | SOCKET_EXCEP);     // XXX     SOCKET_READ | SOCKET_EXCEP
																						 /* XXX */
		m_callback(m_callback_data, NETLIB_MSG_CONNECT, (net_handle_t)socketfd, NULL);
	}
}
