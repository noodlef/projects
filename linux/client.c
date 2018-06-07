#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<strings.h>


#define PORT 48888
static char buff[128];

void do_stuff(int connfd){
	int err = 1;
       
	while(err > 0){
              if((err = recv(connfd, buff, sizeof(buff) - 1, 0)) < 0){
                 printf(" client : do_stuff failed!\n");
	      }
              else{
                 buff[err] = 0;
                 printf("%s\n", buff);
              }
	}
        return;
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
