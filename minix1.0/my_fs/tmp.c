#include<stdio.h>
#include<unistd.h>
#include<termios.h>
int getch(void)
{
     struct termios tm, tm_old;
     int fd = 0, ch;

     if (tcgetattr(fd, &tm) < 0) {
          return -1;
     }

     tm_old = tm;
     cfmakeraw(&tm);
     if (tcsetattr(fd, TCSANOW, &tm) < 0) {
          return -1;
     }

     ch = getchar();
     if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {
          return -1;
     }

     return ch;
}
int main(int argc, char** argv)
{
	char ch;
	while(1){
	    ch = getch();
            printf("%d\n", ch);
	}
	return 0;
}

