#pragma once
#ifndef MIX_WINDOW_H
#define MIX_WINDOW_H
#include"tool.h"
#define KEY_ENTER 13
#define KEY_BACKSPACE 8
#define KEY_ESC 0
#define MAX_SCREEN_BUF_SIZE  (WINDOW_WIDTH * WINDOW_HEIGHT / 4) * 2    // tty 显示缓冲区的最大尺寸

// 编辑模式
#define AM_MODE 0x02                        // 输入模式位
#define TEXT_HIGHLIGHT 0x01                 // 文本高亮显示位
#define IS_APPEND(mode) (!(AM_MODE & mode)) // 添加模式
#define IS_MODIFY(mode) (AM_MODE & mode)    // 修改模式
#define IS_THLIGHT(mode) (TEXT_HIGHLIGHT & mode)

// 当光标到达头或尾时，从文件描述符中读出或写入
typedef int(*tty_fn)(struct tty_window* tty, unsigned int ht, unsigned int rw);
#define MT_HEAD 0
#define MT_TAIL 1
#define MT_READ 2
#define MT_WRITE 3

// 四个窗口的原点坐标
extern struct cursor_pos origin_pos_tty[4];
extern struct tty_window t_window[4];
extern struct tty_window* tty_dev[4];
typedef  int cur_pos_t;
// 光标位置
struct cursor_pos {
	cur_pos_t x;
	cur_pos_t y;
};
// 边框
struct win_fram {
	char shape;
	cur_pos_t width;
	enum CF_color f_color;
	enum CB_color b_color;
};

// 显示的字符
struct mix_char {
	char ch;                        // 字符
	enum CF_color f_color;          // 字体的颜色
	enum CB_color b_color;          // 字体的背景颜色
	char b_set;                     // 是否设置了字体的背景颜色， 0 - 没有
};
// 
struct M_buf {
	struct mix_char m_char;
	int prev;                     
	int next;                      
};

// 显示缓冲区
struct screen_buf {
	int s_head;                                   // 缓冲区头指针 -- 指向第一个字符
	int s_tail;                                   // 缓冲区尾指针 -- 指向最后一个字符的下一个位置
	int s_size;                                   // 缓冲区的大小

	int free_list;                                // 指向空闲的缓冲块  （12.9）
	int s_pos;                                    // 指向当前的读写位置  （12.9）
	struct M_buf m_buf[MAX_SCREEN_BUF_SIZE];      //   （12.9）

	                                              //struct mix_char s_buf[MAX_SCREEN_BUF_SIZE];
	int cur_head;                                 // 指向显示在屏幕上的第一个字符
	int cur_tail;                                 // 指向显示在屏幕上的最后一个字符的下一个位置
};
// 窗口
struct tty_window {
	char name[10];                  // 名字
	struct cursor_pos origin_pos;   // 窗口原点，左上角
	struct cursor_pos current_pos;  // 光标的当前位置
	cur_pos_t win_height;           // 窗口的高度
	cur_pos_t win_width;            // 窗口的宽度
	struct win_fram fram;           // 窗口的边缘的显示字符和颜色
	enum CF_color f_color;          // 设置当前字体显示的颜色
	enum CB_color b_color;          // 屏幕的背景颜色
	enum CB_color char_b_color;     // 当前字符显示的背景颜色
	int button;                     // 表示当前设置的字体背景色是否有效 0 - 无
	struct screen_buf buf;          // 显示缓冲区

	unsigned int fd_set[2];         // fd_set[0]-- 头部读写， fd_set[1] -- 尾部读写
	tty_fn HT_rw;                   // 到达缓冲区头时，对指定文件进行读写数据 （2018.1.14）
									 
};

// 光标移动方向
enum {
	SBUF_UP,
	SBUF_DOWN,
	SBUF_LEFT,
	SBUF_RIGHT
};

// 标准输入输出
extern int MIX_STD_CERR;
extern int MIX_STD_OUT;
extern MIX_STD_IN;


void tty_init();
int tty_read(unsigned minor, char * buf, int count);
int keybord(struct tty_window* tty);
int new_line(unsigned minor);
int clc_win(unsigned minor);
int set_char_fcolor(unsigned minor, enum CF_color f_color);
int set_char_bcolor(unsigned minor, enum CB_color b_color);
int set_tty_color(unsigned minor, enum CB_color b_color);
int mix_cerr(const char* const s);
int tty_write(unsigned minor, struct mix_char * buf, int count);
int move_sbuf_cursor(int direc, struct tty_window* tty);
int get_line_ht(struct tty_window* tty, int pos, int* head, int* tail);
int down_line(struct tty_window* tty, int pos, int lines);
struct cursor_pos
	map_to_screen(struct tty_window* tty, int pos);
int s_flush(struct tty_window* tty);
int print_cur_dir();
int mix_print(struct mix_char* buf, int count, unsigned minor);
void cursor_flash(struct tty_window* tty);
#endif
