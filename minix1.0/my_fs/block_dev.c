#include"file_sys.h"
#include"mix_erro.h"

// 块设备文件读写函数

// 块设备的大小保存在这个数组中
extern unsigned int *blk_size[];

// 数据块写函数--向指定设备从给定偏移处写入指定长度的数据
// para : dev -- 设备号， pos -- 文件中偏移量指针，
//        buf -- 用户缓冲区地址， count -- 要传送的字节数
// return : 返回已写入字节数， 如果出错， 则返回出错码
int block_write(int dev,  mix_off_t * pos, char * buf, int count)
{
	// 先求出数据快号 和 块内偏移
	int block = *pos / BLOCK_SIZE;
	int offset = *pos & (BLOCK_SIZE - 1);
	int chars;
	int written = 0;
	int size;
	struct buffer_head * bh;
	register char * p;
	// 得到块设备的总字节数
	if (blk_size[MAJOR(dev)])
		size = blk_size[MAJOR(dev)][MINOR(dev)];
	else
		size = 0x7fffffff;
	// 将总字节数转化为 总块数
	size = size / BLOCK_SIZE;
	while (count>0) {
		if (block >= size)
			return written ? written : -EIO;
		// 本块可写入字节数
		chars = BLOCK_SIZE - offset;
		// 本块要写入的字节数
		if (chars > count)
			chars = count;
		if (chars == BLOCK_SIZE)
			bh = getblk(dev, block);
		else
			bh = breada(dev, block, block + 1, block + 2, -1);
		block++;
		if (!bh)
			return written ? written : -EIO;
		// 第一次加上 offset , 后面offset就为 0 了
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

// 数据块读函数--向指定设备从给定偏移处读出指定长度的数据
// para : dev -- 设备号， pos -- 文件中偏移量指针，
//        buf -- 用户缓冲区地址， count -- 要传送的字节数
// return : 返回已读入字节数， 如果出错， 则返回出错码
int block_read(int dev,  mix_off_t * pos, char * buf, int count)
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
	// 将总字节数转化为 总块数
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
