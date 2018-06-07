#include"file_sys.h"
#include"kernel.h"
#include"stat.h"
#include<stdio.h>
#define NR_BLK_DEV 7
extern unsigned int device_address[8][10];
typedef int(*FUNC)(int rw, struct buffer_head* bh);
extern FUNC make_request[NR_BLK_DEV] ;
static char* path = "MINIX.txt";

FILE* NOODLE_FILE;
// 打开磁盘
int open_noodle_file(){
	int error;
	char buffer = 1;
	if ((NOODLE_FILE = fopen(path, "r+b")) == 0)
		return error;
	//fwrite(&buffer, 1, 1, NOODLE_FILE);
	return 1;
}

// 关闭磁盘
int close_noodle_file(){
	int error;
	if ((error = fclose(NOODLE_FILE)) < 0)
		return error;
	return 1;
}

// 块设备读写
void ll_rw_block(int rw, struct buffer_head * bh)
{
	unsigned int major;
	if ((major = MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
		!(make_request[MAJOR(bh->b_dev)])) {
		panic("Trying to read nonexistent block-device");
		return;
	}
	make_request[major](rw, bh);
}

// 
int read_from_file(char* buffer, unsigned int f_pos,
	unsigned int count){
	int error;
	if ((error = fseek(NOODLE_FILE, f_pos, SEEK_SET)) < 0)
		return error;
	while (count > 0){
		if ((error = fread(buffer, 1, count, NOODLE_FILE)) < 0)
			return error;
		count -= error;
	}
	return 1;
}

// 
int write_to_file(char* buffer, unsigned int f_pos,
	unsigned int count){
	int error;
	if ((error = fseek(NOODLE_FILE, f_pos, SEEK_SET)) < 0)
		return error;
	while (count > 0){
		if ((error = fwrite(buffer, 1, count, NOODLE_FILE)) < 0)
			return error;
		count -= error;
	}
	return 1;
}


// 硬盘设备驱动程序
int disk(int rw, struct buffer_head* bh)
{
	int dev = bh->b_dev, error;
	int blk_num = bh->b_blocknr;
	//char* addr_disk = device_address[MAJOR(dev)][MINOR(dev)] + blk_num * BLOCK_SIZE;
	unsigned int addr_disk = device_address[MAJOR(dev)][MINOR(dev)] 
		                     + blk_num * BLOCK_SIZE;
	char* addr_buffer = bh->b_data;
	if (rw == READ)
	{
		if ((error = read_from_file(addr_buffer, addr_disk,
			BLOCK_SIZE)) < 0)
			return error;
		// memcpy(addr_buffer, addr_disk, BLOCK_SIZE);
		bh->b_uptodate = 1;
	}
	if (rw == WRITE)
	{
		if ((error = write_to_file(addr_buffer, addr_disk,
			BLOCK_SIZE)) < 0)
			return error;
		// memcpy(addr_disk, addr_buffer, BLOCK_SIZE);
		bh->b_dirt = 0;
	}
	return 0;
}

// 软盘设备驱动程序
int floppy(int rw, struct buffer_head* bh)
{
	int dev = bh->b_dev, error;
	int blk_num = bh->b_blocknr;
	//char* addr_floppy = device_address[MAJOR(dev)][MINOR(dev)] + blk_num * BLOCK_SIZE;
	unsigned int addr_floppy = device_address[MAJOR(dev)][MINOR(dev)] 
		                       + blk_num * BLOCK_SIZE;
	char* addr_buffer = bh->b_data;
	if (rw == READ)
	{
		//memcpy(addr_buffer, addr_floppy, BLOCK_SIZE);
		if ((error = read_from_file(addr_buffer, addr_floppy,
			 BLOCK_SIZE)) < 0)
			return error;
		bh->b_uptodate = 1;
	}
	if (rw == WRITE)
	{
		//memcpy(addr_floppy, addr_buffer, BLOCK_SIZE);
		if ((error = write_to_file(addr_buffer, addr_floppy,
			BLOCK_SIZE)) < 0)
			return error;
		bh->b_dirt = 0;
	}
	return 0;
}
int disk_size[10] = { 0,DISK_SIZE };
int floppy_size[10] = { 0,FLOPPY_SIZE, ROOT_SIZE};
FUNC make_request[NR_BLK_DEV] = { 0,0,floppy,disk,0,0,0 };//
unsigned int* blk_size[7] = { 0,0,floppy_size,disk_size,0,0,0 };
// 所有设备的在内存的起始地址
//char* device_address[8][10] = {
unsigned int device_address[8][10] = {
{ 0 },                                                                                                // 未用
{ 0 },                                                                                                // 内存
{ 0, START_FLOPPY, START_ROOT},                                                                                   // 软盘
{ 0, START_DISK },                                                                                    // 硬盘
{ 0 },                                                                                                // ttyx 串行终端设备
{ 0 },                                                                                                // tty 终端设备
{ 0 },                                                                                                // 打印设备
{0 }                                                                                                  // 没有命名的管道
};
