#pragma once
#include "ostype.h"
#include <set>
#include <map>
#include <list>
#include <string.h>
using namespace std;


#define ERROR_CODE_PARSE_FAILED 		1
#define ERROR_CODE_WRONG_SERVICE_ID		2
#define ERROR_CODE_WRONG_COMMAND_ID		3
#define ERROR_CODE_ALLOC_FAILED			4

class pdu_exception {
public:
	pdu_exception(uint32_t service_id, uint32_t command_id, uint32_t error_code, const char* error_msg)
	{
		m_service_id = service_id;
		m_command_id = command_id;
		m_error_code = error_code;
		m_error_msg = error_msg;
	}

	pdu_exception(uint32_t error_code, const char* error_msg)
	{
		m_service_id = 0;
		m_command_id = 0;
		m_error_code = error_code;
		m_error_msg = error_msg;
	}

	virtual ~pdu_exception() {}

	uint32_t get_service_Id() { return m_service_id; }
	uint32_t get_command_Id() { return m_command_id; }
	uint32_t get_error_code() { return m_error_code; }
	char* get_error_msg() { return (char*)m_error_msg.c_str(); }
private:
	uint32_t	m_service_id;
	uint32_t	m_command_id;
	uint32_t	m_error_code;
	string		m_error_msg;
};

class byte_stream
{
public:
	static int8_t read_int8(uchar_t* buf)
	{
		int8_t data = static_cast<int8_t>(buf[0]);
		return data;
	}
	static uint8_t read_uint8(uchar_t* buf)
	{
		uint8_t data = static_cast<uint8_t>(buf[0]);
		return data;
	}
	static int16_t read_int16(uchar_t* buf)
	{
		int16_t data = (buf[0] << 8) | buf[1];
		return data;
	}
	static uint16_t read_uint16(uchar_t* buf)
	{
		uint16_t data = (buf[0] << 8) | buf[1];
		return data;
	}
	static int32_t read_int32(uchar_t* buf)
	{
		int32_t data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
		return data;
	}
	static uint32_t read_uint32(uchar_t* buf)
	{
		uint32_t data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
		return data;
	}
	static void write_int8(uchar_t* buf, int8_t data)
	{
		buf[0] = static_cast<uchar_t>(data);
	}
	static void write_uint8(uchar_t* buf, uint8_t data)
	{
		buf[0] = static_cast<uchar_t>(data);
	}
	static void write_int16(uchar_t* buf, int16_t data)
	{
		buf[0] = static_cast<uchar_t>(data >> 8);
		buf[1] = static_cast<uchar_t>(data & 0xFF);
	}
	static void write_uint16(uchar_t* buf, uint16_t data)
	{
		buf[0] = static_cast<uchar_t>(data >> 8);
		buf[1] = static_cast<uchar_t>(data & 0xFF);
	}
	static void write_int32(uchar_t* buf, int32_t data)
	{
		buf[0] = static_cast<uchar_t>(data >> 24);
		buf[1] = static_cast<uchar_t>((data >> 16) & 0xFF);
		buf[2] = static_cast<uchar_t>((data >> 8) & 0xFF);
		buf[3] = static_cast<uchar_t>(data & 0xFF);
	}
	static void write_uint32(uchar_t* buf, uint32_t data)
	{
		buf[0] = static_cast<uchar_t>(data >> 24);
		buf[1] = static_cast<uchar_t>((data >> 16) & 0xFF);
		buf[2] = static_cast<uchar_t>((data >> 8) & 0xFF);
		buf[3] = static_cast<uchar_t>(data & 0xFF);
	}
};




class  simple_buffer
{
public:
	simple_buffer()
	{
		m_buffer = NULL;
		m_alloc_size = 0;
		m_write_offset = 0;
	}
	~simple_buffer() 
	{
		m_alloc_size = 0;
		m_write_offset = 0;
		if (m_buffer)
		{
			free(m_buffer);
			m_buffer = NULL;
		}
	}
	uchar_t*  get_buffer() { return m_buffer; }
	uint32_t get_alloc_size() { return m_alloc_size; }
	uint32_t get_write_offset() { return m_write_offset; }
	void inc_write_offset(uint32_t len) { m_write_offset += len; }

	void extend(uint32_t len)
	{
		m_alloc_size = m_write_offset + len;
		m_alloc_size += m_alloc_size >> 2;	// increase by 1/4 allocate size
		uchar_t* new_buf = (uchar_t*)realloc(m_buffer, m_alloc_size);
		m_buffer = new_buf;
	}
	uint32_t write(void* buf, uint32_t len)
	{
		if (m_write_offset + len > m_alloc_size)
		{
			extend(len);
		}

		if (buf)
		{
			memcpy(m_buffer + m_write_offset, buf, len);
		}

		m_write_offset += len;

		return len;
	}
	uint32_t read(void* buf, uint32_t len)
	{
		if (len > m_write_offset)
			len = m_write_offset;
		if (buf)
			memcpy(buf, m_buffer, len);
		m_write_offset -= len;
		memmove(m_buffer, m_buffer + len, m_write_offset);
		return len;
	}
private:
	uchar_t*	m_buffer;
	uint32_t	m_alloc_size;
	uint32_t	m_write_offset;
};
