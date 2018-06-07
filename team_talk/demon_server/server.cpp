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


int _tserver(int argc, char* argv[])
{

	//signal(SIGPIPE, SIG_IGN);

	// 检查配置文件是否存在
	ifstream ifs("config_file.config");
	if (!ifs) {
		mix_log("config_file.config\r\n");
		return -1;
	}
	// 获取配置文件， 并设定服务器监听 ip.port
	config_file_reader config_file("config_file.config");
	const char* listen_IP = config_file.get_config_name("listen_IP");
	const char* listen_port = config_file.get_config_name("listen_port");
	uint16_t port = atoi(listen_port);
	if (!listen_IP || !listen_port) {
		mix_log("can not get listen_IP or listen port!\n");
		return -1;
	}
	// 网络相关的初始化
	int ret = netlib_init();
	if (ret == NETLIB_ERROR) {
		return ret;
	}
	// 服务器进入监听状态
	ret = netlib_listen(listen_IP, port, echo_client_callback, NULL);
	if (ret == NETLIB_ERROR) {
		return ret;
	}
	printf("server start listen on:\nFor client %s:%d\n", listen_IP, port);
	// 
	init_echo_conn();
	printf("now enter the event loop...\n");
	// 将服务器的pid号记录在 server.pid 文件中
	// write_pid();
	// 开始事件循环处理
	netlib_eventloop();
	return 0;
}




int main(int argc, char* argv[]){


	_tserver(argc, argv);
        return 0;

}
