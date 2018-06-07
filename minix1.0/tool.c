#include"tool.h"

const int TTY_WIDTH = (WINDOW_WIDTH - 4 * FRAM_WIDTH - FRAM_GAP - 2 * DIS) / 2;
const int TTY_HEIGHT = (WINDOW_HEIGHT - 4 * FRAM_WIDTH -  FRAM_GAP - 2 * DIS) / 2;
// ���ڳ�ʼ��
void window_init(int cols, int rows)
{
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD size;
	SMALL_RECT rc;
	SetConsoleTitle(L"MINIX_FILE_SYSTEM");
	size.X = cols;
	size.Y = rows;
	SetConsoleScreenBufferSize(h_out, size);
	rc.Top = rc.Left = 0;
	rc.Right = cols - 1;
	rc.Bottom = rows - 1;
	SetConsoleWindowInfo(h_out, 1, &rc);
}
// ���ù��λ��
void set_cursor_position(int x, int y)
{
	COORD position;
	position.X = x;
	position.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), position);
}
//
void set_cursor_position_d()
{
	set_cursor_position(WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1);
}
// �����ı���ɫ���ı�����ɫ
void set_console_color(enum CF_color F_colorID, enum CB_color B_colorID)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), F_colorID | B_colorID);
}

// �����ı�����ɫ
void set_background(enum CB_color colorID)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorID | CFC_White);
}
// �����ı�ǰ��ɫ
void set_foreground(enum CF_color colorID)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorID | CBC_Black);
}

// �ָ��ı���ɫ���ı�����ɫΪĬ��ֵ-- �� ��
void set_console_color_d()
{
	enum CF_color F_colorID = CFC_White;
	enum CB_color B_colorID = CBC_Black;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), F_colorID | B_colorID);
}