#pragma once
#ifndef MIX_ERRO_H
#define MIX_ERRO_H


#define ERROR		99             // 一般错误
#define EPERM		 1             // 操作没有许可
#define ENOENT		 2             // 文件或目录不存在
#define ESRCH		 3
#define EINTR		 4
#define EIO		     5
#define ENXIO		 6
#define E2BIG		 7
#define ENOEXEC		 8
#define EBADF		 9
#define ECHILD		10
#define EAGAIN		11
#define ENOMEM		12
#define EACCES		13                   // 没有许可权限
#define EFAULT		14
#define ENOTBLK		15
#define EBUSY		16
#define EEXIST		17                   // 文件已存在
#define EXDEV		18                   // 
#define ENODEV		19
#define ENOTDIR		20
#define EISDIR		21                   // 是目录文件
#define EINVAL		22
#define ENFILE		23
#define EMFILE		24
#define ENOTTY		25
#define ETXTBSY		26
#define EFBIG		27
#define ENOSPC		28                   // 设备已经没有空间
#define ESPIPE		29
#define EROFS		30
#define EMLINK		31
#define EPIPE		32
#define EDOM		33
#define ERANGE		34
#define EDEADLK		35
#define ENAMETOOLONG	36
#define ENOLCK		37
#define ENOSYS		38
#define ENOTEMPTY	39

#define WARGUMENT   60
void err(unsigned minor, int error);
#endif
