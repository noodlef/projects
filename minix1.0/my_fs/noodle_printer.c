/*
* printer 打印文件
*/
#include"mix_window.h"
#include"file_sys.h"
#include"fcntl.h"
#include"stat.h"
#include"kernel.h"
#include"mix_erro.h"
#include<string.h>

static int txt_format(unsigned int from_fd, unsigned int to_fd);
static int binary_format(unsigned int from_fd, unsigned int to_fd);
int(*print_with_format[5])(unsigned int from_fd, unsigned int to_fd)
= {txt_format, binary_format};

int sys_open(const char * filename, int flag, int mode);
int sys_read(unsigned int fd, char * buf, int count);
int sys_write(unsigned int fd, char * buf, int count);
int sys_close(unsigned int fd);
int sys_lseek(unsigned int fd, mix_off_t offset, int origin);
void print_char(struct mix_char* m_char, struct tty_window* tty);
extern int sys_unlink(const char * name);
static char rw_buffer[BLOCK_SIZE];


// 打开文件 MT_TAIL 和文件 MT_HEAD
// 并清屏

int printer_aux_1(unsigned int minor) {
	struct tty_window* tty = tty_dev[minor];
	int fd, mode, flag;

	flag = O_CREAT | O_RDWR;
	mode = S_IRWXU | S_IRWXG | S_IRWXO;
	
	if ((fd = sys_open("MT_HEAD.ext", flag, mode)) < 0)
		return fd;
	tty->fd_set[MT_HEAD] = fd; // 0 号tty
	if ((fd = sys_open("MT_TAIL.ext", flag, mode)) < 0)
		return fd;
	tty->fd_set[MT_TAIL] = fd; // 0 号tty
	// 清屏并重置显示缓冲区
	clc_win(minor);
	tty->button |= S_PRINTER;// XXXXXX 处于打印机模式
	return 1;
}

// 关闭文件 MT_TAIL 和文件 MT_HEAD
int printer_aux_2(unsigned int minor) {
	struct tty_window* tty = tty_dev[minor];
	
	clc_win(minor);
	// 删除文件, 忽略错误
	if (sys_close(tty->fd_set[MT_HEAD]) >= 0)
		sys_unlink("MT_HEAD.ext");
	if (sys_close(tty->fd_set[MT_TAIL]) >= 0)
		sys_unlink("MT_TAIL.ext");
	return 1;
}

// 以字节的方式将文件读出
static
int read_byte(unsigned int fd, unsigned int minor){
	struct tty_window* tty = tty_dev[minor];
	struct file* m_file = current->filp[fd];
	int left = m_file->f_inode->i_size, count;
	struct mix_char m_char[20];
	char m_buf[20];
	int i, error;

	// 定位到文件尾
	if ((error = sys_lseek(fd, 0, SEEK_END)) < 0)
		return error;
	// 定位到文件头
	if ((error = sys_lseek(tty->fd_set[MT_TAIL], 0, SEEK_SET)) < 0)
		return error;
	// 以字节为单位读文件
	while (left > 0){
		count = 20;
		// XXXXXX, 反序读出字节
		if (left < count)
			count = left;
		if ((error = sys_lseek(fd, -count, SEEK_CUR)) < 0)
			return error;
		// XXXXXX,要么读出 count 个字节，要么返回错误
		if ((count = sys_read(fd, m_buf, count)) < 0)
			return count;
		left -= count;
		if ((error = sys_lseek(fd, -count, SEEK_CUR)) < 0)
			return error;
		for (i = 0; count > 0;){
			m_char[i].b_color = m_char[i].b_set = 0;
			m_char[i].f_color = CFC_White;
			m_char[i++].ch = m_buf[--count];
		}
		if ((error = sys_write(tty->fd_set[MT_TAIL], (char*)m_char,
			i * sizeof(struct mix_char))) < 0)
			return error;
	}
	return 1;
}

// 该函数在tty_io.c 文件中
int read_from_fd(struct mix_char* m_buf, int count, unsigned int fd);
// 显示
static
int printer_aux_init(unsigned int minor){
	struct tty_window* tty = tty_dev[minor];
	unsigned int fd = tty->fd_set[MT_TAIL];
	int count, left, pos, i;
	struct file* m_file = current->filp[fd];
	struct mix_char m_buf[WINDOW_WIDTH * 2];

	pos = tty->buf.free_list;
	tty->buf.cur_head = tty->buf.s_head = pos;
	left = m_file->f_pos / sizeof(struct mix_char);
	if (left > tty->win_height * tty->win_width)
		left = tty->win_height * tty->win_width + 1;  // XXXXXX + 1
	while (left > 0){
		count = tty->win_width * 2;
		if ((count = read_from_fd(m_buf, count, fd)) < 0)
			return count;
		left -= count;
		i = 0;
		while (count-- > 0){
			tty->buf.m_buf[pos].m_char = m_buf[i++];
			pos = tty->buf.m_buf[pos].next;
		}
	}
	tty->buf.cur_tail = tty->buf.s_tail = pos;
	tty->buf.free_list = tty->buf.m_buf[pos].next;
	// 将缓冲区中字符显示出来
	// XXXXX 此处将 cur_tail 设置为 s_tail 没问题
	// 因为 s_flush 函数会根据实际情况将 cur_tail 更新屏幕显示的尾部
	s_flush(tty);
	return 1;
}

static
int wait_for_cmd(unsigned int minor){
	int ch;
	struct tty_window* tty = tty_dev[minor];
	while ((ch = keybord(tty)) != KEY_ESC);
	return 1;
}
// 
int sys_printer_aux(unsigned int R_fd){
	int error_code = 1;
	unsigned int minor = 1;
	struct tty_window* tty = tty_dev[minor];

	// tty初始化
	if ((error_code = printer_aux_1(minor)) < 0) {
		 printer_aux_2(minor);
		 return error_code;
	}
	// 从文件中读出数据
	if ((error_code = read_byte(R_fd, minor)) < 0){
		printer_aux_2(minor);
		return error_code;
	}
	// 文件截零
	if ((error_code = sys_close(R_fd)) < 0) 
		return error_code;
	if ((error_code = sys_open("/dev/printer.ext", O_TRUNC, 0)) < 0)
		return error_code;
	if ((error_code = sys_close(R_fd)) < 0)
		return error_code;
	// 将tty中的数据显示出来
	if ((error_code = printer_aux_init(minor)) < 0){
		printer_aux_2(minor);
		return error_code;
	}
	error_code = wait_for_cmd(minor);
	printer_aux_2(minor);
	return error_code;
}


static int txt_format(unsigned int from_fd, unsigned int to_fd)
{
	char buf[1024];
	int error, left, count;

	left = current->filp[from_fd]->f_inode->i_size;
	if ((error = sys_lseek(from_fd, 0, SEEK_SET)) < 0)
		return error;
	if ((error = sys_lseek(to_fd, 0, SEEK_SET)) < 0)
		return error;
	while (left > 0){
		count = 1024;
		if ((count = sys_read(from_fd, buf, count)) < 0)
			return count;
		left = left - count;
		while (count > 0){
			if ((error = sys_write(to_fd, buf, count)) < 0)
				return error;
			count -= error;
		}
	}
	return 1;
}

static int binary_format(unsigned int from_fd, unsigned int to_fd)
{
	char buf[1024], line[1024], byte;
	int error, left, count, i, j, k;

	left = current->filp[from_fd]->f_inode->i_size;
	if ((error = sys_lseek(from_fd, 0, SEEK_SET)) < 0)
		return error;
	if ((error = sys_lseek(to_fd, 0, SEEK_SET)) < 0)
		return error;
	while (left > 0){
		count = 1024;
		if ((count = sys_read(from_fd, buf, count)) < 0)
			return count;
		left = left - count;
		j = 0;
		while (count-- > 0){
			i = 0;
			byte = buf[j++];
			for (k = 0; k < 8; k++){
				if ((byte >> k) & 0x01)
					line[i++] = '1';
				else
					line[i++] = '0';
			}
			line[i++] = ' ';
			if ((i = sys_write(to_fd, line, i)) < 0)
				return i;
		}
	}
	return 1;
}

extern int sys_readlink(const char * path, char * buf, int bufsiz);
int sys_printer(const char * filename, const char* mode){
	const char* p_filename = "/dev/printer.ext";
	int fd, p_fd, flag, error,
		f_mode, format;
	char buf[1024];

	if (!strcmp(mode, "a"))
		format = 0;
	else if (!strcmp(mode, "b"))
		format = 1;
	else
		return -ERROR;
        flag = O_TRUNC | O_CREAT | O_RDWR;
	f_mode = S_IRWXU | S_IRWXG | S_IRWXO;
	// 判断是否符号链接
	if ((error = sys_readlink(filename, buf, 1023)) >= 0){
		buf[error] = 0;
		M_printf(buf);
		return M_printf("\n");
	}
	if ((fd = sys_open(filename, O_RDONLY, 0)) < 0)
		return fd;
	if ((p_fd = sys_open(p_filename, flag, f_mode)) < 0){
		mix_cerr("can't open printer/n");
		return p_fd;
	}
	if ((error = print_with_format[format](fd, p_fd)) < 0)
		return error;
	if ((error = sys_close(fd)) < 0)
		return error;
	return sys_printer_aux(p_fd);
}
