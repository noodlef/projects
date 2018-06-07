#include"file_sys.h"
#include"kernel.h"
#include"mix_erro.h"
#include"stat.h"
#include"fcntl.h"
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// 文件读函数 -- 根据 i 节点 和文件结构， 读取文件中的数据 -- 正规文件
// para : inode -- 文件 i 节点， filp -- 文件结构
//        buf -- 缓冲区， count -- 要读取字节数
// return : 返回值是实际读取的字节数， 如果出错， 则返回出错码
int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left, chars, nr;
	struct buffer_head * bh;

	if ((left = count) <= 0)
		return 0;
	while (left) {
		// 得到逻辑快号
		if (nr = bmap(inode, (filp->f_pos) / BLOCK_SIZE)) {
			if (!(bh = bread(inode->i_dev, nr)))
				break;
		}
		else
			bh = NULL;
		// 计算偏移量
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
		// 如果得不到数据块对应的逻辑块， 则填 0
		else {
			while (chars-->0)
				put_fs_byte(0, buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;
	return (count - left) ? (count - left) : -ERROR;
}


// 文件写函数 -- 根据 i 节点 和文件结构， 将数据写入文件中-- 正规文件
// para : inode -- 文件 i 节点， filp -- 文件结构
//        buf -- 缓冲区， count -- 要读取字节数
// return : 返回值是实际写入的字节数， 如果出错， 则返回出错码
int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	mix_off_t pos;
	int block, c;
	struct buffer_head * bh;
	char * p;
	int i = 0;

	/*
	* ok, append may not work when many processes are writing at the same time
	* but so what. That way leads to madness anyway.
	*/
	// 如果设置 尾部添加标志， 则设置文件指针指向文件尾
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
