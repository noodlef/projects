#pragma once
#ifndef FILE_SYS_H
#define FILE_SYS_H
#include"mix_type.h"




#define BLOCK_SIZE 256


#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

// 主次设备号
#define MAJOR(a) (((unsigned int)(a))>>8)
#define MINOR(a) ((a)&0xff)
/* devices are as follows: (same as minix, so we can use the minix
* file system. These are major numbers.)
*
* 0 - unused (nodev)
* 1 - /dev/mem
* 2 - /dev/fd
* 3 - /dev/hd
* 4 - /dev/ttyx
* 5 - /dev/tty
* 6 - /dev/lp
* 7 - unnamed pipes
*/

#define S_FLOPPY 2
#define S_DISK 3
#define S_DEVICE(major,minor) ((major << 8) + minor)



extern int ROOT_DEV;                                                           // 根文件系统所在的设备
#define ROOT_INO 1                                                             // 根inode的编号为 1
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))              // 一个逻辑块包含的 i 节点数
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))       // 一个逻辑块包含的目录项个数



#define NR_OPEN 20        // 进程所能打开文件个数，也是文件描述符的总个数
 



//管道
#define PAGE_SIZE 4 * BLOCK_SIZE
#define PIPE_READ_WAIT(inode) ((inode).i_wait)
#define PIPE_WRITE_WAIT(inode) ((inode).i_wait2)
#define PIPE_HEAD(inode) ((inode).i_zone[0])
#define PIPE_TAIL(inode) ((inode).i_zone[1])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))



//imap 和 zmap 所占的最大缓冲块数
#define I_MAP_SLOTS 8
#define Z_MAP_SLOTS 8
#define SUPER_MAGIC 0x137F


#define get_fs_byte(addr) (*((char*)addr))
#define put_fs_byte(value,addr) (*((char*)addr) = (value))
// blk_size存放这指针，该指针指向一个数组，
// 该数组中存放相应设备的容量的大小
extern unsigned int* blk_size[7];






struct buffer_head{
	char * b_data;			           // 指向数据块的指针        
	mix_long b_blocknr;	               // 逻辑块的块号（从 0 开始编号）
	mix_short b_dev;		           // 逻辑块所在的设备号，b_dev = 0 表示该缓冲块处于空闲状态
	unsigned char b_uptodate;          // 更新标志：表示数据是否更新，1（更新，有效），0（无效）
	unsigned char b_dirt;		       // 修改标志：表示缓冲块中的数据与设备中的数据是否一致，0（clean,未修改）， 1（dirty, 修改)
	unsigned char b_count;		       // 使用该块的用户数         
	unsigned char b_lock;		       // 缓冲块是否被锁定， 在从设备中读或向设备中写的过程中，锁定缓冲快
	struct task_struct * b_wait;       // 指向等待该缓冲块解锁的任务
	struct buffer_head * b_prev;       // 指向 free_list 中的 前一个缓冲块
	struct buffer_head * b_next;       // 指向 free_list 中的 后一个缓冲块
	struct buffer_head * b_prev_free;  // 指向 hash_table 中的 前一个缓冲块
	struct buffer_head * b_next_free;  // 指向 hash_table 中的 后一个缓冲块
};


// 设备中的inode
// i_mode字段占2个字节，其中前9位表示三组文件权限，分别表示：
// 1 文件宿主权限 2 同组用户权限 3 其他用户权限
// i_mode 字段的高4位表示6中文件类型，分别为：
// "-" 正规文件
// "d" 目录文件
// "s" 符号链接
// "p" 命名管道
// "c" 字符设备
// "b" 块设备
struct d_inode {
	mix_short i_mode;           // 文件类型和属性
	mix_short i_uid;            // 文件宿主 id
	mix_long i_size;            // 文件长度（字节）
	mix_long i_mtime;           // 文件修改时间
	unsigned char i_gid;        // 文件宿主的组 id
	unsigned char i_nlinks;     // 文件的链接数（有多少个文件目录项指向该 inode)
	mix_short i_zone[9];        // 文件上所占用的盘上逻辑块数组，其中i_zone[0] - i_zone[6] 直接块号
};                              // i_zone[7] 一次间接块号， i_zone[8] 二次间接块号
                                // *？*？*？
                                // caution : 对于设备文件i节点，i_zone[0] 中存放的是设备号 
                                // *？*？*？
// 内存中的inode
struct m_inode {
	mix_short i_mode;
	mix_short i_uid;
	mix_long i_size;
	mix_long i_mtime;
	unsigned char i_gid;
	unsigned char i_nlinks;
	mix_short i_zone[9];
	/* these are in memory also */
	struct task_struct * i_wait;           // 指向等待该I节点的任务
	struct task_struct * i_wait2;	       // for pipes 
	mix_long i_atime;                      // 最后访问该 i 节点的时间
	mix_long i_ctime;                      // 该 i 节点自身被修改的时间
	mix_short i_dev;                       // i 节点所在的设备号
	mix_short i_num;                       // i 节点号，从 1 开始，
	mix_short i_count;                     // i 节点引用次数， 0 表示空闲
	unsigned char i_lock;                  // i 节点锁定标志
	unsigned char i_dirt;                  // i 节点 修改标志
	unsigned char i_pipe;                  // 1 表示 用作管道 i 节点
	unsigned char i_mount;                 // 1 表示该 i 节点安装了文件系统
	unsigned char i_seek;
	unsigned char i_update;                // 更新标志， 1 表示有效，0表示无效
};



struct super_block
{
	mix_short s_ninodes;                   // i 节点数
	mix_short s_nzones;                    // 数据块的个数 ， 逻辑快号从 0 开始编号
	mix_short s_imap_blocks;               // imap占的逻辑块数
	mix_short s_zmap_blocks;               // zmap占的逻辑块数
	mix_short s_firstdatazone;             // 第一个数据块的逻辑块编号
	mix_short s_log_zone_size;             // 逻辑块与磁盘块的比例，一个逻辑快包含几个磁盘快
	mix_long s_max_size;                   // 文件的最大长度（字节）
	mix_short s_magic;                     // 文件系统魔幻数（0x137f)
	// these items only exsit in memory
	struct buffer_head* s_imap[8];         // i节点位图在高速缓冲块指针数组
	struct buffer_head* s_zmap[8];         // z节点位图在高速缓冲块指针数组
	mix_short s_dev;                       // 超级块所在设备号
	struct m_inode* s_isup;                // 被安装文件系统根目录 i 节点
	struct m_inode* s_imount;              // 该文件系统安装到的 i 节点
	mix_long s_time;                       // 修改时间
	struct task_struct* s_wait;            // 等待该超级块的任务
	unsigned char s_lock;                  // 锁定标志
	unsigned char s_rd_only;               // 只读标志
	unsigned char s_dirt;                  // 修改标志
};
struct d_super_block
{
	mix_short s_ninodes;                   // i 节点数
	mix_short s_nzones;                    // 数据块的个数 ， 逻辑快号从 0 开始编号
	mix_short s_imap_blocks;               // imap占的逻辑块数
	mix_short s_zmap_blocks;               // zmap占的逻辑块数
	mix_short s_firstdatazone;             // 第一个数据块的逻辑块编号
	mix_short s_log_zone_size;             // 逻辑块与磁盘块的比例，一个逻辑快包含几个磁盘快
	mix_long s_max_size;                   // 文件的最大长度（字节）
	mix_short s_magic;                     // 文件系统魔幻数（0x137f)
 };

// 文件目录项结构
#define NAME_LEN 14
struct dir_entry
{
	mix_short inode;                      // 文件所对应的 i 节点号
	char name[NAME_LEN];                  // 文件名（14字节）
 };

// 文件表
struct file {
	mix_short f_mode;             // 文件操作模式
	mix_short f_flags;            // 文件打开和控制的标志
	mix_short f_count;            // 对应文件句柄的引用计数
	struct m_inode * f_inode;     // 指向内存中的 i 节点
	off_t f_pos;                  // 文件当前的读写指针的位置
};





// 对位进行操作的函数
struct bit_map {
	unsigned int total_bits;
	char* start_addr;
};
int find_first_zero(struct bit_map* bm);
int find_first_one(struct bit_map* bm);
int clear_bit(unsigned int offset, struct bit_map* bm);
int set_bit(unsigned int offset, struct bit_map* bm);
int test_bit(unsigned int offset, struct bit_map* bm);
void clear(struct bit_map* bm);
void set(struct bit_map* bm);


#define NR_FILE 64
#define NR_SUPER 8
#define NR_INODE 64
// 内存中的个表格
extern struct m_inode inode_table[NR_INODE];               // 内存中的 i 节点表
struct file file_table[NR_FILE];                           // 内存中的文件表
extern struct super_block super_block[NR_SUPER];           // 存放超级块的表

#define NR_BUFFERS nr_buffers
#define NR_HASH 307                                       // hashtable 的大小
extern struct buffer_head* first_buffer;
extern int nr_buffers;





#define DISK_SIZE 1024 * BLOCK_SIZE
#define BUFFER_SIZE 1024 * BLOCK_SIZE
#define FLOPPY_SIZE 1024 * BLOCK_SIZE
#define MEMORY_SIZE 1024 * BLOCK_SIZE
// 硬盘
char HARD_DISK[DISK_SIZE];
// 缓冲区
char BUFFER[BUFFER_SIZE];
// 软盘
char FLOPPY[FLOPPY_SIZE];
// 内存
char MEMORY[MEMORY_SIZE];

#define START_DISK HARD_DISK
#define DISK_END (HARD_DISK + DISK_SIZE)
#define START_BUFFER BUFFER
#define BUFFER_END (BUFFER + BUFFER_SIZE)
#define START_FLOPPY FLOPPY
#define FLOPPY_END (FLOPPY + FLOPPY_SIZE)
#define STRAT_MEMORY MEMORY
#define MEMORY_END (MEMORY + MEMORY_SIZE)








// bitmap.c
extern void ll_rw_block(int rw, struct buffer_head * bh);
extern int new_block(int dev);
extern int free_block(int dev, int block);
extern struct m_inode * new_inode(int dev);
extern void free_inode(struct m_inode * inode);

// buffer.c
void buffer_init();
extern struct buffer_head * get_hash_table(int dev, int block);
extern struct buffer_head * getblk(int dev, int block);
extern void brelse(struct buffer_head * buf);
extern struct buffer_head * bread(int dev, int block);
extern void bread_page(char* addr, int dev, int b[4]);
extern struct buffer_head * breada(int dev, int block, ...);
extern int sync_dev(int dev);
extern int sys_sync(void);
extern void inline invalidate_buffers(int dev);
extern void check_disk_change(int dev);

//
void truncate(struct m_inode * inode);

//inode.c
void sync_inodes(void);
extern void iput(struct m_inode * inode);
extern struct m_inode * iget(int dev, int nr);
extern struct m_inode * get_empty_inode(void);
extern struct m_inode * get_pipe_inode(void);
extern int bmap(struct m_inode * inode, int block);
extern int create_block(struct m_inode * inode, int block);

//dev_init.c
void device_init(int device);
void fs_init();


//super.c
void mount_root(void);
int sys_mount(char * dev_name, char * dir_name, int rw_flag);
int sys_umount(char * dev_name);
struct super_block* get_super(int dev);
void put_super(int dev);



// namei.c
struct m_inode * lnamei(const char * pathname);
struct m_inode * namei(const char * pathname);
int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);
int sys_mknod(const char * filename, int mode, int dev);
int sys_mkdir(const char * pathname, int mode);
int sys_rmdir(const char * name);
int sys_unlink(const char * name);
int sys_symlink(const char * oldname, const char * newname);
int sys_link(const char * oldname, const char * newname);
void sys_ls(const char* pathname, struct m_inode* base);


// open.c
int sys_chname(const char* old_name, const char* new_name);
#endif