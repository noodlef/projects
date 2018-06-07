#include"im_pdu_base.h"
#include <memory>
#include"util.h"

im_pdu::im_pdu()
	{
		m_pdu_header.tag = 0;
		m_pdu_header.version = IM_PDU_VERSION;
		m_pdu_header.cmdid = 0;
		m_pdu_header.sessionid = 0;
		m_pdu_header.destid = 0;
		m_pdu_header.reversed = 0;
	}

im_pdu:: ~im_pdu() { }

	uchar_t* im_pdu::get_buffer() {
		return m_buf.get_buffer();
	}

	uint32_t im_pdu::get_length() {
		return m_buf.get_write_offset();
	}

	uchar_t* im_pdu::get_bodyData() {
		return m_buf.get_buffer() + sizeof(pdu_header_t);
	}

	uint32_t im_pdu::get_bodyLength() {
		uint32_t body_length = 0;
		body_length = m_buf.get_write_offset() - sizeof(pdu_header_t);
		return body_length;
	}


	void im_pdu::set_version(uint8_t version) {
		uchar_t* buf = get_buffer();
		byte_stream::write_uint8(buf + OFFSET_PDU_HDR_VERSION, version);
	}
	void im_pdu::set_tag(uint16_t tag) {
		uchar_t* buf = get_buffer();
		byte_stream::write_uint16(buf + OFFSET_PDU_HDR_TAG, tag);
	}

	void im_pdu::set_command_Id(uint16_t command_id) {
		uchar_t* buf = get_buffer();
		byte_stream::write_uint16(buf + OFFSET_PDU_HDR_CMDID, command_id);
	}

	void im_pdu::set_dest_Id(uint32_t dest_id) {
		uchar_t* buf = get_buffer();
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_DESTID, dest_id);
	}

	void im_pdu::set_session_Id(uint32_t session_id) {
		uchar_t* buf = get_buffer();
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_SESSIONID, session_id);
	}
	void im_pdu::set_sock_handle(net_handle_t sock_handle) { m_sock_handle = sock_handle; }

	void im_pdu::set_reversed(uint32_t reversed) {
		uchar_t* buf = get_buffer();
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_RESERVERD, reversed);
	}

	void im_pdu::write_header()
	{
		uchar_t* buf = get_buffer();

		byte_stream::write_uint16(buf + OFFSET_PDU_HDR_TAG, m_pdu_header.tag);
		byte_stream::write_uint8(buf + OFFSET_PDU_HDR_VERSION, m_pdu_header.version);
		byte_stream::write_uint16(buf + OFFSET_PDU_HDR_CMDID, m_pdu_header.cmdid);
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_PKTLENGTH, get_length());
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_SESSIONID, m_pdu_header.sessionid);
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_DESTID, m_pdu_header.destid);
		byte_stream::write_uint32(buf + OFFSET_PDU_HDR_RESERVERD, m_pdu_header.reversed);
	}

	bool im_pdu::is_pdu_available(uchar_t* buf, uint32_t len, uint32_t& pdu_len)
	{
		if (len < IM_PDU_HEADER_LEN)
			return false;

		pdu_len = byte_stream::read_uint32(buf + OFFSET_PDU_HDR_PKTLENGTH);
		if (pdu_len > len)
		{
			mix_log("pdu_len=%d, len=%d\n", pdu_len, len);
			return false;
		}

		if (0 == pdu_len)
		{
			throw pdu_exception(1, "pdu_len is 0");
		}

		return true;
	}

 std::shared_ptr<im_pdu> im_pdu::read_pdu(uchar_t* buf, uint32_t len)
	{
		uint32_t pdu_len = 0;
		if (len < 1) return NULL; //added by jiaoxieen@2015/11/12
		if (!is_pdu_available(buf, len, pdu_len))
			return NULL;

		uint16_t command_id = byte_stream::read_uint16(buf + OFFSET_PDU_HDR_CMDID);
		std::shared_ptr<im_pdu> pdu = std::make_shared<im_pdu>();

		pdu->write(buf, pdu_len);
		pdu->read_pdu_header(buf, IM_PDU_HEADER_LEN);

		return pdu;
	}

	void im_pdu::write(uchar_t* buf, uint32_t len) 
	{ 
		m_buf.write((void*)buf, len); 
	}

	int im_pdu::read_pdu_header(uchar_t* buf, uint32_t len)
	{
		int ret = -1;
		if (len >= IM_PDU_HEADER_LEN && buf) {
			byte_stream::write_uint16(buf + OFFSET_PDU_HDR_TAG, m_pdu_header.tag);
			byte_stream::write_uint8(buf + OFFSET_PDU_HDR_VERSION, m_pdu_header.version);
			byte_stream::write_uint16(buf + OFFSET_PDU_HDR_CMDID, m_pdu_header.cmdid);
			byte_stream::write_uint32(buf + OFFSET_PDU_HDR_PKTLENGTH, get_length());
			byte_stream::write_uint32(buf + OFFSET_PDU_HDR_SESSIONID, m_pdu_header.sessionid);
			byte_stream::write_uint32(buf + OFFSET_PDU_HDR_DESTID, m_pdu_header.destid);
			byte_stream::write_uint32(buf + OFFSET_PDU_HDR_RESERVERD, m_pdu_header.reversed);
			ret = 0;
		}
		return ret;
	}

	void im_pdu::set_PB_msg(const char* msg)
	{
		//设置包体，则需要重置下空间
		m_buf.read(NULL, m_buf.get_write_offset());
		m_buf.write(NULL, sizeof(pdu_header_t));
		uint32_t msg_size = strlen(msg);
		m_buf.write((void*)msg, msg_size);
		write_header();
	}