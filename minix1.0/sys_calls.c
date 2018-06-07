#include"mix_sys.h"
#include"cmd.h"
#include"stat.h"
#include"file_sys.h"
#include"kernel.h"
#include"mix_erro.h"
fn_ptr sys_call_table[] = {
sys_set_char_color ,      sys_set_tty_color,      sys_clear,             sys_mount_cmd, 
sys_umount_cmd,           sys_ls_cmd,             sys_link_cmd,          sys_symlink_cmd,
sys_unlink_cmd,           sys_rmdir_cmd,          sys_mkdir_cmd,         sys_rfile_cmd,
sys_blkfile_cmd,          sys_creat_cmd,          sys_print_inode
};
int NR_syscalls = 2;

// 设置字体的颜色或是背景色
int sys_set_char_color()
{
	int minor = 0;
	struct tty_window* tty = tty_dev[minor];
	enum CF_color f_color;
	enum CB_color b_color;
	int len_f = 0, len_b = 0;
	int f_pos, b_pos;
	char* buf = MIX_STACK;
	--S_TOP;
	++len_b;
	while (MIX_STACK[--S_TOP] != '\0')
		++len_b;
	b_pos = S_TOP + 1;

	--S_TOP;
	++len_f;
	while (MIX_STACK[--S_TOP] != '\0')
		++len_f;
	f_pos = S_TOP + 1;

	if (!strcmp(buf + f_pos, "red"))
		f_color = CFC_Red;
	else if (!strcmp(buf + f_pos, "green"))
		f_color = CFC_Green;
	else if (!strcmp(buf + f_pos, "blue"))
		f_color = CFC_Blue;
	else if (!strcmp(buf + f_pos, "yellow"))
		f_color = CFC_Yellow;
	else if (!strcmp(buf + f_pos, "purple"))
		f_color = CFC_Purple;
	else if (!strcmp(buf + f_pos, "cyan"))
		f_color = CFC_Cyan;
	else if (!strcmp(buf + f_pos, "gray"))
		f_color = CFC_Gray;
	else if (!strcmp(buf + f_pos, "white"))
		f_color = CFC_White;
	else if (!strcmp(buf + f_pos, "highwhite"))
		f_color = CFC_HighWhite;
	else if (!strcmp(buf + f_pos, "black"))
		f_color = CFC_Black;
	else
		f_color = tty->f_color;
	set_char_fcolor(minor, f_color);

	if (!strcmp(buf + b_pos, "red"))
		b_color = CBC_Red;
	else if (!strcmp(buf + b_pos, "green"))
		b_color = CBC_Green;
	else if (!strcmp(buf + b_pos, "blue"))
		b_color = CBC_Blue;
	else if (!strcmp(buf + b_pos, "yellow"))
		b_color = CBC_Yellow;
	else if (!strcmp(buf + b_pos, "purple"))
		b_color = CBC_Purple;
	else if (!strcmp(buf + b_pos, "cyan"))
		b_color = CBC_Cyan;
	else if (!strcmp(buf + b_pos, "white"))
		b_color = CBC_White;
	else if (!strcmp(buf + b_pos, "highwhite"))
		b_color = CBC_HighWhite;
	else if (!strcmp(buf + b_pos, "black"))
		b_color = CBC_Black;
	else
		return -1;
	set_char_bcolor(minor, b_color);
	return 0;
}

// 设置屏幕的背景色
int sys_set_tty_color()
{
	int minor = 0;
	struct tty_window* tty = tty_dev[minor];
	enum CB_color b_color;
	int len_b = 0;
	int b_pos;
	char* buf = MIX_STACK;
	--S_TOP;
	++len_b;
	while (MIX_STACK[--S_TOP] != '\0')
		++len_b;
	b_pos = S_TOP + 1;

	if (!strcmp(buf + b_pos, "red"))
		b_color = CBC_Red;
	else if (!strcmp(buf + b_pos, "green"))
		b_color = CBC_Green;
	else if (!strcmp(buf + b_pos, "blue"))
		b_color = CBC_Blue;
	else if (!strcmp(buf + b_pos, "yellow"))
		b_color = CBC_Yellow;
	else if (!strcmp(buf + b_pos, "purple"))
		b_color = CBC_Purple;
	else if (!strcmp(buf + b_pos, "cyan"))
		b_color = CBC_Cyan;
	else if (!strcmp(buf + b_pos, "white"))
		b_color = CBC_White;
	else if (!strcmp(buf + b_pos, "highwhite"))
		b_color = CBC_HighWhite;
	else if (!strcmp(buf + b_pos, "black"))
		b_color = CBC_Black;
	else
		return -WARGUMENT;
	set_tty_color(minor, b_color);
	s_flush(tty);
	return 0;
}

int sys_clear()
{
	int minor = 0;
	clc_win(minor);
	return 0;
}


// 安装文件系统
// return : 0 -- 成功， 否则返回错误码
extern int sys_mount(char * dev_name, char * dir_name, int rw_flag);

int sys_mount_cmd()
{
	int dev_pos, dir_pos;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	dir_pos = S_TOP + 1;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	dev_pos = S_TOP + 1;

	return sys_mount(buf + dev_pos, buf + dir_pos, 0);
}

// 卸载文件系统
// para : dev_name -- 设备名
extern int sys_umount(char * dev_name);
 
int  sys_umount_cmd()
{
	int dev_pos;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	dev_pos = S_TOP + 1;

	return sys_umount(buf + dev_pos);
}

extern void sys_ls(const char* pathname, struct m_inode* base);

// 打印目录项
int sys_ls_cmd()
{
	int pathname;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	pathname = S_TOP + 1;
	sys_ls(buf + pathname, 0);
	return 0;
}

// 为文件建立一个文件名目录项
// 为已经存在的文件建立一个新链接，硬链接
// para : oldname -- 原路径名， newname -- 新路径名
// return ： 0 -- 成功， 否则返回出错码
extern int sys_link(const char * oldname, const char * newname);

int sys_link_cmd()
{
	int oldname, newname;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	newname = S_TOP + 1;
	
	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
    oldname = S_TOP + 1;

	return sys_link(buf + oldname, buf + newname);
}


// 建立符号链接
// para : oldname -- 原路径名， newname -- 新路径名
// return ： 0 -- 成功， 否则返回出错码
extern int sys_symlink(const char * oldname, const char * newname);

int sys_symlink_cmd()
{
	int oldname, newname;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	newname = S_TOP + 1;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	oldname = S_TOP + 1;

	return sys_symlink(buf+ oldname, buf + newname);
}


// 删除文件名对应的目录项
// 如果该文件是最后一个链接， 且没有进程正在打开该文件，则删除该文件
// return : 0 -- 成功， 否则返回出错码
extern int sys_unlink(const char * name);

int sys_unlink_cmd()
{
	int name;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	name = S_TOP + 1;

	return sys_unlink(buf + name);
}


// 删除一个空目录
// para : name -- 路径名
// return : 0 -- 成功，否则返回错误码
extern int sys_rmdir(const char * name);

int sys_rmdir_cmd()
{
	int name;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	name = S_TOP + 1;

	return sys_rmdir(buf + name);
}


// 创建一个目录
// para : pathname -- 路径名， mode -- 属性和权限
// return : 0 -- 成功， 否则返回出错码
extern int sys_mkdir(const char * pathname, int mode);

int sys_mkdir_cmd()
{
	int pathname;
	int mode = 0777;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	pathname = S_TOP + 1;



	return sys_mkdir(buf + pathname, mode);
}



// 创建一个设备特殊文件或普通文件节点
// 该函数创建名称为 filename 的文件系统节点，普通文件，设备文件
// para : filename -- 路径名, mode -- 文件类型和访问许可，dev -- 设备号
// return ： 0 -- 成功，否则返回出错码
extern int sys_mknod(const char * filename, int mode, int dev);

int sys_rfile_cmd()
{
	int filename;
	int mode = S_IRWXU | S_IRWXG | S_IRWXO;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;



	return sys_mknod(buf + filename, mode, 0);
}

int sys_blkfile_cmd()
{
	int filename, dev;
	int mode = S_IFBLK | S_IRWXU | S_IRWXG | S_IRWXO;
	char* buf = MIX_STACK;


	dev = *((int*)(buf - sizeof(int)));
	if (MAJOR(dev) != 2 && MAJOR(dev) != 3)
		return -ERROR;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;

	return sys_mknod(buf + filename, mode, dev);
}


// 创建文件系统调用
extern int sys_creat(const char * pathname, int mode);

int sys_creat_cmd()
{
	int filename;
	int mode = S_IRWXU | S_IRWXG | S_IRWXO;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;



	return sys_creat(buf + filename, mode);
}


// 获取文件的 i 节点信息， 不跟随符号链接
extern int sys_lstat(char * filename, struct stat * statbuf);
static int get_mode(char * buf, int mode)
{
	int i = 0;
	switch(mode & S_IFMT) 
	{
		case S_IFLNK:
			buf[i++] = 'L';
			break;
		case S_IFREG:
			buf[i++] = 'R';
			break;
		case S_IFBLK: 
			buf[i++] = 'B';
			break;
		case S_IFDIR: 
			buf[i++] = 'D';
			break;
		case S_IFCHR:  
			buf[i++] = 'C';
			break;
		case S_IFIFO:  
			buf[i++] = 'P';
			break;
		default:
			buf[i++] = '*';
			break;
	}
	buf[i++] = '-';
	//
	if (mode & S_IRUSR)
		buf[i++] = 'r';
	else
		buf[i++] = '*';

	if (mode & S_IWUSR)
		buf[i++] = 'w';
	else
		buf[i++] = '*';

	if (mode & S_IXUSR)
		buf[i++] = 'x';
	else
		buf[i++] = '*';
	buf[i++] = '-';

	//
	if (mode & S_IRGRP)
		buf[i++] = 'r';
	else
		buf[i++] = '*';

	if (mode & S_IWGRP)
		buf[i++] = 'w';
	else
		buf[i++] = '*';

	if (mode & S_IXGRP)
		buf[i++] = 'x';
	else
		buf[i++] = '*';
	buf[i++] = '-';

	//
	if (mode & S_IROTH)
		buf[i++] = 'r';
	else
		buf[i++] = '*';

	if (mode & S_IWOTH)
		buf[i++] = 'w';
	else
		buf[i++] = '*';

	if (mode & S_IXOTH)
		buf[i++] = 'x';
	else
		buf[i++] = '*';
	buf[i++] = '\0';
	return 0;
}

int sys_print_inode()
{
	struct stat statbuf;
	int filename, error;
	char* buf = MIX_STACK;
	char s_mode[20];

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;

	if ((error = sys_lstat(buf + filename, &statbuf)) < 0)
		return error;

	M_printf("device :                                           %-d -- major(%d) -- minor(%d)\n",
		     statbuf.st_dev, MAJOR(statbuf.st_dev), MINOR(statbuf.st_dev));

	M_printf("inode number :                                     %-d\n", statbuf.st_ino);

	get_mode(s_mode, statbuf.st_mode);
	M_printf("mode :                                             %-s\n", s_mode);

	if(S_ISBLK(statbuf.st_mode))
	M_printf("block number :                                     %-d\n", statbuf.st_rdev);
	else
    M_printf("block number :                                     *\n");

	M_printf("links :                                            %-d\n", statbuf.st_nlink);
	M_printf("group id :                                         %-d\n", statbuf.st_gid);
	M_printf("user id :                                          %-d\n", statbuf.st_uid);
	M_printf("size :                                             %-d bytes\n", statbuf.st_size);

	M_printf("last access time of file :                         ");
	print_date(statbuf.st_atime);

	M_printf("last modification time of file :                   ");
	print_date(statbuf.st_mtime);

	M_printf("last modification time of inode :                  ");
	print_date(statbuf.st_ctime);

	return 0;
}