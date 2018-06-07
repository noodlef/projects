#pragma once
#ifndef STAT_H
#define STAT_H
#include"mix_type.h"

struct stat {
	dev_t	st_dev;          // �豸��
	unsigned int	st_ino;          // i �ڵ��
	umode_t	st_mode;         // �ļ�����
	nlink_t	st_nlink;        // �ļ�������
	uid_t	st_uid;          // �ļ��û� id
	gid_t	st_gid;          // �ļ��� id 
	dev_t	st_rdev;         // �豸�ţ��ַ� �豸 ���� ���豸��
	off_t	st_size;         // �ļ��ֽڳ���
	unsigned int	st_atime;        // ������ʱ��
	unsigned int	st_mtime;        // ����޸�ʱ��
	unsigned int	st_ctime;        // i �ڵ�����޸�ʱ��
};



// �ļ� i �ڵ� s_mode �ֶ� 0000 0000 0000 0000
#define S_IFMT  00170000            // �ļ�����λ������ - 1111 0000 0000 0000
#define S_IFLNK	 0120000            // ��������         - 1010 0000 0000 0000
#define S_IFREG  0100000            // �����ļ�         - 1000 0000 0000 0000
#define S_IFBLK  0060000            // ���豸           - 0110 0000 0000 0000
#define S_IFDIR  0040000            // Ŀ¼             - 0100 0000 0000 0000
#define S_IFCHR  0020000            // �ַ��豸         - 0010 0000 0000 0000
#define S_IFIFO  0010000            // �����ܵ�         - 0001 0000 0000 0000

// �ļ�����λ
#define S_ISUID  0004000            // ִ��ʱ�������ļ����û�idΪ�ý��̵��û�id
#define S_ISGID  0002000            // ִ��ʱ��������id
#define S_ISVTX  0001000            // ����Ŀ¼������ɾ����־

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)

// �ļ�����Ȩ��
#define S_IACC  00777             // �ļ�����Ȩ��������
#define S_IRWXU 00700             // ������дִ����ɱ�־           - 111 000 000
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070             // ���Ա��дִ����ɱ�־         - 000 111 000
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007             // �����û���дִ����ɱ�־       - 000 000 111
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001



// ���� file->mode �ĳ���
#define F_M_READ 1           // ...01
#define F_M_WRITE 2          // ...10
#define F_M_WR_MASK 3        // ...11
#endif