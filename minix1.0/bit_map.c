#include"kernel.h"
#include"memory.h"
void init_bit_map(unsigned int sz, char* addr, struct bit_map* bm)
{
	bm->total_bits = sz;
	bm->start_addr = addr;
	// 应该分配的总字节数
	//unsigned int bytes = sz / 8 + 1;
	// 清零
	//memset(addr, 0, bytes);
}
//
int find_first_zero(struct bit_map* bm)
{
	unsigned int bytes = bm->total_bits / 8;
	for (unsigned int i = 0; i != bytes; ++i)
	{
		// ((1 << 8) - 1)) = 0x (1 1 1 1 1 1 1 1)
		if (*(bm->start_addr + i) == (unsigned char)((1 << 8) - 1))
			continue;
		for (int j = 0; j != 8; ++j) {
			unsigned char test = 1 << j;
			// 测试第 i 位是否为 0
			if (!(test & *(bm->start_addr + i))) {
				unsigned int ret = i * 8 + j;
				return ret;
			}
		}
	}
	return bm->total_bits;
}
//	
int find_first_one(struct bit_map* bm)
{
	unsigned int bytes = bm->total_bits / 8;
	for (unsigned int i = 0; i != bytes; ++i)
	{
		if (*(bm->start_addr + i) == (unsigned char)0)
			continue;
		for (int j = 0; j != 8; ++j) {
			unsigned char test = 1 << j;
			// 测试第 i 位是否为 1
			if (test & *(bm->start_addr + i)) {
				unsigned int ret = i * 8 + j;
				return ret;
			}
		}
	}
	return bm->total_bits;
}

//将offset指定的位置清零
int clear_bit(unsigned int offset, struct bit_map* bm)
{
	unsigned char var = 1;
	unsigned int byte_offset, bit_offset;
	unsigned char* tmp;
	byte_offset = offset / 8;
	bit_offset = offset & 7;
	tmp = (unsigned char*)bm->start_addr + byte_offset;
	var = 1 << bit_offset;
	// 检测相应位是否为0
	if (!(var & *tmp))
	{
		panic("trying to clear bit(0)\n");
		//printf("tring to clear bit(0)\n");
		return 1;
	}
	var = ((1 << 8) - 1) - var; // 取反
	*tmp = *tmp & var;
	return 0;
}
//将offset指定的位置置位
int set_bit(unsigned int offset, struct bit_map* bm)
{
	unsigned char var = 1;
	unsigned int byte_offset, bit_offset;
	unsigned char* tmp;
	byte_offset = offset / 8;
	bit_offset = offset & 7;
	tmp = (unsigned char*)bm->start_addr + byte_offset;
	var = 1 << bit_offset;
	// 检测相应位是否为1
	if (var & *tmp)
	{
		panic("trying to clear bit(1)\n");
		//printf("tring to set bit(1)\n");
		return 1;
	}
	*tmp = *tmp | var;
	return 0;
}
//测试相应位是否为1， 如果是1返回true;
int test_bit(unsigned int offset, struct bit_map* bm)
{
	unsigned char var = 1;
	unsigned int byte_offset, bit_offset;
	unsigned char* tmp;
	byte_offset = offset / 8;
	bit_offset = offset & 7;
	tmp = (unsigned char*)bm->start_addr + byte_offset;
	var = 1 << bit_offset;
	if (var & *tmp)
		return 1;
	return 0;
}
// 将所有位全部清零
void clear(struct bit_map* bm)
{
	unsigned int bytes = bm->total_bits / 8;
	memset(bm->start_addr, 0, bytes);
}
// 将所有位全部置位
void set(struct bit_map* bm)
{
	unsigned int bytes = bm->total_bits / 8;
	// 255 = 0x(1 1 1 1 1 1 1 1)
	memset(bm->start_addr, 255, bytes);
}