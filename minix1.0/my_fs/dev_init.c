#include"file_sys.h"
#include"kernel.h"
#include<memory.h>
#include"stat.h"
#include"fcntl.h"
#include"mix_erro.h"
static const unsigned int inode_size = sizeof(struct d_inode);
extern int write_to_file(char* buffer, unsigned int f_pos,
	unsigned int count);
extern struct super_block * read_super(int dev);
extern int open_noodle_file();
extern int close_noodle_file();
extern void tty_init();
extern int sys_useradd();

extern int sys_write(unsigned int fd, char* buf, int count);
extern int sys_open(const char * filename, int flag, int mode);
extern int sys_lseek(unsigned int fd, mix_off_t offset, int origin);
static char buffer[BLOCK_SIZE];
// 软盘空间布局如下
// 启动块 boot_block : 0
// 超级块 super_block : 1
// imap 占 8 块 ： 2 - 9
// zmpa 占 8 块 ： 10 - 17
// inode 占 剩下总块数的 1 / 20
// 再剩下的都是数据块


// 文件系统初始化
int fs_init(){
	int error;
	// 高速缓冲区初始化
	buffer_init();
	// 打开 noodle_file
	if ((error = open_noodle_file()) < 0)
		return error;
	// 安装根文件系统, 根文件系统在软盘上，设备号 = major - 2, minor - 1
	mount_root();
	// 任务结构初始化
	current->egid = current->euid = current->gid = current->uid = 0; // 根用户
	current->umask = 0;
}


// 制作根文件系统
static char root_dev[ROOT_SIZE]; // 1 M 
int  make_root_dev(){
	short mode = S_IRWXU | S_IXGRP | S_IXOTH;
	//char* start_addr = device_address[MAJOR(dev)][MINOR(dev)];
	char* start_addr = root_dev;
	int dev_size = ROOT_SIZE;
	unsigned int total_blocks = dev_size / BLOCK_SIZE;
	unsigned int inode_blocks = (total_blocks - 18) / 20;
	unsigned int data_blocks = total_blocks - 18 - inode_blocks;
	unsigned int inode_num = inode_blocks * (BLOCK_SIZE / inode_size);
	unsigned int firstdatazone = 1 + 1 + 8 + 8 + inode_blocks;
	struct d_super_block* sp = (struct d_super_block*) (start_addr + BLOCK_SIZE);
	struct d_inode* inode;
	struct dir_entry* dir;
	char* addr;
	int i;
	memset(start_addr, 0, total_blocks * BLOCK_SIZE);
	// 设置超级块, major = 2, minor = 1
	sp->s_magic = SUPER_MAGIC;
	sp->s_log_zone_size = 0;
	sp->s_ninodes = inode_num;
	sp->s_nzones = data_blocks;
	sp->s_max_size = 128 * 1024 * 1024;
	sp->s_firstdatazone = firstdatazone;
	sp->s_imap_blocks = sp->s_zmap_blocks = 8;
	// 设置根 i 节点， 1 号 i 节点
	inode = (struct d_inode*) (start_addr + BLOCK_SIZE * 18);
	inode->i_gid = inode->i_uid = 0;
	inode->i_mtime = CURRENT_TIME;
	inode->i_nlinks = 2;
	inode->i_mode = 0777 | S_IFDIR | S_ISVTX;
	inode->i_size = 2 * sizeof(struct dir_entry);
	for (i = 0; i < 9; i++)
		inode->i_zone[i] = 0;
	// 一号磁盘块
	inode->i_zone[0] = firstdatazone;
	// 2个添加目录项
	dir = (struct dir_entry*) (start_addr + BLOCK_SIZE * firstdatazone);
	dir->inode = ROOT_INO;
	dir->name[0] = '.';
	dir->name[1] = '\0';
	dir++;
	dir->inode = ROOT_INO;
	dir->name[0] = '.';
	dir->name[1] = '.';
	dir->name[2] = '\0';
	// 设置 imap 和 zmap
	addr = start_addr + BLOCK_SIZE * 2;
	*addr = 3;
	addr = start_addr + BLOCK_SIZE * 10;
	*addr = 3;
	if (open_noodle_file() < 0)
		return -1;
	if (write_to_file(start_addr, 0, dev_size) < 0)
		return -1;
	if (close_noodle_file() < 0)
		return -1;
	// 文件系统初始化
	fs_init();
	// 新建目录和设备文件
	sys_mkdir("/dev", mode | S_ISVTX);
	sys_mknod("/dev/hda", S_IRUSR|S_IWUSR | S_IFBLK, S_DEVICE(S_DISK, 1)); // hda
	//sys_mknod("/dev/hdc", mode | S_IFBLK, S_DEVICE(S_DISK, 2)); // hdc
	sys_mknod("/dev/floppy",S_IRUSR | S_IWUSR | S_IFBLK, S_DEVICE(S_FLOPPY, 1)); // floppy
	sys_mkdir("/home", mode | S_ISVTX);
	sys_mkdir("/etc", mode | S_ISVTX);
	// 软盘初始化
	device_format("/dev/floppy");
	// 磁盘初始化
	device_format("/dev/hda");
	// 登陆文件
	sys_mknod("/etc/log_in.ext", S_IRWXU | S_IFREG, 0);
	sys_useradd();
	sys_sync();
	if (close_noodle_file() < 0)
		return -1;
	return 1;
}


// 磁盘格式化
int device_format(const char* filename){
	int i, error, dev_size, dev;
	unsigned int total_blocks, inode_blocks, 
		data_blocks, inode_num, firstdatazone;
	unsigned int count, f_pos, fd;
	struct super_block* sb;
	struct d_super_block* sp = (struct d_super_block*)buffer;    
	struct d_inode* inode = (struct d_inode*)buffer;
	struct dir_entry* dir = (struct dir_entry*)buffer;

	if ((fd = sys_open(filename, O_RDWR, 0)) < 0)
		return fd;
	if (!S_ISBLK(current->filp[fd]->f_inode->i_mode))
		return -EPERM;
	dev = current->filp[fd]->f_inode->i_zone[0];
	// 判断是否挂载了文件系统
	for (sb = super_block; sb < super_block + NR_SUPER; sb++){
		if (!sb->s_dev)
			continue;
		if (sb->s_dev == dev && sb->s_imount)
			return -EBUSY;
	}
	//if (!(sb = read_super(dev)) || sb->s_imount)
	//	return -EBUSY;
	dev_size = blk_size[MAJOR(dev)][MINOR(dev)];
	total_blocks = dev_size / BLOCK_SIZE;
	inode_blocks = (total_blocks - 18) / 20;
	data_blocks = total_blocks - 18 - inode_blocks;
	inode_num = inode_blocks * (BLOCK_SIZE / inode_size);
	firstdatazone = 1 + 1 + 8 + 8 + inode_blocks;
	// 磁盘清零
	count = total_blocks * BLOCK_SIZE;
	f_pos = 0;
	if ((error = sys_lseek(fd, f_pos, SEEK_SET)) < 0)
		return error;
	while (count > 0){
		if ((error = sys_write(fd, buffer, count)) < 0)
			return error;
		count -= error;
	}
	//memset(start_addr, 0, total_blocks * BLOCK_SIZE);
	// 设置超级块, major = 2, minor = 1
	sp->s_magic = SUPER_MAGIC;
	sp->s_log_zone_size = 0;
	sp->s_ninodes = inode_num;
	sp->s_nzones = data_blocks;
	sp->s_max_size = 128 * 1024 * 1024;
	sp->s_firstdatazone = firstdatazone;
	sp->s_imap_blocks = sp->s_zmap_blocks = 8;

	count = sizeof(struct d_super_block);
	f_pos = 0 + 1 * BLOCK_SIZE;
	if ((error = sys_lseek(fd, f_pos, SEEK_SET)) < 0)
		return error;
	while (count > 0){
		if ((error = sys_write(fd, buffer, count)) < 0)
			return error;
		count -= error;
	}

	// 设置根 i 节点， 1 号 i 节点
	//inode = (struct d_inode*) (start_addr + BLOCK_SIZE * 18);
	inode->i_gid = inode->i_uid = 0;
	inode->i_mtime = CURRENT_TIME;
	inode->i_nlinks = 2;
	inode->i_mode = 0777 | S_IFDIR | S_ISVTX;
	inode->i_size = 2 * sizeof(struct dir_entry);
	for (i = 0; i < 9; i++)
		inode->i_zone[i] = 0;
	// 一号磁盘块
	inode->i_zone[0] = firstdatazone;

	count = sizeof(struct d_inode);
	f_pos = 0 + 18 * BLOCK_SIZE;
	if ((error = sys_lseek(fd, f_pos, SEEK_SET)) < 0)
		return error;
	while (count > 0){
		if ((error = sys_write(fd, buffer, count)) < 0)
			return error;
		count -= error;
	}

	// 2个添加目录项
	//dir = (struct dir_entry*) (start_addr + BLOCK_SIZE * firstdatazone);
	dir->inode = ROOT_INO;
	dir->name[0] = '.';
	dir->name[1] = '\0';
	dir++;
	dir->inode = ROOT_INO;
	dir->name[0] = '.';
	dir->name[1] = '.';
	dir->name[2] = '\0';

	count = 2 * sizeof(struct dir_entry);
	f_pos = 0 + firstdatazone * BLOCK_SIZE;
	if ((error = sys_lseek(fd, f_pos, SEEK_SET)) < 0)
		return error;
	while (count > 0){
		if ((error = sys_write(fd, buffer, count)) < 0)
			return error;
		count -= error;
	}
	// 设置 imap 和 zmap
	// addr = start_addr + BLOCK_SIZE * 2;
	buffer[0] = 3;
	count = 1;
	// imap
	f_pos = 0 + 2 * BLOCK_SIZE;
	if ((error = sys_lseek(fd, f_pos, SEEK_SET)) < 0)
		return error;
	if ((error = sys_write(fd, buffer, count)) < 0)
			return error;
	// zmap
	f_pos = 0 + 10 * BLOCK_SIZE;
	if ((error = sys_lseek(fd, f_pos, SEEK_SET)) < 0)
		return error;
	if ((error = sys_write(fd, buffer, count)) < 0)
		return error;
	// addr = start_addr + BLOCK_SIZE * 10;
	// *addr = 3;
	return 1;
}


// 
int device_change(int dev)
{
	return 0;
}


void make_root(){
	int error;
	tty_init();
	error = make_root_dev();
	if (error < 0)
		M_printf("failed ! : trying to make root device\n");
}
