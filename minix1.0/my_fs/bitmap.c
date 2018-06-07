#include"file_sys.h"
#include"kernel.h"
#include<string.h>

// 将一个逻辑块清零
static int clear_block(char* addr)
{
	memset(addr, 0, BLOCK_SIZE);
	return 0;
}

// 释放文件所占的逻辑块
int free_block(int dev, int block)
{
	struct super_block* sb;
	struct buffer_head* bh;
	struct bit_map b_map;
	unsigned int offset;
	if (!(sb = get_super(dev)))//-----------
		panic("trying to free block on nonexistent device\n");
	if (block < sb->s_firstdatazone || block >= (sb->s_nzones + 18 + sb->s_ninodes / INODES_PER_BLOCK))
		panic("trying to free block not in datazone\n");
	// 检测是否有进程还在使用该逻辑块
	bh = get_hash_table(dev, block);
	if (bh) {
		if (bh->b_count > 1) {
			brelse(bh);
			return 0;
		}
		bh->b_dirt = 0;
		bh->b_uptodate = 0;
		if(bh->b_count)
			brelse(bh);
	}
	offset = block - (sb->s_firstdatazone - 1);
	b_map.total_bits = BLOCK_SIZE * 8;
	b_map.start_addr = sb->s_zmap[offset / (BLOCK_SIZE * 8)]->b_data;
	if (clear_bit(offset & (BLOCK_SIZE * 8 - 1), &b_map)) {
		M_printf("block (%04x:%d) ", dev, block + sb->s_firstdatazone - 1);
		panic("free_block: bit already cleared");
	}
	sb->s_zmap[block / (BLOCK_SIZE * 8)]->b_dirt = 1;
	return 1;
}

// 获取一个新的逻辑块
int new_block(int dev)
{
	struct buffer_head* bh;
	struct super_block* sb;
	int i, j, k;
	struct bit_map b_map = { BLOCK_SIZE * 8, 0 };
	if (!(sb = get_super(dev)))//-----------------
		panic("trying to get new block from nonexistant device");
	j = BLOCK_SIZE * 8;
	for (i = 0; i<8; i++)
		if (bh = sb->s_zmap[i])
		{
			b_map.start_addr = sb->s_zmap[i]->b_data;
			if ((j = find_first_zero(&b_map))<(BLOCK_SIZE * 8))
				break;
		}
	if (i >= 8 || !bh || j >= (BLOCK_SIZE * 8))
		return 0;
	// 判断获得的逻辑快的编号是否在正确范围内
	k = j + i * (BLOCK_SIZE * 8);
	if (k == 0 || k > sb->s_nzones)
		return 0;
	if (set_bit(j, &b_map))
		panic("new_block: bit already set");
	bh->b_dirt = 1;
	j = k + sb->s_firstdatazone - 1;
	if (!(bh = getblk(dev, j)))
		panic("new_block: cannot get block");
	if (bh->b_count != 1)
		panic("new block: count is != 1");
	clear_block(bh->b_data);
	bh->b_uptodate = 1;
	bh->b_dirt = 1;
	brelse(bh);
	return j;
}

// 释放 i 节点
void free_inode(struct m_inode * inode)
{
	struct super_block* sb;
	struct buffer_head* bh;
	struct bit_map b_map;
	unsigned int offset;
	if (!inode)
		return;
	if (!inode->i_dev) {
		memset(inode, 0, sizeof(*inode));
		return;
	}
	if (inode->i_count>1) {
		M_printf("trying to free inode with count=%d\n", inode->i_count);
		panic("free_inode");
	}
	if (inode->i_nlinks)
		panic("trying to free inode with links");
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to free inode on nonexistent device");
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)
		panic("trying to free inode 0 or nonexistant inode");
	if (!(bh = sb->s_imap[inode->i_num / (BLOCK_SIZE * 8)]))
		panic("nonexistent imap in superblock");
	b_map.total_bits = BLOCK_SIZE * 8;
	b_map.start_addr = bh->b_data;
	offset = inode->i_num & (BLOCK_SIZE * 8 - 1);
	if (clear_bit(offset, &b_map))
		panic("free_inode: bit already cleared");
	bh->b_dirt = 1;
	memset(inode, 0, sizeof(*inode));
}

// 获取 i 节点
struct m_inode * new_inode(int dev)   
{
	struct m_inode* inode;
	struct super_block* sb;
	struct buffer_head* bh;
	int i, j;
	struct bit_map b_map= { BLOCK_SIZE * 8, 0 };

	if (!(inode = get_empty_inode())) //---------
		return NULL;
	if (!(sb = get_super(dev)))
		panic("new_inode with unknown device");
	j = BLOCK_SIZE * 8;
	for (i = 0; i<8; i++)
		if (bh = sb->s_imap[i])
		{ 
			b_map.start_addr = bh->b_data;
			if ((j = find_first_zero(&b_map))<(BLOCK_SIZE * 8))
				break;
		}	
	if (!bh || j >= (BLOCK_SIZE * 8) || (j + i * (BLOCK_SIZE * 8)) > sb->s_ninodes) {
		iput(inode);//-----------
		return NULL;
	}
	if (set_bit(j, &b_map))
		panic("new_inode: bit already set");
	bh->b_dirt = 1;
	inode->i_count = 1;
	inode->i_update = 1;
	inode->i_nlinks = 1;
	inode->i_dev = dev;
	inode->i_uid = current->euid;
	inode->i_gid = current->egid;
	inode->i_dirt = 1;
	inode->i_num = j + i * (BLOCK_SIZE * 8);
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	return inode;
}
