#include<stdio.h>
#include<termios.h>
#include<sys/ioctl.h>
#include<unistd.h>
//#define set_console_textAttribute(F, B) {\
//printf("\033["#F"m \033[1m");\
//printf("\033["#B"m");\
//}
#define clear_screen(void) printf("\033[2J")
#include"tool.h"

int getch(void)
{
     struct termios tm, tm_old;
     int fd = 0, ch;

     if (tcgetattr(fd, &tm) < 0) {//保存现在的终端设置
          return -1;
     }

     tm_old = tm;
     cfmakeraw(&tm);//更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
     if (tcsetattr(fd, TCSANOW, &tm) < 0) {//设置上更改之后的设置
          return -1;
     }

     ch = getchar();
     if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {//更改设置为最初的样子
          return -1;
     }

     return ch;
}

int main()
{
//char* c = 0;
struct winsize size;
int i = 1;
size.ws_row = 20;
size.ws_col = 64;
ioctl(STDIN_FILENO,TIOCSWINSZ,(char*)&size);
//printf("\033[0m \033[2J");
printf("\033[%d;%dH", 2,2);
//printf("\033[31m \033[1m");
//printf("\033[47m");
window_init(0,0);
set_console_color(CFC_Red, CBC_Blue);
set_cursor_position(5, 5);
while(i < 50){
//int ch = getch();
set_cursor_position(i, i);
//ch = getch();
printf("%d\n", i++);
}
//printf("%d rows, %d columns\n",size.ws_row, size.ws_col);  
}
