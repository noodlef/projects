#include"file_sys.h"
#include"kernel.h"
#include"stat.h"
#include"mix_erro.h"
#define put_fs_long(fd,addr) ((*(addr)) = fd)
#define FIONREAD 1


// 管道读操作
// para : inode -- 管道 i 节点， buf -- 缓冲区， count -- 读的字节数
// return : 返回读到的字节个数， 出错返回出错码
int read_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, read = 0;

	while (count>0) {
		// 取管道中的数据长度
		while (!(size = PIPE_SIZE(*inode))) {
			// 如果管道中没有数据
			//wake_up(&PIPE_WRITE_WAIT(*inode));
			if (inode->i_count != 2) /* are there any writers? */
			{
				printk("read_pipe : no writer");
				return read;
			}
			//if (current->signal & ~current->blocked)
			//	return read ? read : -ERESTARTSYS;
			// interruptible_sleep_on(&PIPE_READ_WAIT(*inode));
			printk("read_pipe : waiting for writers to write");
			return read;
		}
		chars = PAGE_SIZE - PIPE_TAIL(*inode);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		read += chars;
		size = PIPE_TAIL(*inode);
		PIPE_TAIL(*inode) += chars;
		PIPE_TAIL(*inode) &= (PAGE_SIZE - 1);
		while (chars-->0)
			put_fs_byte(((char *)inode->i_size)[size++], buf++);
	}
	//wake_up(&PIPE_WRITE_WAIT(*inode));
	return read;
}


// 管道写操作
// para : inode -- 管道 i 节点， buf -- 缓冲区， count -- 写的字节数
// return : 返回写到的字节个数， 出错返回出错码
int write_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, written = 0;

	while (count>0) {
		// 如果当前管道已满
		while (!(size = (PAGE_SIZE - 1) - PIPE_SIZE(*inode))) {
			//wake_up(&PIPE_READ_WAIT(*inode));
			if (inode->i_count != 2) { /* no readers */
				//current->signal |= (1 << (SIGPIPE - 1));
				printk("write_pipe : no reader");
				return written ? written : -1;
			}
			//sleep_on(&PIPE_WRITE_WAIT(*inode));
			printk("write_pipe : waiting for readers to read");
			return written;
		}
		chars = PAGE_SIZE - PIPE_HEAD(*inode);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		written += chars;
		size = PIPE_HEAD(*inode);
		PIPE_HEAD(*inode) += chars;
		PIPE_HEAD(*inode) &= (PAGE_SIZE - 1);
		while (chars-->0)
			((char *)inode->i_size)[size++] = get_fs_byte(buf++);
	}
	//wake_up(&PIPE_READ_WAIT(*inode));
	return written;
}


// 创建管道系统调用
// para : fildes -- 文件句柄数组
int sys_pipe(unsigned int * fildes)
{
	struct m_inode * inode;
	// 文件表项，一个用于读，一个用于写
	struct file * f[2];
	int fd[2];
	int i, j;
	// 先从文件表中取两个空表项
	j = 0;
	for (i = 0; j<2 && i<NR_FILE; i++)
		if (!file_table[i].f_count)
			(f[j++] = i + file_table)->f_count++;
	if (j == 1)
		f[0]->f_count = 0;
	if (j<2)
		return -ERROR;
	j = 0;
	// 从进程的文件描述符数组中找出两个空闲表项
	for (i = 0; j<2 && i<NR_OPEN; i++)
		if (!current->filp[i]) {
			current->filp[fd[j] = i] = f[j];
			j++;
		}
	if (j == 1)
		current->filp[fd[0]] = NULL;
	if (j<2) {
		f[0]->f_count = f[1]->f_count = 0;
		return -ERROR;
	}
	// 获取管道 i 节点
	if (!(inode = get_pipe_inode())) {
		current->filp[fd[0]] =
			current->filp[fd[1]] = NULL;
		f[0]->f_count = f[1]->f_count = 0;
		return -ERROR;
	}
	f[0]->f_inode = f[1]->f_inode = inode;
	f[0]->f_pos = f[1]->f_pos = 0;
	f[0]->f_mode = F_M_READ;		/* read */
	f[1]->f_mode = F_M_WRITE;		/* write */
	put_fs_long(fd[0], 0 + fildes);
	put_fs_long(fd[1], 1 + fildes);
	return 0;
}



int pipe_ioctl(struct m_inode *pino, int cmd, int arg)
{
	switch (cmd) {
	case FIONREAD:
		// verify_area((void *)arg, 4);
		put_fs_long(PIPE_SIZE(*pino), (unsigned long *)arg);
		return 0;
	default:
		return -EINVAL;
	}
}
