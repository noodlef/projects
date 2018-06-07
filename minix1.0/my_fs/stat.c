#include"file_sys.h"
#include"stat.h"
#include"mix_erro.h"
#include"kernel.h"
#include"tool.h"
#include"mix_window.h"
/*
 * 本文件实现提取文件状态信息的系统调用
 *   stat() 和 fstat()
 */
extern int set_char_bcolor(unsigned minor, enum CB_color b_color);
extern int set_tty_color(unsigned minor, enum CB_color b_color);
extern struct super_block super_block[NR_SUPER];

static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	//verify_area(statbuf, sizeof(struct stat));
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_num;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlinks;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_zone[0];
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	for (i = 0; i < sizeof(tmp); i++)
		*(i + (char *)statbuf) = ((char *)&tmp)[i];
		//put_fs_byte(((char *)&tmp)[i], i + (char *)statbuf);
}

// 获取文件的 i 节点信息
int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	cp_stat(inode, statbuf);
	iput(inode);
	return 0;
}


// 获取文件的 i 节点信息， 不跟随符号链接
int sys_lstat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode = lnamei(filename)))
		return -ENOENT;
	cp_stat(inode, statbuf);
	iput(inode);
	return 0;
}

// 根据文件描述符获取文件的 i 节点信息
int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	if (fd >= NR_OPEN || !(f = current->filp[fd]) || !(inode = f->f_inode))
		return -EBADF;
	cp_stat(inode, statbuf);
	return 0;
}

// 读符号链接系统调用, 不复制最后的空字符
int sys_readlink(const char * path, char * buf, int bufsiz)
{
	struct m_inode * inode;
	struct buffer_head * bh;
	int i;
	char c;

	if (bufsiz <= 0)
		return -EBADF;
	if (bufsiz > 1023)
		bufsiz = 1023;
	// verify_area(buf, bufsiz);
	if (!(inode = lnamei(path)))
		return -ENOENT;
	if (!S_ISLNK(inode->i_mode))
		return -EBADF;
	if (inode->i_zone[0])
		bh = bread(inode->i_dev, inode->i_zone[0]);
	else
		bh = NULL;
	iput(inode);
	if (!bh)
		return 0;
	i = 0;
	while (i<bufsiz && (c = bh->b_data[i])) {
		i++;
		put_fs_byte(c, buf++);
	}
	brelse(bh);
	return i;
}


static void print_super(struct super_block* sp){
	int size, count = 50, i, free, color;
	struct bit_map b_map = {BLOCK_SIZE * 8, 0};

	M_printf("s_ninodes : %d, s_nzones : %d\n", sp->s_ninodes, sp->s_nzones);
	M_printf("s_imap_blocks : %d, s_zmap_blocks : %d\n", 
		      sp->s_imap_blocks, sp->s_zmap_blocks);
	M_printf("s_firstdatazone : %d, s_log_zone_size : %d\n", 
		      sp->s_firstdatazone, sp->s_log_zone_size);
	M_printf("s_max_size : %d, s_magic %d\n", sp->s_max_size, sp->s_magic);

	M_printf("device : %d\n", sp->s_dev);
	i = ((BLOCK_SIZE / (sizeof(struct d_super_block))));
	size = 2 + sp->s_imap_blocks + sp->s_zmap_blocks
		+ sp->s_ninodes / ((BLOCK_SIZE / (sizeof(struct d_inode)))) + sp->s_nzones;
	free = 0;
	i = sp->s_nzones + 1;
	while (i-- > 0){
		b_map.start_addr = sp->s_zmap[i / (BLOCK_SIZE * 8)]->b_data;
		if (!test_bit(i % (BLOCK_SIZE * 8), &b_map))
			free++;
	}

	//color = set_char_fcolor(0, CFC_Red);
	color = set_char_bcolor(0, CBC_Blue);
	i = 0;
	while (i++ < (free * count / size))
		M_printf("|");
	set_char_bcolor(0, CBC_HighWhite);
	while (i++ < count)
		M_printf("|");
	//set_char_fcolor(0, color);
	if (color < 0)
		reset_char_bcolor(0);
	else
		set_char_bcolor(0, color);
	M_printf(" %.2f%[%dM/%dM]\n", 100.0 * free / size,
		free * BLOCK_SIZE / 1024, size * BLOCK_SIZE / 1024);
	return ;
}
// 打印超级快信息
int sys_readsuper(){
	int i = NR_SUPER;
	struct super_block* sp;

	while (i-- > 0){
		sp = super_block + i;
		if (!sp->s_dev)
			continue;
		print_super(sp);
	}
	return 1;
}
