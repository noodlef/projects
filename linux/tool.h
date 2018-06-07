#ifndef TOOLS_H
#define TOOLS_H
#include <stdio.h>
#define WINDOW_WIDTH          192 // 控制台窗口的宽度 192
#define WINDOW_HEIGHT         60  // 控制台窗口的高度 60
#define FRAM_WIDTH             1  //
#define FRAM_GAP               3  //
#define DIS 2
extern const int TTY_WIDTH;
extern const int TTY_HEIGHT;
															
//   0 = 黑色      8 = 灰色
//   1 = 蓝色      9 = 淡蓝色
//   2 = 绿色      A = 淡绿
//   3 = 浅绿色    B = 淡浅绿色
//   4 = 红色      C = 淡红色
//   5 = 紫色      D = 淡紫色
//   6 = 黄色      E = 淡黄色
//   7 = 白色      F = 亮白色


// 控制台前景颜色
struct COORD{
       int X, Y;
};
enum CF_color
{
	CFC_Red = 31,
	CFC_Green = 32,
	CFC_Blue = 34,
	CFC_Yellow = 33,
	CFC_Purple = 35,
	CFC_Cyan = 36,//青色
	CFC_Gray = 37,
	CFC_White = 37,
	CFC_HighWhite = 37,
	CFC_Black = 30,
};
//控制台背景色
enum CB_color
{
	CBC_Red = 41,
	CBC_Green = 42,
	CBC_Blue = 44,
	CBC_Yellow = 43,
	CBC_Purple = 45,
	CBC_Cyan = 47,
	CBC_White = 47,
	CBC_HighWhite = 47,
	CBC_Black = 40,
};

void window_init(int cols, int rows);
void set_cursor_position(int x, int y);
void set_foreground(enum CF_color colorID);
void set_background(enum CB_color colorID);
void set_console_color(enum CF_color F_colorID, enum CB_color B_colorID);
void set_console_color_d();
void set_cursor_position_d();
#endif 
