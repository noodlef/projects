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


// Ӳ���豸��������
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

// �����豸��������
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
// �����豸�����ڴ����ʼ��ַ
char* device_address[8][10] = {
{ 0 },                                                                                                // δ��
{ 0 },                                                                                                // �ڴ�
{ 0, START_FLOPPY},                                                                                   // ����
{ 0, START_DISK },                                                                                    // Ӳ��
{ 0 },                                                                                                // ttyx �����ն��豸
{ 0 },                                                                                                // tty �ն��豸
{ 0 },                                                                                                // ��ӡ�豸
{0 }                                                                                                  // û�������Ĺܵ�
};
