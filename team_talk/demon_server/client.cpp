/*
* demon_sever.cpp
*/
#include "stdafx.h"
#include "echo_conn.h"
#include "netlib.h"
#include "config_file_reader.h"
#include "version.h"
#include <fstream>


void echo_client_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	if ((msg == NETLIB_MSG_CONNECT) || (msg == NETLIB_MSG_CONFIRM))
	{
		echo_conn* conn = new echo_conn();
		conn->on_connect(handle, LOGIN_CONN_TYPE_CLIENT);
	}
	else
	{
		mix_log("!!!error msg: %d ", msg);
	}
}


int main(int argc, char* argv[]) {
	const char* server_IP = "127.0.0.1";
	const char* server_port = "48888";
	uint16_t port = atoi(server_port);
	// 网络相关的初始化
	int ret = netlib_init();
	if (ret == NETLIB_ERROR) {
		return ret;
	}

	ret = netlib_connect(server_IP, port, echo_client_callback, NULL);
	if (ret == NETLIB_ERROR) {
		return ret;
	}

	init_echo_conn();
	printf("now enter the event loop...\n");
	netlib_eventloop();
	return 1;
}
