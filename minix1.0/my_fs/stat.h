#pragma once
#ifndef STAT_H
#define STAT_H
#include"mix_type.h"

struct stat {
	mix_dev_t	st_dev;          // 设备号
	unsigned int	st_ino;          // i 节点号
	mix_umode_t	st_mode;         // 文件属性
	mix_nlink_t	st_nlink;        // 文件链接数
	mix_uid_t	st_uid;          // 文件用户 id
	mix_gid_t	st_gid;          // 文件组 id 
	mix_dev_t	st_rdev;         // 设备号（字符 设备 或者 块设备）
       	mix_off_t	st_size;         // 文件字节长度
	unsigned int	st_atime;        // 最后访问时间
	unsigned int	st_mtime;        // 最后修改时间
	unsigned int	st_ctime;        // i 节点最后修改时间
};



// 文件 i 节点 s_mode 字段 0000 0000 0000 0000
#define S_IFMT  00170000            // 文件类型位屏蔽码 - 1111 0000 0000 0000
#define S_IFLNK	 0120000            // 符号链接         - 1010 0000 0000 0000
#define S_IFREG  0100000            // 正规文件         - 1000 0000 0000 0000
#define S_IFBLK  0060000            // 块设备           - 0110 0000 0000 0000
#define S_IFDIR  0040000            // 目录             - 0100 0000 0000 0000
#define S_IFCHR  0020000            // 字符设备         - 0010 0000 0000 0000
#define S_IFIFO  0010000            // 命名管道         - 0001 0000 0000 0000

// 文件属性位
#define S_ISUID  0004000            // 执行时，设置文件的用户id为该进程的用户id
#define S_ISGID  0002000            // 执行时，设置组id
#define S_ISVTX  0001000            // 对于目录，受限删除标志

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)

// 文件访问权限
#define S_IACC  00777             // 文件访问权限屏蔽码
#define S_IRWXU 00700             // 宿主读写执行许可标志           - 111 000 000
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070             // 组成员读写执行许可标志         - 000 111 000
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007             // 其他用户读写执行许可标志       - 000 000 111
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001



// 用于 file->mode 的常量
#define F_M_READ 1           // ...01
#define F_M_WRITE 2          // ...10
#define F_M_WR_MASK 3        // ...11
#endif
