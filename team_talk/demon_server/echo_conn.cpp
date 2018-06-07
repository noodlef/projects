#include"echo_conn.h"
#include "stdafx.h"
#include <string>
#include<list>
#include"Thread.h"
list<std::shared_ptr<im_pdu>> msg_in_list;
thread_notify notify_msg_in;
list<std::shared_ptr<im_pdu>> msg_out_list;
thread_notify notify_msg_out;

void* read_from_tty(void* arg) 
{
	echo_conn* conn = (echo_conn*)arg;
	char buffer[256] = { '\0' };
	while (1) {
		fgets(buffer, sizeof(buffer), stdin);
		std::shared_ptr<im_pdu> pdu = std::make_shared<im_pdu>();
		pdu->set_PB_msg(buffer);
		notify_msg_in.lock();
		msg_in_list.push_back(pdu);
		notify_msg_in.signal();
		notify_msg_in.unlock();
	}
}


void* write_to_tty(void* arg)
{
	echo_conn* conn = (echo_conn*)arg;
	char buffer[256] = { '\0' };
	while (1) {
		// 从 task_list 中取出任务
		notify_msg_out.lock();
		while (msg_out_list.empty())
			notify_msg_out.wait();
		std::shared_ptr<im_pdu> pdu = msg_out_list.front();
		msg_out_list.pop_front();
		notify_msg_out.unlock();
		// do stuff
		memcpy(buffer, pdu->get_bodyData(), pdu->get_bodyLength());
		buffer[pdu->get_bodyLength()] = '\0';
		fputs(buffer, stdout);
	}
}


void* msg_send(void* arg) {
	echo_conn* conn = (echo_conn*)arg;
	while (1) {
		notify_msg_in.lock();
		while(msg_in_list.empty())
			notify_msg_in.wait();
		shared_ptr<im_pdu> pdu = msg_in_list.front();
		msg_in_list.pop_front();
		notify_msg_in.unlock();
		conn->send_pdu(pdu);
	}
}


static conn_map_t g_echo_client_conn_map;
static conn_map_t g_msg_serv_conn_map;
static uint32_t g_total_online_user_cnt = 0;

void echo_conn_timer_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	uint64_t cur_time = get_tick_count();
	for (conn_map_t::iterator it = g_echo_client_conn_map.begin(); it != g_echo_client_conn_map.end(); ) {
		conn_map_t::iterator it_old = it;
		it++;
		echo_conn* conn = (echo_conn*)it_old->second;
		conn->on_timer(cur_time);
	}
}

void init_echo_conn()
{
	netlib_register_timer(echo_conn_timer_callback, NULL, 1000);
}

echo_conn::echo_conn()
{

}

echo_conn::~echo_conn()
{

}

void echo_conn::close()
{
	if (m_handle != NETLIB_INVALID_HANDLE) {
		netlib_close(m_handle);
		if (m_conn_type == LOGIN_CONN_TYPE_CLIENT) {
			g_echo_client_conn_map.erase(m_handle);
		}
	}
	release_ref();
}

void echo_conn::on_connect(net_handle_t handle, int conn_type)
{
	m_handle = handle;
	m_conn_type = conn_type;
	conn_map_t* conn_map = &g_echo_client_conn_map;
	if (conn_type == LOGIN_CONN_TYPE_CLIENT) {
		conn_map->insert(make_pair(handle, this));
	}
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*)im_conn_callback);
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA, (void*)conn_map);
	
	pthread_t m_thread_id;
	pthread_create(&m_thread_id, NULL, read_from_tty, this);
	pthread_create(&m_thread_id, NULL, write_to_tty, this);
	pthread_create(&m_thread_id, NULL, msg_send, this);
}

void echo_conn::on_close()
{
	close();
}


void echo_conn::on_timer(uint64_t curr_tick)
{
#if 0
	if (m_conn_type == LOGIN_CONN_TYPE_CLIENT) {
		if (curr_tick > m_last_recv_tick + CLIENT_TIMEOUT) {
			Close();
		}
	}
	else {
		if (curr_tick > m_last_send_tick + SERVER_HEARTBEAT_INTERVAL) {
			IM::Other::IMHeartBeat msg;
			CImPdu pdu;
			pdu.SetPBMsg(&msg);
			pdu.SetServiceId(SID_OTHER);
			pdu.SetCommandId(CID_OTHER_HEARTBEAT);
			SendPdu(&pdu);
		}

		if (curr_tick > m_last_recv_tick + SERVER_TIMEOUT) {
			log("connection to MsgServer timeout ");
			Close();
		}
	}
#endif   
}


void echo_conn::handle_pdu(std::shared_ptr<im_pdu> pdu)
{
	notify_msg_out.lock();
	msg_out_list.push_back(pdu);
	notify_msg_out.signal();
	notify_msg_out.unlock();
}

void echo_conn::_handle_msg_clientInfo(im_pdu* pdu)
{
	if (pdu) {
		send_pdu(pdu);
	}
}

void echo_conn::_handle_msg_clientInfo(std::shared_ptr<im_pdu> pPdu)
{
	if (pPdu) {
		send_pdu(pPdu);
	}
}

void echo_conn::_handle_msg_login(std::shared_ptr<im_pdu> pdu)
{
	
}
