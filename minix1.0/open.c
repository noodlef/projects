#include"kernel.h"
#include"file_sys.h"
#include"mix_erro.h"
#include"utime.h"
#include"stat.h"
#include"fcntl.h"
#include"string.h"
// ���ļ�����

#define get_fs_long(addr) (*(addr))
// δʵ�� �� ȡ �ļ�ϵͳ����Ϣ
int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;
}

// �����ļ��ķ��ʺ��޸�ʱ��
// para : filename -- ·������ times -- �޸��ļ���ʱ��
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

// ����ļ�����Ȩ��
// para : mode -- Ҫ����Ȩ�ޣ���Ϊ �ɶ�-1�� ��д-2�� ��ִ��-4�� �ļ��Ƿ����
// return �������������ɾͷ��ء������������򷵻س�����
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


// �ı䵱ǰ����Ŀ¼
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

// �޸��ļ���
int sys_chname(const char* old_name, const char* new_name) {

	int error_code = 1, tmp;

	if (!strcmp(old_name, new_name))
		return error_code;
	// ����һ��Ӳ����
	if((tmp = sys_link(old_name, new_name)) < 0)
		return (error_code = tmp);
   // ɾ��ԭ��������
	if ((tmp = sys_unlink(old_name)) < 0)
		error_code = tmp;
	return error_code;
}


// �ı���̵ĵ�ǰ ��Ŀ¼
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


// �޸��ļ�����
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

// �޸��ļ�����ϵͳ����
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

// ���ļ�ϵͳ����
// para : flag -- o_rdonly, o_wronly, o_rdwr, o_creat,
//                o_excl(�������ļ����벻����), o_append(�ļ�β���)
// mode -- �ļ�����
int sys_open(const char * filename, int flag, int mode)
{
	struct m_inode * inode;
	struct file * f;
	int i, fd;
	// ���û����õ��ļ����ģʽ����̵�ģʽ����������
	mode &= 0777 & ~current->umask;
	// ��ȡһ���ļ�������
	for (fd = 0; fd<NR_OPEN; fd++)
		if (!current->filp[fd])
			break;
	if (fd >= NR_OPEN)
		return -EINVAL;

	//current->close_on_exec &= ~(1 << fd);
	// ���ļ������ҵ�һ�����б���
	f = 0 + file_table;
	for (i = 0; i<NR_FILE; i++, f++)
		if (!f->f_count) break;
	if (i >= NR_FILE)
		return -EINVAL;
	(current->filp[fd] = f)->f_count++;

	// ���ļ�
	if ((i = open_namei(filename, flag, mode, &inode))<0) {
		current->filp[fd] = NULL;
		f->f_count = 0;
		return i;
	}

	// ������ַ��豸�ļ�
	if (S_ISCHR(inode->i_mode))
		if (check_char_dev(inode, inode->i_zone[0], flag)) {
			iput(inode);
			current->filp[fd] = NULL;
			f->f_count = 0;
			return -EAGAIN;
		}
	

	// ����ǿ��豸�ļ�, ��Ҫ�����Ƭ�Ƿ񱻸���
	if (S_ISBLK(inode->i_mode))
		check_disk_change(inode->i_zone[0]);                      // ע�� �� check_disk_change() û����ȫʵ�֣���Ҫ�����޸ġ� 

	// �����ļ�����
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return (fd);
}

// �����ļ�ϵͳ����
int sys_creat(const char * pathname, int mode)
{
	return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}

// �ر��ļ�
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