#pragma once
/*
* ImPduBase.h
*
*  Created on: 2013-8-27
*      Author: ziteng@mogujie.com
*/



#ifndef IMPDUBASE_H_
#define IMPDUBASE_H_
#include"util_pdu.h"
#include<memory>
#define IM_PDU_HEADER_LEN		21
#define IM_PDU_VERSION			1


#define OFFSET_PDU_HDR_TAG              0
#define OFFSET_PDU_HDR_VERSION       2
#define OFFSET_PDU_HDR_CMDID          3
#define OFFSET_PDU_HDR_PKTLENGTH   5
#define OFFSET_PDU_HDR_SESSIONID    9
#define OFFSET_PDU_HDR_DESTID         13
#define OFFSET_PDU_HDR_RESERVERD   17



#define ALLOC_FAIL_ASSERT(p) if (p == NULL) { \
throw CPduException(m_pdu_header.service_id, m_pdu_header.command_id, ERROR_CODE_ALLOC_FAILED, "allocate failed"); \
}

#define CHECK_PB_PARSE_MSG(ret) { \
    if (ret == false) \
    {\
        log("parse pb msg failed.");\
        return;\
    }\
}



//////////////////////////////
/*
typedef struct {
uint32_t 	length;		  // the whole pdu length
uint16_t 	version;	  // pdu version number
uint16_t	flag;		  // not used
uint16_t	service_id;	  //
uint16_t	command_id;	  //
uint16_t	seq_num;     // °üÐòºÅ
uint16_t    reversed;    // ±£Áô
} PduHeader_t;
*/

//////////////////////////////
typedef struct {
	uint16_t 	tag;
	uint8_t 	version;	  // pdu version number
	uint16_t	cmdid;	  //
	uint32_t	pkLength;
	uint32_t    sessionid;
	uint32_t    destid;
	uint32_t    reversed;
} pdu_header_t;

class  im_pdu
{
public:
	im_pdu();
	
	virtual ~im_pdu();

	uchar_t* get_buffer();
	uint32_t get_length();
	uchar_t* get_bodyData();
	uint32_t get_bodyLength();

	uint8_t get_version() { return m_pdu_header.version; }
	uint16_t get_tag() { return m_pdu_header.tag; }
	uint16_t get_command_Id() { return m_pdu_header.cmdid; }
	uint32_t get_dest_Id() { return m_pdu_header.destid; }
	uint32_t get_session_Id() { return m_pdu_header.sessionid; }
	uint32_t get_reversed() { return m_pdu_header.reversed; }
	net_handle_t get_sock_handle() { return m_sock_handle; }

	void set_version(uint8_t version);
	void set_tag(uint16_t tag);
	void set_command_Id(uint16_t command_id);
	void set_dest_Id(uint32_t dest_id);
	void set_session_Id(uint32_t session_id);
	void set_sock_handle(net_handle_t sock_handle);
	void set_reversed(uint32_t reversed);
	
	void write_header();
	
	static bool is_pdu_available(uchar_t* buf, uint32_t len, uint32_t& pdu_len);
	
	
        static  std::shared_ptr<im_pdu> read_pdu(uchar_t* buf, uint32_t len);
	void write(uchar_t* buf, uint32_t len);
	int read_pdu_header(uchar_t* buf, uint32_t len);
	
	void set_PB_msg(const char* msg);
	
protected:
	simple_buffer	m_buf;
	pdu_header_t	m_pdu_header;
	net_handle_t    m_sock_handle;

};


#endif /* IMPDUBASE_H_ */

