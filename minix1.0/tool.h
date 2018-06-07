#pragma once
#ifndef TOOLS_H
#define TOOLS_H
#include<Windows.h>
#include <stdio.h>
#define WINDOW_WIDTH          60  // ����̨���ڵĿ�� 192
#define WINDOW_HEIGHT         40  // ����̨���ڵĸ߶� 60
#define FRAM_WIDTH             2  //
#define FRAM_GAP               3  //
#define DIS 2
extern const int TTY_WIDTH;
extern const int TTY_HEIGHT;
															
//   0 = ��ɫ      8 = ��ɫ
//   1 = ��ɫ      9 = ����ɫ
//   2 = ��ɫ      A = ����
//   3 = ǳ��ɫ    B = ��ǳ��ɫ
//   4 = ��ɫ      C = ����ɫ
//   5 = ��ɫ      D = ����ɫ
//   6 = ��ɫ      E = ����ɫ
//   7 = ��ɫ      F = ����ɫ


// ����̨ǰ����ɫ
enum CF_color
{
	CFC_Red = FOREGROUND_INTENSITY | FOREGROUND_RED,
	CFC_Green = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
	CFC_Blue = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
	CFC_Yellow = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
	CFC_Purple = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
	CFC_Cyan = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,//��ɫ
	CFC_Gray = FOREGROUND_INTENSITY,
	CFC_White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CFC_HighWhite = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	CFC_Black = 0,
};
//����̨����ɫ
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
