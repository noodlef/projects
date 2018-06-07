#pragma once
#ifndef TOOLS_H
#define TOOLS_H
#include <stdio.h>
#define WINDOW_WIDTH          160  // ����̨���ڵĿ�� 192
#define WINDOW_HEIGHT         40  // ����̨���ڵĸ߶� 60

															
//   0 = ��ɫ      8 = ��ɫ
//   1 = ��ɫ      9 = ����ɫ
//   2 = ��ɫ      A = ����
//   3 = ǳ��ɫ    B = ��ǳ��ɫ
//   4 = ��ɫ      C = ����ɫ
//   5 = ��ɫ      D = ����ɫ
//   6 = ��ɫ      E = ����ɫ
//   7 = ��ɫ      F = ����ɫ


// ����̨ǰ����ɫ
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
	CFC_Cyan = 36,//��ɫ
	CFC_Gray = 37,
	CFC_White = 37,
	CFC_HighWhite = 37,
	CFC_Black = 30,
};
//����̨����ɫ
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
