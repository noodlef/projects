#include"file_sys.h"
#include"fcntl.h"
#include"mix_erro.h"
#include"kernel.h"
/*
* 本文件实现文件系统调用fcntl()
* 和两个文件句柄复制系统调用 dup(), dup2()
*/


extern int sys_close(unsigned int fd);

// 复制文件描述符
// arg -- 指定新文件描述符的最小值
static int dupfd(unsigned int fd, unsigned int arg)
{
	if (fd >= NR_OPEN || !current->filp[fd])
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;
	while (arg < NR_OPEN)
		if (current->filp[arg])
			arg++;
		else
			break;
	if (arg >= NR_OPEN)
		return -EMFILE;
	//current->close_on_exec &= ~(1 << arg);
	(current->filp[arg] = current->filp[fd])->f_count++;
	return arg;
}


// 复制文件描述符
int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	sys_close(newfd);
	return dupfd(oldfd, newfd);
}

// 复制文件描述符
int sys_dup(unsigned int fildes)
{
	return dupfd(fildes, 0);
}



// 文件控制系统调用

int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct file * filp;

	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	switch (cmd) {
	// 复制文件句柄
	case F_DUPFD:
		return dupfd(fd, arg);
	case F_GETFD:
		return 0;//(current->close_on_exec >> fd) & 1;
	case F_SETFD:
		if (arg & 1)
			;//current->close_on_exec |= (1 << fd);
		else
			;//current->close_on_exec &= ~(1 << fd);
		return 0;
	// 取文件状态标志和访问模式
	case F_GETFL:
		return filp->f_flags;
    // 设置文件状态和访问模式
	case F_SETFL:
		filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
		filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
		return 0;
	case F_GETLK:	case F_SETLK:	case F_SETLKW:
		return -1;
	default:
		return -1;
	}
}