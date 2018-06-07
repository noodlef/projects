#include"file_sys.h"
#include"mix_erro.h"
#include"stat.h"
#include"fcntl.h"
#include"kernel.h"


#define IS_SEEKABLE(major) (seekable[major])
// 设备文件指针是否可定位数组
int seekable[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };


extern int rw_char(int rw, int dev, char * buf, int count, mix_off_t * pos);
extern int read_pipe(struct m_inode * inode, char * buf, int count);
extern int write_pipe(struct m_inode * inode, char * buf, int count);
extern int block_read(int dev, mix_off_t * pos, char * buf, int count);
extern int block_write(int dev, mix_off_t * pos, char * buf, int count);
extern int file_read(struct m_inode * inode, struct file * filp,
	char * buf, int count);
extern int file_write(struct m_inode * inode, struct file * filp,
	char * buf, int count);

// 系统调用 重定位文件读写指针
// para : fd -- 文件描述符， offset -- 新的文件读写指针偏移量，
//        origin -- 偏移的起始位置 (可为 SEEK_SET-文件起始， SEEK_CUR -- 文件当前读写位置， SEEK_END -- 文件尾)
// return ： 返回文件重定位后的读写位置
int sys_lseek(unsigned int fd, mix_off_t offset, int origin)
{
	struct file * file;
	int tmp;
	// 参数有效性检测
	if (fd >= NR_OPEN || !(file = current->filp[fd]) || !(file->f_inode)
		|| !IS_SEEKABLE(MAJOR(file->f_inode->i_dev))) // IS_SEEKABLE( ) 文件指针可定位
		return -EBADF;
	// 如果是管道
	if (file->f_inode->i_pipe)
		return -ESPIPE;
	switch (origin) {
	// 以文件起始处为原点设置文件指针
	case SEEK_SET:
		if (offset<0) return -EINVAL;
		file->f_pos = offset;
		break;
	// 以文件当前位置为原点设置文件指针
	case SEEK_CUR:
		if (file->f_pos + offset<0) return -EINVAL;
		file->f_pos += offset;
		break;
	// 以文件末尾为原点设置文件指针
	case SEEK_END:
		if ((tmp = file->f_inode->i_size + offset) < 0)
			return -EINVAL;
		file->f_pos = tmp;
		break;
	default:
		return -EINVAL;
	}
	return file->f_pos;
}

// 读文件系统调用
// para : fd -- 文件句柄， buf -- 缓冲区， count -- 要读的字节数
// return : 返回已读的字节数 或 出错码
int sys_read(unsigned int fd, char * buf, int count)
{
	struct file * file;
	struct m_inode * inode;
	// 参数有效性检测
	if (fd >= NR_OPEN || count<0 || !(file = current->filp[fd]))
		return -EINVAL;
	if ((file->f_flags & O_ACCMODE) == O_WRONLY)
		return -EPERM;
	if (!count)
		return 0;
	// 缓冲区大小 检测
	// verify_area(buf, count);
	inode = file->f_inode;
	// 管道文件
	if (inode->i_pipe)
		return (file->f_mode & F_M_WR_MASK & F_M_READ) ? read_pipe(inode, buf, count) : -EIO;
	// 字符设备文件
	if (S_ISCHR(inode->i_mode))
		return rw_char(READ, inode->i_zone[0], buf, count, &file->f_pos);
	// 块设备文件
	if (S_ISBLK(inode->i_mode))
		return block_read(inode->i_zone[0], &file->f_pos, buf, count);
	// 正规文件， 目录文件， 符号链接
	if (S_ISDIR(inode->i_mode) || S_ISREG(inode->i_mode) || S_ISLNK(inode->i_mode)) {
		if (count + file->f_pos > inode->i_size)
			count = inode->i_size - file->f_pos;
		if (count <= 0)
			return 0;
		return file_read(inode, file, buf, count);
	}
	printk("(Read)inode->i_mode=%06o\n", inode->i_mode);
	return -EINVAL;
}

// 写文件系统调用
// para : fd -- 文件句柄， buf -- 缓冲区， count -- 要读的字节数
// return : 返回已写的字节数 或 出错码
int sys_write(unsigned int fd, char * buf, int count)
{
	struct file * file;
	struct m_inode * inode;
	// 参数有效性检测
	if (fd >= NR_OPEN || count <0 || !(file = current->filp[fd]))
		return -EINVAL;
	if ((file->f_flags & O_ACCMODE) == O_RDONLY)
		return -EPERM;
	if (!count)
		return 0;
	inode = file->f_inode;
	// 管道文件
	if (inode->i_pipe)
		return (file->f_mode & F_M_WR_MASK & F_M_WRITE) ? write_pipe(inode, buf, count) : -EIO;
	// 字符设备文件
	if (S_ISCHR(inode->i_mode))
		return rw_char(WRITE, inode->i_zone[0], buf, count, &file->f_pos);
	// 块设备文件
	if (S_ISBLK(inode->i_mode))
		return block_write(inode->i_zone[0], &file->f_pos, buf, count);
    // 正规文件
	if (S_ISREG(inode->i_mode))
		return file_write(inode, file, buf, count);
	printk("(Write)inode->i_mode=%06o\n", inode->i_mode);
	return -EINVAL;
}
