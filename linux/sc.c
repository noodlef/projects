
//#include<sys/type.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>
#include<fcntl.h>
#include<strings.h>
#include<stdio.h>
struct conn_table {
	char ip[32];
	char port[8];
	short state;
};

static struct conn_table conn_table[32]=
{
	{"0.0.0.0"  ,"0000", 0 },
	{"127.0.0.1","48888", 0 },
	{"127.0.0.1","48889", 0 },
 };
static int conn_count = 2;

static int search_conn_table() {
	for (int i = 1; i < conn_count; i++) {
		if (!conn_table[i].state) {
			return i;
		}
	}
	return -1;
}

static int search_ip(struct sockaddr_in* client) {
	char ip[32] = { 0 };
        short port = ntohs(client->sin_port);
	inet_ntop(AF_INET, &client->sin_addr, ip, sizeof(ip));
	for (int i = 1; i < conn_count; i++) {
		if (!strcmp(conn_table[i].ip, ip) && 
			(atoi(conn_table[i].port) == port)) {
			return i;
		}
	}
	return -1;
}



void do_stuff(int connfd) {
	struct sockaddr_in client_addr, server_addr;
	int len = sizeof(client_addr);
	char buff[32] = { 0 };
	int err = getsockname(connfd, (struct sockaddr*)&client_addr, &len);
	assert(err != -1);
	inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff));
	printf("%s %d connecting to ", buff, ntohs(client_addr.sin_port));

	err = getpeername(connfd, (struct sockaddr*)&server_addr, &len);
	assert(err != -1);
	inet_ntop(AF_INET, &server_addr.sin_addr, buff, sizeof(buff));
	printf("%s %d established ...\n", buff, ntohs(server_addr.sin_port));
	return;
}


int main(int argc, char* argv[]) {
	int index = 2, backlog = 5;
	const char* ip = conn_table[index].ip;    
	int port = atoi(conn_table[index].port);
	// 设定服务器地址
	struct sockaddr_in sever_addr, client_addr;
	bzero(&sever_addr, sizeof(sever_addr));
	sever_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &sever_addr.sin_addr);
	sever_addr.sin_port = htons(port);
	// 创建监听描述符
	int listenfd, connfd, i;
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	// 绑定ip
	int err = bind(listenfd, (struct sockaddr*) &sever_addr, 
		sizeof(sever_addr));
	assert(err >= 0);
	// listen
	err = listen(listenfd, backlog);
	assert(err >= 0);

        struct pollfd pollfd = {listenfd, POLLIN, 0};
	while (!conn_table[0].state) {
		err = poll(&pollfd, 1, 16);
		if (err < 0) {
			printf("poll failure!\n");
			exit(0);
		}
		else if (err == 0) ;//continue;
		else {
                        int len = sizeof(client_addr);
			connfd = accept(listenfd, (struct sockaddr*)&client_addr, &len);
			if(connfd < 0) {
				printf("accept failure!\n");
				continue;
			}
			// do stuff
			do_stuff(connfd);
			// update conn_table;
			if ((i = search_ip(&client_addr)) > 0) {
				conn_table[i].state = 1;
				if (search_conn_table() < 0)
					conn_table[i].state = 1;
			}
			
		}
		connfd = socket(PF_INET, SOCK_STREAM, 0);
		assert(connfd != -1);
		err = bind(connfd, (struct sockaddr*) &sever_addr,
			       sizeof(sever_addr));
		assert(err >= 0);
		i = search_conn_table();
		if (i > 0) {
			// client_addr
			bzero(&client_addr, sizeof(client_addr));
			client_addr.sin_family = AF_INET;
			inet_pton(AF_INET, conn_table[i].ip, &client_addr.sin_addr);
			sever_addr.sin_port = htons(atoi(conn_table[i].port));
			// connect
			err = connect(connfd, (struct sockaddr*)&client_addr,
				sizeof(client_addr));
			if (err < 0) continue;
			// do stuff
			do_stuff(connfd);
			// update conn_table
			conn_table[i].state = 1;
			if (search_conn_table() < 0) 
				conn_table[i].state = 1;
		}
	}
	// close fds


	return 0;

}
