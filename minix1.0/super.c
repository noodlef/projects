#include"file_sys.h"
#include"kernel.h"
#include"mix_type.h"
#include"mix_erro.h"
#include"stat.h"
#include"memory.h"
struct super_block super_block[NR_SUPER];
/* this is initialized in init/main.c */
// major = 2, minor = 1;
// 这是根文件系统的设备号，主设备号 = 2 （软盘），次设备号 = 1
int ROOT_DEV = 2 * (1 << 8) + 1;

// 释放超级块
static void free_super(struct super_block * sb)
{
	memset(sb, 0, sizeof(struct super_block));
	sb->s_lock = 0;
	sb->s_dev = 0;

}

// 在超级快表中寻找指定设备的超级快
// return : 成功 -- 返回设备的超级快， 否则返回 0
struct super_block* get_super(int dev)
{
	struct super_block* s;
	if (!dev)
		return NULL;
	s = 0 + super_block;
	while (s < NR_SUPER + super_block)
		if (s->s_dev == dev) {
			//wait_on_super(s);
			if (s->s_dev == dev)
				return s;
			s = 0 + super_block;
		}
		else
			s++;
	return NULL;
}

// 释放指定设备的超级快
void put_super(int dev)
{
	struct super_block * sb;
	int i;
	// 不能释放根文件系统的超级快
	if (dev == ROOT_DEV) {
		panic("root diskette changed: prepare for armageddon\n");
		return;
	}
	if (!(sb = get_super(dev)))
		return;
	// 不能释放已经安装文件系统的超级块
	if (sb->s_imount) {
		panic("Mounted disk changed - tssk, tssk\n");
		return;
	}
	//lock_super(sb);
	//sb->s_dev = 0;
	// 释放 imap 和 zmap 所占的高速缓冲块
	for (i = 0; i<I_MAP_SLOTS; i++)
		brelse(sb->s_imap[i]);
	for (i = 0; i<Z_MAP_SLOTS; i++)
		brelse(sb->s_zmap[i]);
	free_super(sb);
	return;
}


// 从设备中读出超级块
// para : dev -- 设备号
static struct super_block * read_super(int dev)
{
	struct super_block * s;
	struct buffer_head * bh;
	int i, block;

	if (!dev)
		return NULL;
	check_disk_change(dev);
	if (s = get_super(dev))
		return s;
	// 从超级块表中找出一个空闲的表项
	for (s = 0 + super_block;; s++) {
		if (s >= NR_SUPER + super_block)
			return NULL;
		if (!s->s_dev)
			break;
	}
	s->s_dev = dev;
	s->s_isup = NULL;
	s->s_imount = NULL;
	s->s_time = 0;
	s->s_rd_only = 0;
	s->s_dirt = 0;
	//lock_super(s);
	// 读超级块信息
	if (!(bh = bread(dev, 1))) {
		//s->s_dev = 0;
		//free_super(s);
		put_super(dev);
		return NULL;
	}
	*((struct d_super_block *) s) =
		*((struct d_super_block *) bh->b_data);
	brelse(bh);
	if (s->s_magic != SUPER_MAGIC) {
		//s->s_dev = 0;
		//free_super(s);
		put_super(dev);
		return NULL;
	}
	for (i = 0; i<I_MAP_SLOTS; i++)
		s->s_imap[i] = NULL;
	for (i = 0; i<Z_MAP_SLOTS; i++)
		s->s_zmap[i] = NULL;
	block = 2;
	// 将imap 和 zmap 读入内存
	// 但并不释放他们，它们在缓冲区的引用计数为1
	for (i = 0; i < s->s_imap_blocks; i++)
		if (s->s_imap[i] = bread(dev, block))
			block++;
		else
			break;
	for (i = 0; i < s->s_zmap_blocks; i++)
		if (s->s_zmap[i] = bread(dev, block))
			block++;
		else
			break;
	if (block != 2 + s->s_imap_blocks + s->s_zmap_blocks) {
		for (i = 0; i<I_MAP_SLOTS; i++)
			brelse(s->s_imap[i]);
		for (i = 0; i<Z_MAP_SLOTS; i++)
			brelse(s->s_zmap[i]);
		put_super(dev);
		return NULL;
	}
	// 将 imap 和 zmap 的第零位 置 1
	s->s_imap[0]->b_data[0] |= 1;
	s->s_zmap[0]->b_data[0] |= 1;
	return s;
}




// 挂载根文件系统
void mount_root(void)
{
	int i, free;
	struct super_block * p;
	struct m_inode * mi;
	struct bit_map b_map;
	unsigned int offset;

	//if (32 != sizeof(struct d_inode))
	//	 panic("bad i-node size");
	// 初始化文件表
	for (i = 0; i<NR_FILE; i++)
		file_table[i].f_count = 0;
	// 检查根文件系统所在设备是不是 软盘
	if (MAJOR(ROOT_DEV) == 2) {
		M_printf("Insert root floppy and press ENTER\n");
		//wait_for_keypress();
	}
	// 初始化超级块表
	for (p = &super_block[0]; p < &super_block[NR_SUPER]; p++) {
		//p->s_dev = 0;
		//p->s_lock = 0;
		//p->s_wait = NULL;
		memset(p, 0, sizeof(struct super_block));
	}
	if (!(p = read_super(ROOT_DEV)))
		panic("Unable to mount root\n");
	if (!(mi = iget(ROOT_DEV, ROOT_INO)))
		panic("Unable to read root i-node\n");
	mi->i_count += 3;	/* NOTE! it is logically used 4 times, not 1 */
	p->s_isup = p->s_imount = mi;
	current->pwd = mi;
	current->root = mi;
	b_map.total_bits = BLOCK_SIZE * 8;
	free = 0;
	i = p->s_nzones + 1;
		//- (1 + 1 + p->s_imap_blocks + p->s_zmap_blocks 
		// + p->s_ninodes / (BLOCK_SIZE / sizeof(struct d_inode)) );
	while (--i >= 0)
	{
		b_map.start_addr = p->s_zmap[i / (BLOCK_SIZE * 8)]->b_data;
		offset = i % (BLOCK_SIZE * 8);
		if (!test_bit(offset, &b_map))
			free++;
	}
	M_printf("%d/%d free blocks\n", free, p->s_nzones);
	free = 0;
	i = p->s_ninodes + 1;
	while (--i >= 0)
	{
		b_map.start_addr = p->s_imap[i / (BLOCK_SIZE * 8)]->b_data;
		offset = i % (BLOCK_SIZE * 8);
		if (!test_bit(offset, &b_map))
			free++;
	}
	M_printf("%d/%d free inodes\n", free, p->s_ninodes);
}

// 安装文件系统
// return : 0 -- 成功， 否则返回错误码
int sys_mount(char * dev_name, char * dir_name, int rw_flag)
{
	struct m_inode * dev_i, *dir_i;
	struct super_block * sb;
	int dev;
	// 获取设备号
	if (!(dev_i = namei(dev_name)))
		return -ENOENT;
	dev = dev_i->i_zone[0];
	// 如果不是块设备就返回出错码
	if (!S_ISBLK(dev_i->i_mode)) {
		iput(dev_i);
		return -EPERM;
	}
	iput(dev_i);
	if (!(dir_i = namei(dir_name)))
		return -ENOENT;
	// 判断该inode是否被其他的进程占用
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {
		iput(dir_i);
		return -EBUSY;
	}
	// 文件系统只能安装在目录文件下
	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -EPERM;
	}
	if (!(sb = read_super(dev))) {
		iput(dir_i);
		return -EBUSY;
	}
	if (sb->s_imount) {
		iput(dir_i);
		return -EBUSY;
	}
	if (dir_i->i_mount) {
		iput(dir_i);
		put_super(dev);
		return -EPERM;
	}
	// 读入文件系统的 1 号 i 节点
	if (!(sb->s_isup = iget(sb->s_dev, ROOT_INO))) {
		iput(dir_i);
		put_super(dev);
		return -ENODEV;
	}
	sb->s_imount = dir_i;
	dir_i->i_mount = 1;
	dir_i->i_dirt = 1;		/* NOTE! we don't iput(dir_i) */
	return 0;			    /* we do that in umount */
}

// 卸载文件系统
// para : dev_name -- 设备名
int sys_umount(char * dev_name)
{
	struct m_inode * inode;
	struct super_block * sb;
	int dev;
	// 获取设备号
	if (!(inode = namei(dev_name)))
		return -ENOENT;
	dev = inode->i_zone[0];
	if (!S_ISBLK(inode->i_mode)) {
		iput(inode);
		return -ENOTBLK;
	}
	iput(inode);
	// 获取设备的超级快
	if (dev == ROOT_DEV)
		return -EBUSY;
	if (!(sb = get_super(dev)) || !(sb->s_imount))
		return -ENOENT;
	if (!sb->s_imount->i_mount)
		printk("Mounted inode has i_mount=0\n");
	// 如果有进程在使用该设备的inode， 则返回
	for (inode = inode_table + 0; inode<inode_table + NR_INODE; inode++)
		if (inode->i_dev == dev && inode->i_count 
			                    && inode->i_num != ROOT_INO)
			return -EBUSY;
	sb->s_imount->i_mount = 0;
	iput(sb->s_imount);
	sb->s_imount = NULL;
	iput(sb->s_isup); //？？？？？？ 释放文件系统的根目录 i 节点，可是在安装文件系统时，该值被设置为 0
	sb->s_isup = NULL;
	put_super(dev);
	sync_dev(dev);
	return 0;
}