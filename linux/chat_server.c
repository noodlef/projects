#define _GNU_SOURCE 1
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<strings.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<poll.h>

#define PORT 48888
#define BUFFER_SIZE 64
static char buff[128];

void do_stuff(int connfd){
	int filefd = open("../minix1.0/tty_io.c", O_RDONLY);
        assert(filefd > 0);
        size_t count = 1024;
        off_t offset = 0, err = 1;
	
        off_t size = lseek(filefd, 0, SEEK_END);
        assert(size != -1);

        sprintf(buff, "%d bytes to send!\n", size);
        send(connfd, buff, strlen(buff), 0);

	while(err > 0){
 	      if((err = sendfile(connfd, filefd, &offset, count)) < 0){
                  printf("do_stuff failed!\n");
                  return;
	      }
              size = size - err;
	}

        sprintf(buff, "%d bytes received, %d bytes left!\n", offset, size);
        send(connfd, buff, strlen(buff), 0);
        return;
}

int main(int argc, char* argv[]){
	/*signal(SIGTERM, handle_term);
        if{
	    printf("usage : %s ip_address port_number backlog\n", basename(argv[0]);
            return 1;
        }*/
        const char* ip = "127.0.0.1";    // argv[1];
        int port = PORT;                 // atoi(argv[2]);
        int backlog = 5;
        socklen_t len;
        
        int listenfd, connfd;
        listenfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(listenfd >= 0);
        
        struct sockaddr_in sever_addr, client_addr;
        bzero(&sever_addr, sizeof(sever_addr));
        sever_addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &sever_addr.sin_addr);
        sever_addr.sin_port = htons(port);
        
        int err = bind(listenfd, (struct sockaddr*) &sever_addr, sizeof(sever_addr));
        assert(err != -1);
            
        err = listen(listenfd, backlog);
        assert(err != -1);
        
         
       // for(; ;){
            connfd = accept(listenfd, (struct sockaddr*) &client_addr, &len);
            assert(connfd != -1);
            
            inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff));
            printf("connection from %s, port %d\n", buff, ntohs(client_addr.sin_port));
            
            do_stuff(connfd);
            
            close(connfd);
       // }
        return 0;
        
}
