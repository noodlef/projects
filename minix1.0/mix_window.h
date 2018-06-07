#pragma once
#ifndef MIX_WINDOW_H
#define MIX_WINDOW_H
#include"tool.h"
#define KEY_ENTER 13
#define KEY_BACKSPACE 8
#define KEY_ESC 0
#define MAX_SCREEN_BUF_SIZE  (WINDOW_WIDTH * WINDOW_HEIGHT / 4) * 2    // tty ��ʾ�����������ߴ�

// �༭ģʽ
#define AM_MODE 0x02                        // ����ģʽλ
#define TEXT_HIGHLIGHT 0x01                 // �ı�������ʾλ
#define IS_APPEND(mode) (!(AM_MODE & mode)) // ���ģʽ
#define IS_MODIFY(mode) (AM_MODE & mode)    // �޸�ģʽ
#define IS_THLIGHT(mode) (TEXT_HIGHLIGHT & mode)

// ����굽��ͷ��βʱ�����ļ��������ж�����д��
typedef int(*tty_fn)(struct tty_window* tty, unsigned int ht, unsigned int rw);
#define MT_HEAD 0
#define MT_TAIL 1
#define MT_READ 2
#define MT_WRITE 3

// �ĸ����ڵ�ԭ������
extern struct cursor_pos origin_pos_tty[4];
extern struct tty_window t_window[4];
extern struct tty_window* tty_dev[4];
typedef  int cur_pos_t;
// ���λ��
struct cursor_pos {
	cur_pos_t x;
	cur_pos_t y;
};
// �߿�
struct win_fram {
	char shape;
	cur_pos_t width;
	enum CF_color f_color;
	enum CB_color b_color;
};

// ��ʾ���ַ�
struct mix_char {
	char ch;                        // �ַ�
	enum CF_color f_color;          // �������ɫ
	enum CB_color b_color;          // ����ı�����ɫ
	char b_set;                     // �Ƿ�����������ı�����ɫ�� 0 - û��
};
// 
struct M_buf {
	struct mix_char m_char;
	int prev;                     
	int next;                      
};

// ��ʾ������
struct screen_buf {
	int s_head;                                   // ������ͷָ�� -- ָ���һ���ַ�
	int s_tail;                                   // ������βָ�� -- ָ�����һ���ַ�����һ��λ��
	int s_size;                                   // �������Ĵ�С

	int free_list;                                // ָ����еĻ����  ��12.9��
	int s_pos;                                    // ָ��ǰ�Ķ�дλ��  ��12.9��
	struct M_buf m_buf[MAX_SCREEN_BUF_SIZE];      //   ��12.9��

	                                              //struct mix_char s_buf[MAX_SCREEN_BUF_SIZE];
	int cur_head;                                 // ָ����ʾ����Ļ�ϵĵ�һ���ַ�
	int cur_tail;                                 // ָ����ʾ����Ļ�ϵ����һ���ַ�����һ��λ��
};
// ����
struct tty_window {
	char name[10];                  // ����
	struct cursor_pos origin_pos;   // ����ԭ�㣬���Ͻ�
	struct cursor_pos current_pos;  // ���ĵ�ǰλ��
	cur_pos_t win_height;           // ���ڵĸ߶�
	cur_pos_t win_width;            // ���ڵĿ��
	struct win_fram fram;           // ���ڵı�Ե����ʾ�ַ�����ɫ
	enum CF_color f_color;          // ���õ�ǰ������ʾ����ɫ
	enum CB_color b_color;          // ��Ļ�ı�����ɫ
	enum CB_color char_b_color;     // ��ǰ�ַ���ʾ�ı�����ɫ
	int button;                     // ��ʾ��ǰ���õ����屳��ɫ�Ƿ���Ч 0 - ��
	struct screen_buf buf;          // ��ʾ������

	unsigned int fd_set[2];         // fd_set[0]-- ͷ����д�� fd_set[1] -- β����д
	tty_fn HT_rw;                   // ���ﻺ����ͷʱ����ָ���ļ����ж�д���� ��2018.1.14��
									 
};

// ����ƶ�����
enum {
	SBUF_UP,
	SBUF_DOWN,
	SBUF_LEFT,
	SBUF_RIGHT
};

// ��׼�������
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
