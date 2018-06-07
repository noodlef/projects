#include"mix_sys.h"
#include"cmd.h"
#include"stat.h"
#include"file_sys.h"
#include"kernel.h"
#include"mix_erro.h"
#include<string.h>
fn_ptr sys_call_table[] = {
sys_set_char_color ,      sys_set_tty_color,      sys_clear,             sys_mount_cmd, 
sys_umount_cmd,           sys_ls_cmd,             sys_link_cmd,          sys_symlink_cmd,
sys_unlink_cmd,           sys_rmdir_cmd,          sys_mkdir_cmd,         sys_rfile_cmd,
sys_blkfile_cmd,          sys_creat_cmd,          sys_print_inode,       sys_vim_cmd,
sys_format_cmd,           sys_quit_cmd,           sys_cd_cmd,            sys_useradd_cmd,
sys_printer_cmd,          sys_readsuper_cmd
};
int NR_syscalls = 2;

// �����������ɫ���Ǳ���ɫ
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

// ������Ļ�ı���ɫ
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


// ��װ�ļ�ϵͳ
// return : 0 -- �ɹ��� ���򷵻ش�����
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

// ж���ļ�ϵͳ
// para : dev_name -- �豸��
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

// ��ӡĿ¼��
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

// Ϊ�ļ�����һ���ļ���Ŀ¼��
// Ϊ�Ѿ����ڵ��ļ�����һ�������ӣ�Ӳ����
// para : oldname -- ԭ·������ newname -- ��·����
// return �� 0 -- �ɹ��� ���򷵻س�����
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


// ������������
// para : oldname -- ԭ·������ newname -- ��·����
// return �� 0 -- �ɹ��� ���򷵻س�����
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


// ɾ���ļ�����Ӧ��Ŀ¼��
// ������ļ������һ�����ӣ� ��û�н������ڴ򿪸��ļ�����ɾ�����ļ�
// return : 0 -- �ɹ��� ���򷵻س�����
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


// ɾ��һ����Ŀ¼
// para : name -- ·����
// return : 0 -- �ɹ������򷵻ش�����
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


// ����һ��Ŀ¼
// para : pathname -- ·������ mode -- ���Ժ�Ȩ��
// return : 0 -- �ɹ��� ���򷵻س�����
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



// ����һ���豸�����ļ�����ͨ�ļ��ڵ�
// �ú�����������Ϊ filename ���ļ�ϵͳ�ڵ㣬��ͨ�ļ����豸�ļ�
// para : filename -- ·����, mode -- �ļ����ͺͷ�����ɣ�dev -- �豸��
// return �� 0 -- �ɹ������򷵻س�����
extern int sys_mknod(const char * filename, int mode, int dev);

int sys_rfile_cmd()
{
	int filename;
	int mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
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


// ���������ļ�ϵͳ����
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


// ��ȡ�ļ��� i �ڵ���Ϣ�� �������������
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

	M_printf("device :                        %-d -- major(%d) -- minor(%d)\n",
		     statbuf.st_dev, MAJOR(statbuf.st_dev), MINOR(statbuf.st_dev));

	M_printf("inode number :                  %-d\n", statbuf.st_ino);

	get_mode(s_mode, statbuf.st_mode);
	M_printf("mode :                          %-s\n", s_mode);

	if(S_ISBLK(statbuf.st_mode))
	M_printf("device number :                 %-d\n", statbuf.st_rdev);
	else
    M_printf("device number :                 *\n");

	M_printf("links :                         %-d\n", statbuf.st_nlink);
	M_printf("group id :                      %-d\n", statbuf.st_gid);
	M_printf("user id :                       %-d\n", statbuf.st_uid);
	M_printf("size :                          %-d bytes\n", statbuf.st_size);

	M_printf("last access time :              ");
	print_date(statbuf.st_atime);

	M_printf("last modification time :        ");
	print_date(statbuf.st_mtime);

	M_printf("last modification time :        ");
	print_date(statbuf.st_ctime);

	return 0;
}

int sys_vim(const char* filename);
int sys_vim_cmd(){
	int filename, error;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;

	error = sys_vim(buf + filename);
	return error;
}

extern int device_format(const char* filename);
int sys_format_cmd(){
	int filename, error;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;

	error = device_format(buf + filename);
	return error;
}

extern int sys_save_sbuf(char* pathname, unsigned int minor);
extern int close_noodle_file();
int sys_quit_cmd(){
	int error, minor = 0;
	
	// ������Ļ����
	if ((error = sys_save_sbuf("/etc/log_in.sh", 0)) < 0)
		return error;
	sys_sync();
	clc_win(minor);
	iput(current->pwd);
	current->pwd = current->root;
	current->pwd->i_count++;
	//close_noodle_file();
	mix_cmd();
	return 1;
}

// 
extern int sys_cd(const char* filename);
int sys_cd_cmd(){
	int filename, error;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;

	error = sys_cd(buf + filename);
	return error;
}

// ������û�
extern int sys_useradd();
int sys_useradd_cmd(){
	return sys_useradd();
}

// ��ӡ��
extern int sys_printer(const char * filename, const char* mode);
int sys_printer_cmd(){
	
	int filename, mode;
	char* buf = MIX_STACK;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	mode = S_TOP + 1;

	--S_TOP;
	while (MIX_STACK[--S_TOP] != '\0');
	filename = S_TOP + 1;
	return sys_printer(buf + filename, buf + mode);
}

int sys_readsuper();
int sys_readsuper_cmd(){
	return sys_readsuper();
}
