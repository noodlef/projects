#pragma once
/*
* imconn.h
*
*  Created on: 2013-6-5
*      Author: ziteng
*/

#ifndef IMCONN_H_
#define IMCONN_H_

#include "netlib.h"
#include "util.h"
#include "im_pdu_base.h"
#include<memory>

#define SERVER_HEARTBEAT_INTERVAL	5000
#define SERVER_TIMEOUT				30000
#define CLIENT_HEARTBEAT_INTERVAL	30000
#define CLIENT_TIMEOUT				120000
#define MOBILE_CLIENT_TIMEOUT       60000 * 5
#define READ_BUF_SIZE	2048

class im_conn : public ref_object
{
public:
	im_conn();
	virtual ~im_conn();
	bool is_busy() { return m_busy; }
	int send_pdu(im_pdu* pdu);
	int send_pdu(std::shared_ptr<im_pdu> pdu);
	int send(void* data, int len);
	
	uint16_t get_peer_port() { return m_peer_port; }
	string get_peer_ip() { return m_peer_ip; }

	virtual void on_connect(net_handle_t handle) { m_handle = handle; }
	virtual void on_confirm() { }
	virtual void on_read();
	virtual void on_write();
	virtual void on_close() { }

	virtual void on_timer(uint64_t curr_tick) { }
	virtual void on_write_compelete() { };

	virtual void handle_pdu(im_pdu* pdu) { }
	virtual void handle_pdu(std::shared_ptr<im_pdu> pdu) { }

protected:
	/* socketfd */
	net_handle_t	m_handle;
	/* whether the send_buffer is free */
	bool			m_busy;
	/* peer IP.port */
	string			m_peer_ip;
	uint16_t		m_peer_port;
	/* recv_buffer, send_buffer */
	simple_buffer	m_in_buf;
	simple_buffer	m_out_buf;
	/* statistics data members */
	bool			m_policy_conn;
	uint32_t		m_recv_bytes;
	uint64_t		m_last_send_tick;
	uint64_t		m_last_recv_tick;
	uint64_t        m_last_all_user_tick;
};

void im_conn_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam);
void read_policy_file();
typedef hash_map<net_handle_t, im_conn*> conn_map_t;
typedef hash_map<uint32_t, im_conn*> user_map_t;
#endif /* IMCONN_H_ */
