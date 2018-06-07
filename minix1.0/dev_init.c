#include"file_sys.h"
#include"kernel.h"
#include"memory.h"
#include"stat.h"
const unsigned int inode_size = sizeof(struct d_inode);
extern char* device_address[8][10];
// 软盘空间布局如下
// 启动块 boot_block : 0
// 超级块 super_block : 1
// imap 占 8 块 ： 2 - 9
// zmpa 占 8 块 ： 10 - 17
// inode 占 剩下总块数的 1 / 20
// 再剩下的都是数据块
void device_init(int dev)
{
	char* start_addr = device_address[MAJOR(dev)][MINOR(dev)];
	int dev_size = blk_size[MAJOR(dev)][MINOR(dev)];
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
	sp->s_max_size = 1024;
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
}

// 文件系统初始化
void fs_init()
{
	short mode = S_IRWXU | S_IRWXG | S_IRWXO ;
	// 高速缓冲区初始化
	buffer_init();
	// 软盘初始化， 制作根文件系统
	device_init(S_DEVICE(S_FLOPPY, 1));
	// 磁盘初始化
	device_init(S_DEVICE(S_DISK, 1));
	// 安装根文件系统, 根文件系统在软盘上，设备号 = major - 2, minor - 1
	mount_root();
	
	// 任务结构初始化
	current->egid = current->euid = current->gid = current->uid = 0; // 根用户
	current->umask = 0;

	sys_mkdir("/dev", mode | S_ISVTX);
	sys_mknod("/dev/hda", mode | S_IFBLK, S_DEVICE(S_DISK, 1)); // hda
	sys_mknod("/dev/hdc", mode | S_IFBLK, S_DEVICE(S_DISK, 2)); // hdc
	sys_mknod("/dev/floppy", mode | S_IFBLK, S_DEVICE(S_FLOPPY, 2)); // floppy
	sys_mkdir("/home", mode | S_ISVTX);
	sys_mkdir("/etc", mode | S_ISVTX);
}

// 
int device_change(int dev)
{
	return 0;
}