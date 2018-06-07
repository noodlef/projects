#include"kernel.h"
#include"memory.h"
void init_bit_map(unsigned int sz, char* addr, struct bit_map* bm)
{
	bm->total_bits = sz;
	bm->start_addr = addr;
	// Ӧ�÷�������ֽ���
	//unsigned int bytes = sz / 8 + 1;
	// ����
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
			// ���Ե� i λ�Ƿ�Ϊ 0
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
			// ���Ե� i λ�Ƿ�Ϊ 1
			if (test & *(bm->start_addr + i)) {
				unsigned int ret = i * 8 + j;
				return ret;
			}
		}
	}
	return bm->total_bits;
}

//��offsetָ����λ������
int clear_bit(unsigned int offset, struct bit_map* bm)
{
	unsigned char var = 1;
	unsigned int byte_offset, bit_offset;
	unsigned char* tmp;
	byte_offset = offset / 8;
	bit_offset = offset & 7;
	tmp = (unsigned char*)bm->start_addr + byte_offset;
	var = 1 << bit_offset;
	// �����Ӧλ�Ƿ�Ϊ0
	if (!(var & *tmp))
	{
		panic("trying to clear bit(0)\n");
		//printf("tring to clear bit(0)\n");
		return 1;
	}
	var = ((1 << 8) - 1) - var; // ȡ��
	*tmp = *tmp & var;
	return 0;
}
//��offsetָ����λ����λ
int set_bit(unsigned int offset, struct bit_map* bm)
{
	unsigned char var = 1;
	unsigned int byte_offset, bit_offset;
	unsigned char* tmp;
	byte_offset = offset / 8;
	bit_offset = offset & 7;
	tmp = (unsigned char*)bm->start_addr + byte_offset;
	var = 1 << bit_offset;
	// �����Ӧλ�Ƿ�Ϊ1
	if (var & *tmp)
	{
		panic("trying to clear bit(1)\n");
		//printf("tring to set bit(1)\n");
		return 1;
	}
	*tmp = *tmp | var;
	return 0;
}
//������Ӧλ�Ƿ�Ϊ1�� �����1����true;
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
// ������λȫ������
void clear(struct bit_map* bm)
{
	unsigned int bytes = bm->total_bits / 8;
	memset(bm->start_addr, 0, bytes);
}
// ������λȫ����λ
void set(struct bit_map* bm)
{
	unsigned int bytes = bm->total_bits / 8;
	// 255 = 0x(1 1 1 1 1 1 1 1)
	memset(bm->start_addr, 255, bytes);
}