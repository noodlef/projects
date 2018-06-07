#include"file_sys.h"
#include"kernel.h"
#include"memory.h"
#include"stat.h"
const unsigned int inode_size = sizeof(struct d_inode);
extern char* device_address[8][10];
// ���̿ռ䲼������
// ������ boot_block : 0
// ������ super_block : 1
// imap ռ 8 �� �� 2 - 9
// zmpa ռ 8 �� �� 10 - 17
// inode ռ ʣ���ܿ����� 1 / 20
// ��ʣ�µĶ������ݿ�
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
	// ���ó�����, major = 2, minor = 1
	sp->s_magic = SUPER_MAGIC;
	sp->s_log_zone_size = 0;
	sp->s_ninodes = inode_num;
	sp->s_nzones = data_blocks;
	sp->s_max_size = 1024;
	sp->s_firstdatazone = firstdatazone;
	sp->s_imap_blocks = sp->s_zmap_blocks = 8;
	// ���ø� i �ڵ㣬 1 �� i �ڵ�
	inode = (struct d_inode*) (start_addr + BLOCK_SIZE * 18);
	inode->i_gid = inode->i_uid = 0;
	inode->i_mtime = CURRENT_TIME;
	inode->i_nlinks = 2;
	inode->i_mode = 0777 | S_IFDIR | S_ISVTX;
	inode->i_size = 2 * sizeof(struct dir_entry);
	for (i = 0; i < 9; i++)
		inode->i_zone[i] = 0;
	// һ�Ŵ��̿�
	inode->i_zone[0] = firstdatazone;
	// 2�����Ŀ¼��
	dir = (struct dir_entry*) (start_addr + BLOCK_SIZE * firstdatazone);
	dir->inode = ROOT_INO;
	dir->name[0] = '.';
	dir->name[1] = '\0';
	dir++;
	dir->inode = ROOT_INO;
	dir->name[0] = '.';
	dir->name[1] = '.';
	dir->name[2] = '\0';
	// ���� imap �� zmap
	addr = start_addr + BLOCK_SIZE * 2;
	*addr = 3;
	addr = start_addr + BLOCK_SIZE * 10;
	*addr = 3;
}

// �ļ�ϵͳ��ʼ��
void fs_init()
{
	short mode = S_IRWXU | S_IRWXG | S_IRWXO ;
	// ���ٻ�������ʼ��
	buffer_init();
	// ���̳�ʼ���� �������ļ�ϵͳ
	device_init(S_DEVICE(S_FLOPPY, 1));
	// ���̳�ʼ��
	device_init(S_DEVICE(S_DISK, 1));
	// ��װ���ļ�ϵͳ, ���ļ�ϵͳ�������ϣ��豸�� = major - 2, minor - 1
	mount_root();
	
	// ����ṹ��ʼ��
	current->egid = current->euid = current->gid = current->uid = 0; // ���û�
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