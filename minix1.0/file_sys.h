#pragma once
#ifndef FILE_SYS_H
#define FILE_SYS_H
#include"mix_type.h"




#define BLOCK_SIZE 256


#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

// �����豸��
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



extern int ROOT_DEV;                                                           // ���ļ�ϵͳ���ڵ��豸
#define ROOT_INO 1                                                             // ��inode�ı��Ϊ 1
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))              // һ���߼�������� i �ڵ���
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))       // һ���߼��������Ŀ¼�����



#define NR_OPEN 20        // �������ܴ��ļ�������Ҳ���ļ����������ܸ���
 



//�ܵ�
#define PAGE_SIZE 4 * BLOCK_SIZE
#define PIPE_READ_WAIT(inode) ((inode).i_wait)
#define PIPE_WRITE_WAIT(inode) ((inode).i_wait2)
#define PIPE_HEAD(inode) ((inode).i_zone[0])
#define PIPE_TAIL(inode) ((inode).i_zone[1])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))



//imap �� zmap ��ռ����󻺳����
#define I_MAP_SLOTS 8
#define Z_MAP_SLOTS 8
#define SUPER_MAGIC 0x137F


#define get_fs_byte(addr) (*((char*)addr))
#define put_fs_byte(value,addr) (*((char*)addr) = (value))
// blk_size�����ָ�룬��ָ��ָ��һ�����飬
// �������д����Ӧ�豸�������Ĵ�С
extern unsigned int* blk_size[7];






struct buffer_head{
	char * b_data;			           // ָ�����ݿ��ָ��        
	mix_long b_blocknr;	               // �߼���Ŀ�ţ��� 0 ��ʼ��ţ�
	mix_short b_dev;		           // �߼������ڵ��豸�ţ�b_dev = 0 ��ʾ�û���鴦�ڿ���״̬
	unsigned char b_uptodate;          // ���±�־����ʾ�����Ƿ���£�1�����£���Ч����0����Ч��
	unsigned char b_dirt;		       // �޸ı�־����ʾ������е��������豸�е������Ƿ�һ�£�0��clean,δ�޸ģ��� 1��dirty, �޸�)
	unsigned char b_count;		       // ʹ�øÿ���û���         
	unsigned char b_lock;		       // ������Ƿ������� �ڴ��豸�ж������豸��д�Ĺ����У����������
	struct task_struct * b_wait;       // ָ��ȴ��û�������������
	struct buffer_head * b_prev;       // ָ�� free_list �е� ǰһ�������
	struct buffer_head * b_next;       // ָ�� free_list �е� ��һ�������
	struct buffer_head * b_prev_free;  // ָ�� hash_table �е� ǰһ�������
	struct buffer_head * b_next_free;  // ָ�� hash_table �е� ��һ�������
};


// �豸�е�inode
// i_mode�ֶ�ռ2���ֽڣ�����ǰ9λ��ʾ�����ļ�Ȩ�ޣ��ֱ��ʾ��
// 1 �ļ�����Ȩ�� 2 ͬ���û�Ȩ�� 3 �����û�Ȩ��
// i_mode �ֶεĸ�4λ��ʾ6���ļ����ͣ��ֱ�Ϊ��
// "-" �����ļ�
// "d" Ŀ¼�ļ�
// "s" ��������
// "p" �����ܵ�
// "c" �ַ��豸
// "b" ���豸
struct d_inode {
	mix_short i_mode;           // �ļ����ͺ�����
	mix_short i_uid;            // �ļ����� id
	mix_long i_size;            // �ļ����ȣ��ֽڣ�
	mix_long i_mtime;           // �ļ��޸�ʱ��
	unsigned char i_gid;        // �ļ��������� id
	unsigned char i_nlinks;     // �ļ������������ж��ٸ��ļ�Ŀ¼��ָ��� inode)
	mix_short i_zone[9];        // �ļ�����ռ�õ������߼������飬����i_zone[0] - i_zone[6] ֱ�ӿ��
};                              // i_zone[7] һ�μ�ӿ�ţ� i_zone[8] ���μ�ӿ��
                                // *��*��*��
                                // caution : �����豸�ļ�i�ڵ㣬i_zone[0] �д�ŵ����豸�� 
                                // *��*��*��
// �ڴ��е�inode
struct m_inode {
	mix_short i_mode;
	mix_short i_uid;
	mix_long i_size;
	mix_long i_mtime;
	unsigned char i_gid;
	unsigned char i_nlinks;
	mix_short i_zone[9];
	/* these are in memory also */
	struct task_struct * i_wait;           // ָ��ȴ���I�ڵ������
	struct task_struct * i_wait2;	       // for pipes 
	mix_long i_atime;                      // �����ʸ� i �ڵ��ʱ��
	mix_long i_ctime;                      // �� i �ڵ������޸ĵ�ʱ��
	mix_short i_dev;                       // i �ڵ����ڵ��豸��
	mix_short i_num;                       // i �ڵ�ţ��� 1 ��ʼ��
	mix_short i_count;                     // i �ڵ����ô����� 0 ��ʾ����
	unsigned char i_lock;                  // i �ڵ�������־
	unsigned char i_dirt;                  // i �ڵ� �޸ı�־
	unsigned char i_pipe;                  // 1 ��ʾ �����ܵ� i �ڵ�
	unsigned char i_mount;                 // 1 ��ʾ�� i �ڵ㰲װ���ļ�ϵͳ
	unsigned char i_seek;
	unsigned char i_update;                // ���±�־�� 1 ��ʾ��Ч��0��ʾ��Ч
};



struct super_block
{
	mix_short s_ninodes;                   // i �ڵ���
	mix_short s_nzones;                    // ���ݿ�ĸ��� �� �߼���Ŵ� 0 ��ʼ���
	mix_short s_imap_blocks;               // imapռ���߼�����
	mix_short s_zmap_blocks;               // zmapռ���߼�����
	mix_short s_firstdatazone;             // ��һ�����ݿ���߼�����
	mix_short s_log_zone_size;             // �߼�������̿�ı�����һ���߼�������������̿�
	mix_long s_max_size;                   // �ļ�����󳤶ȣ��ֽڣ�
	mix_short s_magic;                     // �ļ�ϵͳħ������0x137f)
	// these items only exsit in memory
	struct buffer_head* s_imap[8];         // i�ڵ�λͼ�ڸ��ٻ����ָ������
	struct buffer_head* s_zmap[8];         // z�ڵ�λͼ�ڸ��ٻ����ָ������
	mix_short s_dev;                       // �����������豸��
	struct m_inode* s_isup;                // ����װ�ļ�ϵͳ��Ŀ¼ i �ڵ�
	struct m_inode* s_imount;              // ���ļ�ϵͳ��װ���� i �ڵ�
	mix_long s_time;                       // �޸�ʱ��
	struct task_struct* s_wait;            // �ȴ��ó����������
	unsigned char s_lock;                  // ������־
	unsigned char s_rd_only;               // ֻ����־
	unsigned char s_dirt;                  // �޸ı�־
};
struct d_super_block
{
	mix_short s_ninodes;                   // i �ڵ���
	mix_short s_nzones;                    // ���ݿ�ĸ��� �� �߼���Ŵ� 0 ��ʼ���
	mix_short s_imap_blocks;               // imapռ���߼�����
	mix_short s_zmap_blocks;               // zmapռ���߼�����
	mix_short s_firstdatazone;             // ��һ�����ݿ���߼�����
	mix_short s_log_zone_size;             // �߼�������̿�ı�����һ���߼�������������̿�
	mix_long s_max_size;                   // �ļ�����󳤶ȣ��ֽڣ�
	mix_short s_magic;                     // �ļ�ϵͳħ������0x137f)
 };

// �ļ�Ŀ¼��ṹ
#define NAME_LEN 14
struct dir_entry
{
	mix_short inode;                      // �ļ�����Ӧ�� i �ڵ��
	char name[NAME_LEN];                  // �ļ�����14�ֽڣ�
 };

// �ļ���
struct file {
	mix_short f_mode;             // �ļ�����ģʽ
	mix_short f_flags;            // �ļ��򿪺Ϳ��Ƶı�־
	mix_short f_count;            // ��Ӧ�ļ���������ü���
	struct m_inode * f_inode;     // ָ���ڴ��е� i �ڵ�
	off_t f_pos;                  // �ļ���ǰ�Ķ�дָ���λ��
};





// ��λ���в����ĺ���
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
// �ڴ��еĸ����
extern struct m_inode inode_table[NR_INODE];               // �ڴ��е� i �ڵ��
struct file file_table[NR_FILE];                           // �ڴ��е��ļ���
extern struct super_block super_block[NR_SUPER];           // ��ų�����ı�

#define NR_BUFFERS nr_buffers
#define NR_HASH 307                                       // hashtable �Ĵ�С
extern struct buffer_head* first_buffer;
extern int nr_buffers;





#define DISK_SIZE 1024 * BLOCK_SIZE
#define BUFFER_SIZE 1024 * BLOCK_SIZE
#define FLOPPY_SIZE 1024 * BLOCK_SIZE
#define MEMORY_SIZE 1024 * BLOCK_SIZE
// Ӳ��
char HARD_DISK[DISK_SIZE];
// ������
char BUFFER[BUFFER_SIZE];
// ����
char FLOPPY[FLOPPY_SIZE];
// �ڴ�
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