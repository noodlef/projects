#define _GNU_SOURCE 1
#include<sys/type.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>
#include<fcntl.h>

#define PORT 49999
#define BUFFER_SIZE 64

void do_stuff(int fd){

	pollfd fds[2] = {{0, POLLIN, 0},{confd, POLLIN | POLLRDHUP, 0}};
	char read_buf[BUFFER_SIZE];
	int pipefd[2];
	int ret = pipe(pipefd);
        assert(ret != -1);

	while(1){
	      ret = poll(fds, 2, -1);
	      if(ret < 0) { printf("poll failure\n"); break;}
              if(fds[1].revents & POLLRDHUB) { printf("server close the connection\n"); break;}
	      else if(fds[1].revents & POLLIN){
	              memset(read_buf, '\0', BUFFER_SIZE);
 	              recv(fd[1].fd, read_buf, BUFFER_SIZE - 1, 0);
		      printf("%s\n", read_buf);
	      }
              if(fds[0].revents & POLLIN){
		 ret = splice(0, NULL, pipefd[1], NULL, 32768, 
                                 SPLICE_F_MORE | SPLICE_F_MOVE);
	         ret = splice(pipefd[0], NULL, connfd, NULL, 32768, 
                                 SPLICE_F_MORE | SPLICE_F_MOVE);
	      }
	}
}

int main(int argc, char* argv[]){
	
        const char* ip = "127.0.0.1";    // argv[1];
        int port = PORT;                 // atoi(argv[2]);
        int backlog = 5;
        socklen_t len;
        
        int connfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(connfd >= 0);
        
        struct sockaddr_in sever_addr, client_addr;
        bzero(&sever_addr, sizeof(sever_addr));
        sever_addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &sever_addr.sin_addr);
        sever_addr.sin_port = htons(port);
        
           
        int err = connect(connfd, (struct sockaddr*)&sever_addr, sizeof(sever_addr));
        assert(err != -1);
        
	
        len = sizeof(client_addr);
        err = getsockname(connfd, (struct sockaddr*)&client_addr, &len);
        assert(err != -1);
        inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff));
        printf("%s %d connecting to ...", buff, ntohs(client_addr.sin_port));

        do_stuff(connfd);
        
        close(connfd);
        return 0;
        
}
