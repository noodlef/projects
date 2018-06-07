#include"file_sys.h"
#include"kernel.h"
#include<memory.h>
#include"stat.h"
#include"mm.h"

// inode_table
struct m_inode inode_table[NR_INODE] = { { 0, }, };

// ǰ������
static void read_inode(struct m_inode * inode);
static void write_inode(struct m_inode * inode);

// �� inode ����
static void clear_inode(struct m_inode* inode) {
	memset(inode, 0, sizeof(struct m_inode));
}

//����inode��Ч
void invalidate_inodes(int dev)
{
	int i;
	struct m_inode * inode;
	inode = 0 + inode_table;
	for (i = 0; i<NR_INODE; i++, inode++) {
		//wait_on_inode(inode);
		if (inode->i_dev == dev) {
			if (inode->i_count)
				panic("inode in use on removed disk");
			clear_inode(inode);
			//inode->i_dev = inode->i_dirt = inode->i_update = 0; // �޸� 
		}
	}
}

// �� inode ���豸�е�i �ڵ����ͬ��
void sync_inodes(void)
{
	int i;
	struct m_inode * inode;
	inode = 0 + inode_table;
	for (i = 0; i<NR_INODE; i++, inode++) {
		//wait_on_inode(inode);
		if (inode->i_dirt && !inode->i_pipe && inode->i_update)
			write_inode(inode);
	}
}

// ���ļ����ݿ�ӳ�䵽�߼����ϣ��������߼����
// �� creat = 1 ʱ�� ����Ӧ�߼��鲻����ʱ�������´��̿�
static int _bmap(struct m_inode * inode, int block, int create)
{
	struct buffer_head * bh;
	int i;
	int per_block;
	if (block<0)
		panic("_bmap: block<0");
	per_block = BLOCK_SIZE / sizeof(unsigned int);
	if (block >= 7 + per_block + per_block * per_block)
		panic("_bmap: block>big");
	if (block<7) {
		if (create && !inode->i_zone[block])
			if (inode->i_zone[block] = new_block(inode->i_dev)) {
				inode->i_ctime = CURRENT_TIME;
				inode->i_dirt = 1;
			}
		return inode->i_zone[block];
	}
	block -= 7;
	if (block< per_block) {
		if (create && !inode->i_zone[7])
			if (inode->i_zone[7] = new_block(inode->i_dev)) {
				inode->i_dirt = 1;
				inode->i_ctime = CURRENT_TIME;
			}
		if (!inode->i_zone[7])
			return 0;
		if (!(bh = bread(inode->i_dev, inode->i_zone[7])))
			return 0;
		i = ((unsigned int *)(bh->b_data))[block];
		if (create && !i)
			if (i = new_block(inode->i_dev)) {
				((unsigned int *)(bh->b_data))[block] = i;
				bh->b_dirt = 1;
			}
		brelse(bh);
		return i;
	}
	block -= per_block;
	if (create && !inode->i_zone[8])
		if (inode->i_zone[8] = new_block(inode->i_dev)) {
			inode->i_dirt = 1;
			inode->i_ctime = CURRENT_TIME;
		}
	if (!inode->i_zone[8])
		return 0;
	if (!(bh = bread(inode->i_dev, inode->i_zone[8])))
		return 0;
	i = ((unsigned int *)bh->b_data)[block / per_block];
	if (create && !i)
		if (i = new_block(inode->i_dev)) {
			((unsigned short *)(bh->b_data))[block / per_block] = i;
			bh->b_dirt = 1;
		}
	brelse(bh);
	if (!i)
		return 0;
	if (!(bh = bread(inode->i_dev, i)))
		return 0;
	i = ((unsigned int *)bh->b_data)[block % per_block];
	if (create && !i)
		if (i = new_block(inode->i_dev)) {
			((unsigned int *)(bh->b_data))[block % per_block] = i;
			bh->b_dirt = 1;
		}
	brelse(bh);
	return i;
}

// �����ļ����ݿ�Ż����Ӧ���߼����
int bmap(struct m_inode * inode, int block)
{
	return _bmap(inode, block, 0);
}

// ���ļ����ݿ�ӳ�䵽�߼����ϣ��������߼����
// �� ��Ӧ�߼��鲻����ʱ�������´��̿�
int create_block(struct m_inode * inode, int block)
{
	return _bmap(inode, block, 1);
}


// �ͷ�һҳ
int free_page(char* addr)
{
	int blk_num = 4;
	memset(addr, 0, blk_num * BLOCK_SIZE);
	return 0;
}

// �ͷ� i �ڵ�
void iput(struct m_inode* inode)
{
	if (!inode)
		return;
	//wait_on_inode(inode);
	if (!inode->i_count)
		panic("iput: trying to free free inode");
	// �ܵ� i �ڵ�
	if (inode->i_pipe) {
		//wake_up(&inode->i_wait);
		//wake_up(&inode->i_wait2);
		if (--inode->i_count)
			return;
		free_page((char*)inode->i_size);//
		inode->i_count = 0;
		inode->i_dirt = 0;
		inode->i_pipe = 0;
		return;
	}
	// Ϊ��iget() �е� iput(empty_node)
	if (!inode->i_dev) {
		inode->i_count--;
		return;
	}
	// ����ǿ��豸 i �ڵ�
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		//wait_on_inode(inode);
	}
repeat:
	if (inode->i_count>1) {
		inode->i_count--;
		return;
	}
	if (!inode->i_nlinks) {
		truncate(inode);
		free_inode(inode);
		return;
	}
	if (inode->i_dirt) {
		write_inode(inode);	/* we can sleep - so do again */
		//wait_on_inode(inode);
		goto repeat;
	}
	inode->i_count--;
	return;
}

// ��inode_table ��ȡ���б���
struct m_inode * get_empty_inode(void)
{
	struct m_inode* inode;
	static struct m_inode* last_inode = inode_table;
	int i;
	// �� inode_table �в��ҿ��б���
	do {
		inode = NULL;
		for (i = NR_INODE; i; i--) {
			if (last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!last_inode->i_count) {
				inode = last_inode;
				break;
			}
			++last_inode;
		}
		// û����inode table���ҵ����еı���
		// ��ʾ����ͣ��
		if (!inode) {
			for (i = 0; i<NR_INODE; i++)
				M_printf("%04x: %6d\n", inode_table[i].i_dev,
					inode_table[i].i_num);
			panic("No free inodes in mem\n");
		}
		//wait_on_inode(inode);
		// �ҵ����б���
		while (inode->i_dirt && inode->i_update) {
			write_inode(inode);
			//wait_on_inode(inode);
		}
	} while (inode->i_count);
	memset(inode, 0, sizeof(*inode));
	inode->i_count = 1;                   
	return inode;
}

int get_free_page()
{
	return 0;
}


// ��ȡ�ܵ� i �ڵ�
struct m_inode* get_pipe_inode(void)
{
	struct m_inode* inode;

	if (!(inode = get_empty_inode()))
		return NULL;
	if (!(inode->i_size = get_free_page())) {
		inode->i_count = 0;
		return NULL;
	}
	inode->i_count = 2;	/* sum of readers/writers */
	PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;
	inode->i_pipe = 1;
	return inode;
}

// ���豸�ж�ȡָ���� i �ڵ㵽 i �ڵ����
struct m_inode* iget(int dev, int nr)
{
	struct m_inode * inode, *empty;
	if (!dev)
		panic("iget with dev==0");
	// Ԥ�Ȼ�ȡһ�����еı���
	empty = get_empty_inode();
	inode = inode_table;
	// �� inode table �в���
	while (inode < NR_INODE + inode_table) {
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode++;
			continue;
		}
		//wait_on_inode(inode);
		if (inode->i_dev != dev || inode->i_num != nr || !inode->i_update) {   // �޸� 12.28 �� ��� !inode->i_update
			inode = inode_table;
			continue;
		}
		// �ҵ��˶�Ӧ�� inode
		inode->i_count++;
		// ������Ƿ����ļ�ϵͳ��װ��
		if (inode->i_mount) {
			int i;
			for (i = 0; i<NR_SUPER; i++)
				if (super_block[i].s_imount == inode)
					break;
			if (i >= NR_SUPER) {
				M_printf("Mounted inode hasn't got sb\n");
				if (empty)
					iput(empty);
				return inode;
			}
			iput(inode);
			dev = super_block[i].s_dev;    // ����ֱ�Ӵ� isup �ж�ȡ i �ڵ�
			nr = ROOT_INO;
			inode = inode_table;
			continue;
		}
		if (empty)
			iput(empty);
		return inode;
	}
	// �� inode table ��û���ҵ�
	// ��˵���䲻���ļ�ϵͳ���ڵ㣬���ֱ�Ӵ��豸�ж��뼴��
	if (!empty)
		return (NULL);
	inode = empty;
	inode->i_dev = dev;
	inode->i_num = nr;
	read_inode(inode);
	return inode;
}


// �� inode д�����
static void write_inode(struct m_inode * inode)
{
	struct super_block* sb;
	struct buffer_head* bh;
	int block;
	//lock_inode(inode);
	if (!inode->i_dev || !inode->i_dirt) {
		//unlock_inode(inode);
		return;
	}
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to write inode without device\n");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num - 1) / INODES_PER_BLOCK;
	if (!(bh = bread(inode->i_dev, block)))
		panic("unable to read i-node block");
	((struct d_inode *)bh->b_data)
		[(inode->i_num - 1) % INODES_PER_BLOCK] =
		*(struct d_inode *)inode;
	bh->b_dirt = 1;
	inode->i_dirt = 0;
	brelse(bh);
	//unlock_inode(inode);
}


// �Ӵ����ж��� inode
static void read_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	//lock_inode(inode);
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to read inode without dev\n");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num - 1) / INODES_PER_BLOCK;
	if (!(bh = bread(inode->i_dev, block)))
		panic("unable to read i-node block\n");
	*(struct d_inode *)inode =
		((struct d_inode *)bh->b_data)
		[(inode->i_num - 1) % INODES_PER_BLOCK];
	brelse(bh);
	inode->i_update = 1;   
	// ����ǿ��豸 i �ڵ�
	if (S_ISBLK(inode->i_mode)) {
		int i = inode->i_zone[0];
		if (blk_size[MAJOR(i)])
			inode->i_size =  blk_size[MAJOR(i)][MINOR(i)];
		else
			inode->i_size = 0x7fffffff;
	}
	//unlock_inode(inode);
}

