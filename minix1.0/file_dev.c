#include"file_sys.h"
#include"kernel.h"
#include"mix_erro.h"
#include"stat.h"
#include"fcntl.h"
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// �ļ������� -- ���� i �ڵ� ���ļ��ṹ�� ��ȡ�ļ��е����� -- �����ļ�
// para : inode -- �ļ� i �ڵ㣬 filp -- �ļ��ṹ
//        buf -- �������� count -- Ҫ��ȡ�ֽ���
// return : ����ֵ��ʵ�ʶ�ȡ���ֽ����� ������� �򷵻س�����
int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left, chars, nr;
	struct buffer_head * bh;

	if ((left = count) <= 0)
		return 0;
	while (left) {
		// �õ��߼����
		if (nr = bmap(inode, (filp->f_pos) / BLOCK_SIZE)) {
			if (!(bh = bread(inode->i_dev, nr)))
				break;
		}
		else
			bh = NULL;
		// ����ƫ����
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN(BLOCK_SIZE - nr, left);
		filp->f_pos += chars;
		left -= chars;
		if (bh) {
			char * p = nr + bh->b_data;
			while (chars-->0)
				put_fs_byte(*(p++), buf++);
			brelse(bh);
		}
		// ����ò������ݿ��Ӧ���߼��飬 ���� 0
		else {
			while (chars-->0)
				put_fs_byte(0, buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return (count - left) ? (count - left) : -ERROR;
}


// �ļ�д���� -- ���� i �ڵ� ���ļ��ṹ�� ������д���ļ���-- �����ļ�
// para : inode -- �ļ� i �ڵ㣬 filp -- �ļ��ṹ
//        buf -- �������� count -- Ҫ��ȡ�ֽ���
// return : ����ֵ��ʵ��д����ֽ����� ������� �򷵻س�����
int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	off_t pos;
	int block, c;
	struct buffer_head * bh;
	char * p;
	int i = 0;

	/*
	* ok, append may not work when many processes are writing at the same time
	* but so what. That way leads to madness anyway.
	*/
	// ������� β����ӱ�־�� �������ļ�ָ��ָ���ļ�β
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	while (i<count) {
		if (!(block = create_block(inode, pos / BLOCK_SIZE)))
			break;
		if (!(bh = bread(inode->i_dev, block)))
			break;
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;
		bh->b_dirt = 1;
		c = BLOCK_SIZE - c;
		if (c > count - i) 
			c = count - i;
		pos += c;
		if (pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		while (c-->0)
			*(p++) = get_fs_byte(buf++);
		brelse(bh);
	}
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {               //?????????????
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i ? i : -ERROR);
}
