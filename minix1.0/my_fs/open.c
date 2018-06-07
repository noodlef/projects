#include"kernel.h"
#include"file_sys.h"
#include"mix_erro.h"
#include"utime.h"
#include"stat.h"
#include"fcntl.h"
#include"string.h"
// 打开文件操作

#define get_fs_long(addr) (*(addr))
// 未实现 ， 取 文件系统的信息
int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;
}

// 设置文件的访问和修改时间
// para : filename -- 路径名， times -- 修改文件的时间
int sys_utime(char * filename, struct utimbuf * times)
{
	struct m_inode * inode;
	long actime, modtime;

	if (!(inode = namei(filename)))
		return -ENOENT;
	if (times) {
		actime = get_fs_long((unsigned long *)&times->actime);
		modtime = get_fs_long((unsigned long *)&times->modtime);
	}
	else
		actime = modtime = CURRENT_TIME;
	inode->i_atime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

// 检查文件访问权限
// para : mode -- 要检查的权限，可为 可读-1， 可写-2， 可执行-4， 文件是否存在
// return ：　如果访问许可就返回　０　，　否则返回出错码
int sys_access(const char * filename, int mode)
{
	struct m_inode * inode;
	int res, i_mode;

	mode &= 0007;
	if (!(inode = namei(filename)))
		return -EACCES;
	i_mode = res = inode->i_mode & S_IACC;
	if (current->uid == inode->i_uid)
		res >>= 6;
	else if (current->gid == inode->i_gid)
		res >>= 3;
	iput(inode);
	if ((res & 0007 & mode) == mode)
		return 0;
	/*
	* XXX we are doing this test last because we really should be
	* swapping the effective with the real user id (temporarily),
	* and then calling suser() routine.  If we do call the
	* suser() routine, it needs to be called last.
	*/
	if ((!current->uid) &&
		(!(mode & 1) || (i_mode & 0111)))
		return 0;
	return -EACCES;
}


// 改变当前工作目录
int sys_chdir(const char * filename)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	iput(current->pwd);
	current->pwd = inode;
	return (0);
}

// 修改文件名
int sys_chname(const char* old_name, const char* new_name) {

	int error_code = 1, tmp;

	if (!strcmp(old_name, new_name))
		return error_code;
	// 建立一个硬连接
	if((tmp = sys_link(old_name, new_name)) < 0)
		return (error_code = tmp);
   // 删除原来的连接
	if ((tmp = sys_unlink(old_name)) < 0)
		error_code = tmp;
	return error_code;
}


// 改变进程的当前 根目录
int sys_chroot(const char * filename)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	iput(current->root);
	current->root = inode;
	return (0);
}


// 修改文件属性
int sys_chmod(const char * filename, int mode)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	if ((current->euid != inode->i_uid) && !suser()) {
		iput(inode);
		return -EACCES;
	}
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

// 修改文件宿主系统调用
int sys_chown(const char * filename, int uid, int gid)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	if (!suser()) {
		iput(inode);
		return -EACCES;
	}
	inode->i_uid = uid;
	inode->i_gid = gid;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}


static int check_char_dev(struct m_inode * inode, int dev, int flag)
{
	return 0;
}

// 打开文件系统调用
// para : flag -- o_rdonly, o_wronly, o_rdwr, o_creat,
//                o_excl(被创建文件必须不存在), o_append(文件尾添加)
// mode -- 文件属性
int sys_open(const char * filename, int flag, int mode)
{
	struct m_inode * inode;
	struct file * f;
	int i, fd;
	// 将用户设置的文件许可模式与进程的模式屏蔽码相与
	mode &= 0777 & ~current->umask;
	// 获取一个文件描述符
	for (fd = 0; fd<NR_OPEN; fd++)
		if (!current->filp[fd])
			break;
	if (fd >= NR_OPEN)
		return -EINVAL;

	//current->close_on_exec &= ~(1 << fd);
	// 在文件表中找到一个空闲表项
	f = 0 + file_table;
	for (i = 0; i<NR_FILE; i++, f++)
		if (!f->f_count) break;
	if (i >= NR_FILE)
		return -EINVAL;
	(current->filp[fd] = f)->f_count++;

	// 打开文件
	if ((i = open_namei(filename, flag, mode, &inode))<0) {
		current->filp[fd] = NULL;
		f->f_count = 0;
		return i;
	}

	// 如果是字符设备文件
	if (S_ISCHR(inode->i_mode))
		if (check_char_dev(inode, inode->i_zone[0], flag)) {
			iput(inode);
			current->filp[fd] = NULL;
			f->f_count = 0;
			return -EAGAIN;
		}
	

	// 如果是块设备文件, 需要检查盘片是否被更换
	if (S_ISBLK(inode->i_mode))
		check_disk_change(inode->i_zone[0]);                      // 注意 ： check_disk_change() 没有完全实现，还要进行修改。 

	// 设置文件表项
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return (fd);
}

// 创建文件系统调用
int sys_creat(const char * pathname, int mode)
{
	return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}

// 关闭文件
int sys_close(unsigned int fd)
{
	struct file * filp;

	if (fd >= NR_OPEN)
		return -EINVAL;
	//current->close_on_exec &= ~(1 << fd);
	if (!(filp = current->filp[fd]))
		return -EINVAL;
	current->filp[fd] = NULL;
	if (filp->f_count == 0)
		panic("Close: file count is 0");
	if (--filp->f_count)
		return (0);
	iput(filp->f_inode);
	return (0);
}