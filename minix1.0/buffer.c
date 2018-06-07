#include"file_sys.h"
#include<stdio.h>
#include"memory.h"
#include"stdarg.h"
#include"kernel.h"

struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;
static struct task_struct * buffer_wait = 0;
static struct buffer_head * first_buffer;
int NR_BUFFERS = 0;

// ���ٻ�����ʼ��
void buffer_init()
{
	struct buffer_head* beg = (struct buffer_head*)START_BUFFER;
	char* end = BUFFER_END;
	int i;
	while ((end -= BLOCK_SIZE) >= (char*)(beg + 1)) 
	{
		beg->b_dev = 0;
		beg->b_dirt = 0;
		beg->b_count = 0;
		beg->b_lock = 0;
		beg->b_uptodate = 0;
		beg->b_wait = 0;
		beg->b_next = 0;
		beg->b_prev = 0;
		beg->b_data = end;
		beg->b_prev_free = beg - 1;
		beg->b_next_free = beg + 1;
		beg++;
		NR_BUFFERS++;
	}
	beg--;
	free_list = (struct buffer_head*)START_BUFFER;
	first_buffer = free_list;
	free_list->b_prev_free = beg;
	beg->b_next_free = free_list;
	for (i = 0; i<NR_HASH; i++)
		hash_table[i] = 0;
}
 
// ���߼�������ݴ� from ���� �� to
void copy_block(char* from, char* to)
{
	memcpy(to, from, BLOCK_SIZE);
}

// hash function ��ϣ����
#define _hashfn(dev,block) (((unsigned int)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]


// �����ٻ����� free_list �� hash_table ���Ƴ�
static inline void remove_from_queues(struct buffer_head* bh)
{
	/* remove from hash-queue */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev, bh->b_blocknr) == bh)
		hash(bh->b_dev, bh->b_blocknr) = bh->b_next;
	/* remove from free list */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)
		free_list = bh->b_next_free;
}

// �����ٻ������뵽 free_list �� hash_table 
static inline void insert_into_queues(struct buffer_head * bh)
{
	/* put at end of free list */
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
	/* put the buffer in new hash-queue if it has a device */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev, bh->b_blocknr);
	hash(bh->b_dev, bh->b_blocknr) = bh;
	if(bh->b_next)
		bh->b_next->b_prev = bh;
}

// �����豸�źͿ�Ų��Ҹ��ٻ�����Ƿ���hash_table ��
static struct buffer_head * find_buffer(int dev, int block)
{
	struct buffer_head * tmp;
	for (tmp = hash(dev, block); tmp != NULL; tmp = tmp->b_next)
		if (tmp->b_dev == dev && tmp->b_blocknr == block)
			return tmp;
	return NULL;
}

struct buffer_head* get_hash_table(int dev, int block)
{
	struct buffer_head * bh;
	for (;;) {
		if (!(bh = find_buffer(dev, block)))
			return NULL;
		bh->b_count++;
		//wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;
		bh->b_count--;
	}
}

// ��ָ���豸�ڸ��ٻ��������߼�����Ϊ��Ч
void inline invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;
	bh = first_buffer;
	for (i = 0; i<NR_BUFFERS; i++, bh++) {
		if (bh->b_dev != dev)
			continue;
		//wait_on_buffer(bh);
		if (bh->b_count >= 1)
			panic("invalidate_buffers failed\n");
		if (bh->b_dev == dev)
		{
			bh->b_uptodate = 0;
			bh->b_dirt = 0;
		}
	}
}

// ��ָ���豸�ڸ��ٻ������߼�������̽���ͬ��
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;
	bh = first_buffer;
	for (i = 0; i<NR_BUFFERS; i++, bh++) {
		if (bh->b_dev != dev)
			continue;
		//wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt && bh->b_uptodate)
			ll_rw_block(WRITE, bh);
	}
	sync_inodes();
	bh = first_buffer;
	for (i = 0; i<NR_BUFFERS; i++, bh++) {
		if (bh->b_dev != dev)
			continue;
		//wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt && bh->b_uptodate)
			ll_rw_block(WRITE, bh);
	}
	return 0;
}

// �����е� i �ڵ� �� ���ٻ���� ����ͬ��
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;
	sync_inodes();		
	bh = first_buffer;
	for (i = 0; i<NR_BUFFERS; i++, bh++) {
		//wait_on_buffer(bh);
		if (bh->b_dirt && bh->b_uptodate)
			ll_rw_block(WRITE, bh);
	}
	return 0;
}

// �����Ƭ�Ƿ񱻸���
extern void invalidate_inodes(int dev); // �ú���λ�� inode.c
extern int device_change(int dev); // �ú���λ�� dev_init.c
void check_disk_change(int dev)
{
	int i;

	// ��ǰ��֧�� ���� �� Ӳ�� ���ƶ�����
	// ������Ǿ�ֱ�ӷ���
	if (MAJOR(dev) != 2 && MAJOR(dev) != 3)
		return;
	// ��� �����Ƿ񱻸���
	if (!device_change(dev & 0x03))
		return;
	// ������
	for (i = 0; i<NR_SUPER; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);
	invalidate_buffers(dev);
}

// �����豸�źͿ�Ż�ȡ����Ӧ�ĸ��ٻ����
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
struct buffer_head * getblk(int dev, int block)
{
	struct buffer_head * tmp, *bh;
	if (bh = get_hash_table(dev, block))
		return bh;
	tmp = free_list;
	do {
		if (tmp->b_count)
			continue;
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {
			bh = tmp;
			if (!BADNESS(tmp))
				break;
		}
	} while ((tmp = tmp->b_next_free) != free_list);
	if (!bh)
		panic("there is no buffer\n");
	if (bh->b_dirt)
		sync_dev(bh->b_dev);
	bh->b_count = 1;
	bh->b_dirt = 0;
	bh->b_uptodate = 0;
	remove_from_queues(bh);
	bh->b_dev = dev;
	bh->b_blocknr = block;
	insert_into_queues(bh);
	return bh;
}

// �ͷŸ��ٻ����
void brelse(struct buffer_head * buf)
{
	if (!buf)
		return;
	//wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer\n");
	//wake_up(&buffer_wait);
}

// �����̿�������ٻ������� �� ���ü��� = 1
struct buffer_head * bread(int dev, int block)
{
	struct buffer_head * bh;
	if (!(bh = getblk(dev, block)))
		panic("bread: getblk returned NULL\n");
	if (bh->b_uptodate)
		return bh;
	ll_rw_block(READ, bh);
	//wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return NULL;
}


// �Ӹ��ٻ�������ȡһ���ڴ�ҳ�Ĵ�������
void bread_page(char* address, int dev, int b[4])
{
	struct buffer_head * bh[4];
	int i;
	for (i = 0; i<4; i++)
		if (b[i]) {
			if (bh[i] = getblk(dev, b[i]))
				if (!bh[i]->b_uptodate)
					ll_rw_block(READ, bh[i]);
		}
		else
			bh[i] = NULL;
	for (i = 0; i<4; i++, address += BLOCK_SIZE)
		if (bh[i]) {
			//wait_on_buffer(bh[i]);
			if (bh[i]->b_uptodate)
				copy_block(bh[i]->b_data, address);
			brelse(bh[i]);
		}
}

// Ԥ������߼���
struct buffer_head * breada(int dev, int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;
	va_start(args, first);
	if (!(bh = getblk(dev, first)))
		panic("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block(READ, bh);
	while ((first = va_arg(args, int)) >= 0) {
		tmp = getblk(dev, first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA, bh);
			brelse(tmp);
		}
	}
	va_end(args);
	//wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return (NULL);
}
