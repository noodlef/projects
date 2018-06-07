#include"file_sys.h"
#include"kernel.h"
#include"mix_erro.h"
// 前向声明
extern int tty_read(unsigned minor, char * buf, int count);

extern int tty_write(unsigned minor, char * buf, int count);


// 定义字符设备读写函数指针类型
typedef (*crw_ptr)(int rw, unsigned minor, char * buf, int count, off_t * pos);

// 串口终端 设备 读写
static int rw_ttyx(int rw, unsigned minor, char * buf, int count, off_t * pos)
{
	return ((rw == READ) ? tty_read(minor, buf, count) :
		tty_write(minor, buf, count));
}

// 终端读写函数
static int rw_tty(int rw,unsigned minor,char * buf,int count, off_t * pos)
{
	// 检查进程是否有终端设备
	if (current->tty<0)
		return -EPERM;
	return rw_ttyx(rw, current->tty, buf, count, pos);
}

// 内存数据读写
static int rw_ram(int rw, char * buf, int count, off_t *pos)
{
	return -EIO;
}

// 物理内存数据读写操作函数
static int rw_mem(int rw, char * buf, int count, off_t * pos)
{
	return -EIO;
}

// 内核内存读写函数
static int rw_kmem(int rw, char * buf, int count, off_t * pos)
{
	return -EIO;
}


// 端口写
void outb(char* buf, int port)
{
	return;
}
// 端口 读
char inb(int port)
{
	return 0;
}
// 端口读写操作函数
static int rw_port(int rw, char * buf, int count, off_t * pos)
{
	int port = *pos;

	while (count-->0 && port<65536) {
		if (rw == READ)
			put_fs_byte(inb(port), buf++);
		else
			outb(get_fs_byte(buf++), port);
	}
	return 0;
}

//
static int rw_memory(int rw, unsigned minor, char * buf, int count, off_t * pos)
{
	switch (minor) {
	case 0:
		return rw_ram(rw, buf, count, pos);
	case 1:
		return rw_mem(rw, buf, count, pos);
	case 2:
		return rw_kmem(rw, buf, count, pos);
	case 3:
		return (rw == READ) ? 0 : count;	/* rw_null */
	case 4:
		return rw_port(rw, buf, count, pos);
	default:
		return -EIO;
	}
}


// 设备种数
#define NRDEVS ((sizeof (crw_table))/(sizeof (crw_ptr)))

static crw_ptr crw_table[] = {
	NULL,		/* nodev */
	rw_memory,	/* /dev/mem etc */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	rw_ttyx,	/* /dev/ttyx */
	rw_tty,		/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL };		/* unnamed pipes */


int rw_char(int rw, int dev, char * buf, int count, off_t * pos)
{
	crw_ptr call_addr;

	if (MAJOR(dev) >= NRDEVS)
		return -ENODEV;
	if (!(call_addr = crw_table[MAJOR(dev)]))
		return -ENODEV;
	return call_addr(rw, MINOR(dev), buf, count, pos);
}
