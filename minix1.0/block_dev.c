#include"file_sys.h"
#include"mix_erro.h"

// ���豸�ļ���д����

// ���豸�Ĵ�С���������������
extern unsigned int *blk_size[];

// ���ݿ�д����--��ָ���豸�Ӹ���ƫ�ƴ�д��ָ�����ȵ�����
// para : dev -- �豸�ţ� pos -- �ļ���ƫ����ָ�룬
//        buf -- �û���������ַ�� count -- Ҫ���͵��ֽ���
// return : ������д���ֽ����� ������� �򷵻س�����
int block_write(int dev, off_t * pos, char * buf, int count)
{
	// ��������ݿ�� �� ����ƫ��
	int block = *pos / BLOCK_SIZE;
	int offset = *pos & (BLOCK_SIZE - 1);
	int chars;
	int written = 0;
	int size;
	struct buffer_head * bh;
	register char * p;
	// �õ����豸�����ֽ���
	if (blk_size[MAJOR(dev)])
		size = blk_size[MAJOR(dev)][MINOR(dev)];
	else
		size = 0x7fffffff;
	// �����ֽ���ת��Ϊ �ܿ���
	size = size / BLOCK_SIZE;
	while (count>0) {
		if (block >= size)
			return written ? written : -EIO;
		// �����д���ֽ���
		chars = BLOCK_SIZE - offset;
		// ����Ҫд����ֽ���
		if (chars > count)
			chars = count;
		if (chars == BLOCK_SIZE)
			bh = getblk(dev, block);
		else
			bh = breada(dev, block, block + 1, block + 2, -1);
		block++;
		if (!bh)
			return written ? written : -EIO;
		// ��һ�μ��� offset , ����offset��Ϊ 0 ��
		p = offset + bh->b_data;
		offset = 0;
		*pos += chars;
		written += chars;
		count -= chars;
		while (chars-->0)
			*(p++) = get_fs_byte(buf++);
		bh->b_dirt = 1;
		brelse(bh);
	}
	return written;
}

// ���ݿ������--��ָ���豸�Ӹ���ƫ�ƴ�����ָ�����ȵ�����
// para : dev -- �豸�ţ� pos -- �ļ���ƫ����ָ�룬
//        buf -- �û���������ַ�� count -- Ҫ���͵��ֽ���
// return : �����Ѷ����ֽ����� ������� �򷵻س�����
int block_read(int dev, off_t * pos, char * buf, int count)
{
	int block = *pos / BLOCK_SIZE;;
	int offset = *pos & (BLOCK_SIZE - 1);
	int chars;
	int size;
	int read = 0;
	struct buffer_head * bh;
	register char * p;

	if (blk_size[MAJOR(dev)])
		size = blk_size[MAJOR(dev)][MINOR(dev)];
	else
		size = 0x7fffffff;
	// �����ֽ���ת��Ϊ �ܿ���
	size = size / BLOCK_SIZE;
	while (count>0) {
		if (block >= size)
			return read ? read : -EIO;
		chars = BLOCK_SIZE - offset;
		if (chars > count)
			chars = count;
		if (!(bh = breada(dev, block, block + 1, block + 2, -1)))
			return read ? read : -EIO;
		block++;
		p = offset + bh->b_data;
		offset = 0;
		*pos += chars;
		read += chars;
		count -= chars;
		while (chars-->0)
			put_fs_byte(*(p++), buf++);
		brelse(bh);
	}
	return read;
}
