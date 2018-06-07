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
	mix_long euid;                     // 有效用户 id
	mix_long uid;                      // 真实用户 id
	mix_long egid;                     // 有效用户组 id
	mix_long gid;                      // 真实用户的组 id
	struct m_inode* root;              // 指向进程当前所处的文件系统的根 i 节点
	struct m_inode* pwd;               // 进程当前工作目录
	short umask;                       // 进程访问文件屏蔽码
	struct file* filp[NR_OPEN];        // 文件结构指针表，表项号就是文件描述符的值
	int tty;                           // 进程使用的终端的子设备号， -1 表示没有终端
};
extern struct task_struct* current;
extern unsigned int current_time();
extern int suser();
void print_date(int time);
#define CURRENT_TIME (current_time())
#endif
