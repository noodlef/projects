#include"im_conn.h"

static im_conn* find_im_conn(conn_map_t* im_conn_map, net_handle_t handle)
{
	im_conn* conn = NULL;
	conn_map_t::iterator iter = im_conn_map->find(handle);
	if (iter != im_conn_map->end())
	{
		conn = iter->second;
		conn->add_ref();
	}

	return conn;
}

void im_conn_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	//NOTUSED_ARG(handle);
	//NOTUSED_ARG(pParam);

	if (!callback_data)
		return;
	conn_map_t* conn_map = (conn_map_t*)callback_data;
	im_conn* conn = find_im_conn(conn_map, handle);
	if (!conn)
		return;
	//log("msg=%d, handle=%d ", msg, handle);
	switch (msg)
	{
	case NETLIB_MSG_CONFIRM:
		conn->on_confirm();
		break;
	case NETLIB_MSG_READ:
		conn->on_read();
		break;
	case NETLIB_MSG_WRITE:
		conn->on_write();
		break;
	case NETLIB_MSG_CLOSE:
		conn->on_close();
		break;
	default:
		mix_log("!!!imconn_callback error msg: %d ", msg);
		break;
	}
	conn->release_ref();
}




im_conn::im_conn()
	{
		m_busy = false;
		m_handle = NETLIB_INVALID_HANDLE;
		m_recv_bytes = 0;
		m_last_send_tick = m_last_recv_tick = get_tick_count();
	}

im_conn:: ~im_conn()
	{
		/* do nothing */
		mix_log("im_conn::~im_conn, handle=%d ", m_handle);
	}

	
	int im_conn::send_pdu(im_pdu* pdu)
	{
		return send(pdu->get_buffer(), pdu->get_length());
	}

	int im_conn::send_pdu(std::shared_ptr<im_pdu> pdu)
	{
		return this->send(pdu->get_buffer(), pdu->get_length());
	}

	int im_conn::send(void* data, int len)
	{
		m_last_send_tick = get_tick_count();
		//	++g_send_pkt_cnt;

		if (m_busy)
		{
			m_out_buf.write(data, len);
			return len;
		}

		int offset = 0;
		int remain = len;
		while (remain > 0) {
			int send_size = remain;
			if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) {
				send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
			}

			int ret = netlib_send(m_handle, (char*)data + offset, send_size);
			if (ret <= 0) {
				ret = 0;
				break;
			}

			offset += ret;
			remain -= ret;
		}

		if (remain > 0)
		{
			m_out_buf.write((char*)data + offset, remain);
			m_busy = true;
			mix_log("send busy, remain=%d ", m_out_buf.get_write_offset());
		}
		else
		{
			on_write_compelete();
		}

		return len;
	}
	
	
	void im_conn::on_read()
	{
		for (;;)
		{
			uint32_t free_buf_len = m_in_buf.get_alloc_size() - m_in_buf.get_write_offset();
			if (free_buf_len < READ_BUF_SIZE)
				m_in_buf.extend(READ_BUF_SIZE);
			int ret = netlib_recv(m_handle, m_in_buf.get_buffer() + m_in_buf.get_write_offset(), READ_BUF_SIZE);
			if (ret <= 0)
				break;
			m_recv_bytes += ret;
			m_in_buf.inc_write_offset(ret);
			m_last_recv_tick = get_tick_count();
		}

		std::shared_ptr<im_pdu> pdu = NULL;
		try
		{
			while ((pdu = im_pdu::read_pdu(m_in_buf.get_buffer(), m_in_buf.get_write_offset())))
			{
				uint32_t pdu_len = pdu->get_length();
				pdu->set_sock_handle(m_handle);

				handle_pdu(pdu);

				m_in_buf.read(NULL, pdu_len);
				pdu.reset();
			}
		}
		catch (pdu_exception& ex) {
			mix_log("!!!catch exception, sid=%u, cid=%u, err_code=%u, err_msg=%s, close the connection ",
				ex.get_service_Id(), ex.get_command_Id(), ex.get_error_code(), ex.get_error_msg());
			if (pdu) {
				pdu.reset();
			}
			on_close();
		}
	}

    void im_conn::on_write()
	{
		/* send_buffer is empty*/
		if (!m_busy) return;

		while (m_out_buf.get_write_offset() > 0) {
			int send_size = m_out_buf.get_write_offset();
			if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) {
				send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
			}
			int ret = netlib_send(m_handle, m_out_buf.get_buffer(), send_size);
			if (ret <= 0) {
				ret = 0;
				break;
			}

			m_out_buf.read(NULL, ret);
		}
		if (m_out_buf.get_write_offset() == 0) {
			m_busy = false;
		}
		mix_log("onWrite, remain=%d ", m_out_buf.get_write_offset());
	}
