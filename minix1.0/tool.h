#pragma once
#ifndef TOOLS_H
#define TOOLS_H
#include<Windows.h>
#include <stdio.h>
#define WINDOW_WIDTH          60  // 控制台窗口的宽度 192
#define WINDOW_HEIGHT         40  // 控制台窗口的高度 60
#define FRAM_WIDTH             2  //
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
enum CF_color
{
	CFC_Red = FOREGROUND_INTENSITY | FOREGROUND_RED,
	CFC_Green = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
	CFC_Blue = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
	CFC_Yellow = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
	CFC_Purple = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
	CFC_Cyan = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,//青色
	CFC_Gray = FOREGROUND_INTENSITY,
	CFC_White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CFC_HighWhite = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CFC_Black = 0,
};
//控制台背景色
enum CB_color
{
	CBC_Red = BACKGROUND_INTENSITY | BACKGROUND_RED,
	CBC_Green = BACKGROUND_INTENSITY | BACKGROUND_GREEN,
	CBC_Blue = BACKGROUND_INTENSITY | BACKGROUND_BLUE,
	CBC_Yellow = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
	CBC_Purple = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
	CBC_Cyan = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
	CBC_White = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	CBC_HighWhite = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	CBC_Black = 0,
};

void window_init(int cols, int rows);
void set_cursor_position(int x, int y);
void set_foreground(enum CF_color colorID);
void set_background(enum CB_color colorID);
void set_console_color(enum CF_color F_colorID, enum CB_color B_colorID);
void set_console_color_d();
void set_cursor_position_d();
#endif 
