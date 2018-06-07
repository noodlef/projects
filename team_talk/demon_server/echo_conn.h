#pragma once
/*
* EchoConn.h
*
*  Created on: 2016-4-1
*      Author: tongfan
*/
#ifndef LOGINCONN_H_
#define LOGINCONN_H_
#include <memory>
#include "im_conn.h"

enum {
	LOGIN_CONN_TYPE_CLIENT = 1,
	LOGIN_CONN_TYPE_MSG_SERV
};

class echo_conn : public im_conn
{
public:
	echo_conn();
	virtual ~echo_conn();
	virtual void close();
	void on_connect(net_handle_t handle, int conn_type);
	virtual void on_close();
	virtual void on_timer(uint64_t curr_tick);
	virtual void handle_pdu(std::shared_ptr<im_pdu> pPdu);
private:
	void _handle_msg_clientInfo(im_pdu* pPdu);
	void _handle_msg_clientInfo(std::shared_ptr<im_pdu> pPdu);
	void _handle_userCnt_update(im_pdu* pPdu);
	void _handle_msg_servRequest(im_pdu* pPdu);
	void _handle_msg_login(std::shared_ptr<im_pdu> pPdu);
private:
	int	m_conn_type;
};

void init_echo_conn();

#endif /* LOGINCONN_H_ */