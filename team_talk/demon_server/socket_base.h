#pragma once
/*
*  a wrap for non-block socket class for Windows, LINUX and MacOS X platform
*/

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include"ostype.h"
#include"util.h"
#include"event_dispatch.h"

// SOCKET µÄ×´Ì¬
enum
{
	SOCKET_STATE_IDLE,
	SOCKET_STATE_LISTENING,
	SOCKET_STATE_CONNECTING,
	SOCKET_STATE_CONNECTED,
	SOCKET_STATE_CLOSING
};

class socket_base : public ref_object
{
public:
	socket_base();
	virtual ~socket_base();

	/* functions that set data members */
	void set_socket(socket_t fd) { m_socket = fd; }
	void set_state(uint8_t state) { m_state = state; }
	void set_callback(callback_t callback) { m_callback = callback; }
	void set_callback_data(void* data) { m_callback_data = data; }
	void set_remote_IP(char* ip) { m_remote_ip = ip; }
	void set_remote_port(uint16_t port) { m_remote_port = port; }
	void set_send_bufSize(uint32_t send_size);
	void set_recv_bufSize(uint32_t recv_size);
	
	/* functions that return data members */
	socket_t get_socket() { return m_socket; }
	const char*	get_remote_IP() { return m_remote_ip.c_str(); }
	uint16_t	get_remote_port() { return m_remote_port; }
	const char*	get_local_IP() { return m_local_ip.c_str(); }
	uint16_t	get_local_port() { return m_local_port; }

public:
	/* server calls here */
	int listen(
		const char*		server_ip,
		uint16_t		port,
		callback_t		callback,
		void*			callback_data);
	
	/* client calls here */
	net_handle_t connect(
		const char*		server_ip,
		uint16_t		port,
		callback_t		callback,
		void*			callback_data);

	/* calls API send() */
	int send(void* buf, int len);
	
	/* calls API recv() */
	int recv(void* buf, int len);
	
	int close();
	
public:
	void on_read();
	
	void on_write();
	
	void on_close();
	
private:
	int _get_error_code();

	bool _is_block(int error_code);
	/* set socketfd nonblock */
	void _set_nonblock(SOCKET fd);
	/* set IP.port of socketfd( XXX time_wait) reusable */
	void _set_reuseAddr(SOCKET fd);
	/* shutdown nagle algorithm */
	void _set_no_delay(SOCKET fd);
		/* set sockaddr_in */
	void _set_addr(const char* ip, const uint16_t port, sockaddr_in* pAddr);
	/* 
	 * creat a new socktfd when server accepts a new connection 
	 */
	void _accept_new_socket();
	
private:
	/* IP , port */
	string			m_remote_ip;
	uint16_t		m_remote_port;
	string			m_local_ip;
	uint16_t		m_local_port;
	/* callback function */
	callback_t		m_callback;
	void*			m_callback_data;
	/* 
	 * socketfd, stat of that socketfd
	 */
	uint8_t			m_state;
	socket_t		m_socket;
};


void insert_socket_base(socket_base* pSocket);
void remove_socket_base(socket_base* pSocket);
socket_base* find_socket_base(net_handle_t fd);
#endif
