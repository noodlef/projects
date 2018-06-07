#include"file_sys.h"
#include"stat.h"
#include"mix_erro.h"
#include"kernel.h"
/*
 * 本文件实现提取文件状态信息的系统调用
 *   stat() 和 fstat()
 */

static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	//verify_area(statbuf, sizeof(struct stat));
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_num;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlinks;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_zone[0];
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	for (i = 0; i < sizeof(tmp); i++)
		*(i + (char *)statbuf) = ((char *)&tmp)[i];
		//put_fs_byte(((char *)&tmp)[i], i + (char *)statbuf);
}

// 获取文件的 i 节点信息
int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	cp_stat(inode, statbuf);
	iput(inode);
	return 0;
}


// 获取文件的 i 节点信息， 不跟随符号链接
int sys_lstat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode = lnamei(filename)))
		return -ENOENT;
	cp_stat(inode, statbuf);
	iput(inode);
	return 0;
}

// 根据文件描述符获取文件的 i 节点信息
int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	if (fd >= NR_OPEN || !(f = current->filp[fd]) || !(inode = f->f_inode))
		return -EBADF;
	cp_stat(inode, statbuf);
	return 0;
}

// 读符号链接系统调用, 不复制最后的空字符
int sys_readlink(const char * path, char * buf, int bufsiz)
{
	struct m_inode * inode;
	struct buffer_head * bh;
	int i;
	char c;

	if (bufsiz <= 0)
		return -EBADF;
	if (bufsiz > 1023)
		bufsiz = 1023;
	// verify_area(buf, bufsiz);
	if (!(inode = lnamei(path)))
		return -ENOENT;
	if (!S_ISLNK(inode->i_mode))
		return -EBADF;
	if (inode->i_zone[0])
		bh = bread(inode->i_dev, inode->i_zone[0]);
	else
		bh = NULL;
	iput(inode);
	if (!bh)
		return 0;
	i = 0;
	while (i<bufsiz && (c = bh->b_data[i])) {
		i++;
		put_fs_byte(c, buf++);
	}
	brelse(bh);
	return i;
}