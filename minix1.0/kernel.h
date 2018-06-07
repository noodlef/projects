#pragma once
#ifndef KERNEL_H
#define KERNEL_H
#include"file_sys.h"
#include"stdarg.h"
#include"stdio.h"
#include"time.h"
int panic(const char* format, ...);
int printk(const char* format, ...);
extern int M_printf(const char*s, ...);
struct task_struct
{
	mix_long euid;                     // ��Ч�û� id
	mix_long uid;                      // ��ʵ�û� id
	mix_long egid;                     // ��Ч�û��� id
	mix_long gid;                      // ��ʵ�û����� id
	struct m_inode* root;              // ָ����̵�ǰ�������ļ�ϵͳ�ĸ� i �ڵ�
	struct m_inode* pwd;               // ���̵�ǰ����Ŀ¼
	short umask;                       // ���̷����ļ�������
	struct file* filp[NR_OPEN];        // �ļ��ṹָ�������ž����ļ���������ֵ
	int tty;                           // ����ʹ�õ��ն˵����豸�ţ� -1 ��ʾû���ն�
};
extern struct task_struct* current;
extern unsigned int current_time();
extern int suser();
void print_date(int time);
#define CURRENT_TIME (current_time())
#endif
