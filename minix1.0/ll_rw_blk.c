#include"memory.h"
#include"file_sys.h"
#include"kernel.h"
#include<stdio.h>
#define NR_BLK_DEV 7
extern char* device_address[8][10];
typedef int(*FUNC)(int rw, struct buffer_head* bh);
extern FUNC make_request[NR_BLK_DEV] ;

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


// 硬盘设备驱动程序
int disk(int rw, struct buffer_head* bh)
{
	int dev = bh->b_dev;
	int blk_num = bh->b_blocknr;
	char* addr_disk = device_address[MAJOR(dev)][MINOR(dev)] + blk_num * BLOCK_SIZE;
	char* addr_buffer = bh->b_data;
	if (rw == READ)
	{
		memcpy(addr_buffer, addr_disk, BLOCK_SIZE);
		bh->b_uptodate = 1;
	}
	if (rw == WRITE)
	{
		memcpy(addr_disk, addr_buffer, BLOCK_SIZE);
		bh->b_dirt = 0;
	}
	return 0;
}

// 软盘设备驱动程序
int floppy(int rw, struct buffer_head* bh)
{
	int dev = bh->b_dev;
	int blk_num = bh->b_blocknr;
	char* addr_floppy = device_address[MAJOR(dev)][MINOR(dev)] + blk_num * BLOCK_SIZE;
	char* addr_buffer = bh->b_data;
	if (rw == READ)
	{
		memcpy(addr_buffer, addr_floppy, BLOCK_SIZE);
		bh->b_uptodate = 1;
	}
	if (rw == WRITE)
	{
		memcpy(addr_floppy, addr_buffer, BLOCK_SIZE);
		bh->b_dirt = 0;
	}
	return 0;
}

int disk_size[10] = { 0,DISK_SIZE };
int floppy_size[2] = { 0,FLOPPY_SIZE };
FUNC make_request[NR_BLK_DEV] = { 0,0,floppy,disk,0,0,0 };//
unsigned int* blk_size[7] = { 0,0,floppy_size,disk_size,0,0,0 };
// 所有设备的在内存的起始地址
char* device_address[8][10] = {
{ 0 },                                                                                                // 未用
{ 0 },                                                                                                // 内存
{ 0, START_FLOPPY},                                                                                   // 软盘
{ 0, START_DISK },                                                                                    // 硬盘
{ 0 },                                                                                                // ttyx 串行终端设备
{ 0 },                                                                                                // tty 终端设备
{ 0 },                                                                                                // 打印设备
{0 }                                                                                                  // 没有命名的管道
};
