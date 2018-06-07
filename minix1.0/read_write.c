#include"file_sys.h"
#include"mix_erro.h"
#include"stat.h"
#include"fcntl.h"
#include"kernel.h"


#define IS_SEEKABLE(major) (seekable[major])
// �豸�ļ�ָ���Ƿ�ɶ�λ����
int seekable[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };


extern int rw_char(int rw, int dev, char * buf, int count, off_t * pos);
extern int read_pipe(struct m_inode * inode, char * buf, int count);
extern int write_pipe(struct m_inode * inode, char * buf, int count);
extern int block_read(int dev, off_t * pos, char * buf, int count);
extern int block_write(int dev, off_t * pos, char * buf, int count);
extern int file_read(struct m_inode * inode, struct file * filp,
	char * buf, int count);
extern int file_write(struct m_inode * inode, struct file * filp,
	char * buf, int count);

// ϵͳ���� �ض�λ�ļ���дָ��
// para : fd -- �ļ��������� offset -- �µ��ļ���дָ��ƫ������
//        origin -- ƫ�Ƶ���ʼλ�� (��Ϊ SEEK_SET-�ļ���ʼ�� SEEK_CUR -- �ļ���ǰ��дλ�ã� SEEK_END -- �ļ�β)
// return �� �����ļ��ض�λ��Ķ�дλ��
int sys_lseek(unsigned int fd, off_t offset, int origin)
{
	struct file * file;
	int tmp;
	// ������Ч�Լ��
	if (fd >= NR_OPEN || !(file = current->filp[fd]) || !(file->f_inode)
		|| !IS_SEEKABLE(MAJOR(file->f_inode->i_dev))) // IS_SEEKABLE( ) �ļ�ָ��ɶ�λ
		return -EBADF;
	// ����ǹܵ�
	if (file->f_inode->i_pipe)
		return -ESPIPE;
	switch (origin) {
	// ���ļ���ʼ��Ϊԭ�������ļ�ָ��
	case SEEK_SET:
		if (offset<0) return -EINVAL;
		file->f_pos = offset;
		break;
	// ���ļ���ǰλ��Ϊԭ�������ļ�ָ��
	case SEEK_CUR:
		if (file->f_pos + offset<0) return -EINVAL;
		file->f_pos += offset;
		break;
	// ���ļ�ĩβΪԭ�������ļ�ָ��
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

// ���ļ�ϵͳ����
// para : fd -- �ļ������ buf -- �������� count -- Ҫ�����ֽ���
// return : �����Ѷ����ֽ��� �� ������
int sys_read(unsigned int fd, char * buf, int count)
{
	struct file * file;
	struct m_inode * inode;
	// ������Ч�Լ��
	if (fd >= NR_OPEN || count<0 || !(file = current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
	// ��������С ���
	// verify_area(buf, count);
	inode = file->f_inode;
	// �ܵ��ļ�
	if (inode->i_pipe)
		return (file->f_mode & F_M_WR_MASK & F_M_READ) ? read_pipe(inode, buf, count) : -EIO;
	// �ַ��豸�ļ�
	if (S_ISCHR(inode->i_mode))
		return rw_char(READ, inode->i_zone[0], buf, count, &file->f_pos);
	// ���豸�ļ�
	if (S_ISBLK(inode->i_mode))
		return block_read(inode->i_zone[0], &file->f_pos, buf, count);
	// �����ļ��� Ŀ¼�ļ��� ��������
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

// д�ļ�ϵͳ����
// para : fd -- �ļ������ buf -- �������� count -- Ҫ�����ֽ���
// return : ������д���ֽ��� �� ������
int sys_write(unsigned int fd, char * buf, int count)
{
	struct file * file;
	struct m_inode * inode;
	// ������Ч�Լ��
	if (fd >= NR_OPEN || count <0 || !(file = current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
	inode = file->f_inode;
	// �ܵ��ļ�
	if (inode->i_pipe)
		return (file->f_mode & F_M_WR_MASK & F_M_WRITE) ? write_pipe(inode, buf, count) : -EIO;
	// �ַ��豸�ļ�
	if (S_ISCHR(inode->i_mode))
		return rw_char(WRITE, inode->i_zone[0], buf, count, &file->f_pos);
	// ���豸�ļ�
	if (S_ISBLK(inode->i_mode))
		return block_write(inode->i_zone[0], &file->f_pos, buf, count);
    // �����ļ�
	if (S_ISREG(inode->i_mode))
		return file_write(inode, file, buf, count);
	printk("(Write)inode->i_mode=%06o\n", inode->i_mode);
	return -EINVAL;
}