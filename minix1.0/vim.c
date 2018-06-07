/*
 * vim 实现文件添加， 删除，查看等功能
 */
#include"mix_window.h"
#include"file_sys.h"
#include"fcntl.h"
#include"stat.h"
#include"kernel.h"

int sys_open(const char * filename, int flag, int mode);
int sys_read(unsigned int fd, char * buf, int count);
int sys_write(unsigned int fd, char * buf, int count);
int sys_close(unsigned int fd);
int sys_lseek(unsigned int fd, off_t offset, int origin);
void print_char(struct mix_char* m_char, struct tty_window* tty);
extern int sys_unlink(const char * name);
static char rw_buffer[BLOCK_SIZE];



static void write_buf(char* buf, int count)
{

}

int vim_aux(unsigned int fd, unsigned int tmp_fd)
{
	int minor = 0, ch, save;
	int error_code;
	struct file* m_file = current->filp[fd];
	int left = m_file->f_inode->i_size, count;
	struct tty_window* tty = tty_dev[minor];
	struct cursor_pos cursor_pos;
	struct mix_char m_char = {0, CFC_Purple, CBC_Blue,1};
	// 如果文件不为空， 则将文件中的字符读入显示缓冲区
	while(left) {
		if((count = sys_read(fd, rw_buffer, BLOCK_SIZE)) < 0)
			return (error_code = count);
		// 写入缓冲区
		write_buf(rw_buffer, count);
		left -= count;
	}
	// 将缓冲区中字符显示出来
	// XXXXX 此处将 cur_tail 设置为 s_tail 没问题
	// 因为 s_flush 函数会根据实际情况将 cur_tail 更新屏幕显示的尾部
	tty->buf.cur_tail = tty->buf.s_tail;
	s_flush(tty);
	
	// 按下 ESC 则退出
	repeat:
	while ((ch = keybord(tty)) != KEY_ESC) {
		if(ch == KEY_BACKSPACE)
			delete_buf(tty);
		if (ch < 32 || ch  > 128)
			continue;
		// 没有设置文本高亮显示
		if (!IS_THLIGHT(tty->button))
			m_char.b_set = 0;
		else {
			m_char.b_set = 1;
			m_char.b_color = tty->char_b_color;
		}
		m_char.f_color = tty->f_color;
		// 写入
		tty_write(minor, &m_char, 1);
	}
	// 等待输入命令, 最后一行
	cursor_pos = tty->current_pos;
	{
		int count = tty->win_width, lock, head, tail, pos;
		struct cursor_pos  tmp_pos;
		tty->current_pos.x = tty->origin_pos.x;
		tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
		while (count-- > 0) {
			print_char(&m_char, tty);
			++tty->current_pos.x;
		}
		m_char.b_set = 0;
		tty->current_pos.x = tty->origin_pos.x;
		tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
		//
		m_char.ch = 'y';
		print_char(&m_char, tty);
		++tty->current_pos.x;
		m_char.ch = '-';
		print_char(&m_char, tty);
		++tty->current_pos.x;
		m_char.ch = 'n';
		print_char(&m_char, tty);
		++tty->current_pos.x;
		m_char.ch = '-';
		print_char(&m_char, tty);
		++tty->current_pos.x;
		m_char.ch = 'b';
		print_char(&m_char, tty);
		++tty->current_pos.x;
		//
		lock = 1;
		while (lock) {
			ch = get_key();
			if (ch < 32 || ch > 128)
				continue;
			switch (ch) {
			case 'y':
				lock = 0;
				save = 1;
				break;
			case 'n':
				lock = 0;
				save = 0;
				break;
			case 'b':
				// 清空
				m_char.ch = 0;
				m_char.b_set = 0;
				tty->current_pos.x = tty->origin_pos.x;
				tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
				while (count-- > 0) {
					print_char(&m_char, tty);
					++tty->current_pos.x;
				}
				// 如果有字符就显示
				tty->current_pos.x = tty->origin_pos.x;
				tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
				if (tty->buf.s_tail == tty->buf.s_head)
					goto repeat;
				pos = tty->buf.m_buf[tty->buf.cur_tail].prev;
				get_line_ht(tty, pos, &head, &tail);
				tmp_pos = map_to_screen(tty, head);
				// 有字符
				if (tmp_pos.y == tty->current_pos.y) {
					for (; head != tty->buf.cur_tail; head = tty->buf.m_buf[head].next) {
						print_char(&m_char, tty);
						++tty->current_pos.x;
					}
				}
				// 恢复光标位置
				tty->current_pos = cursor_pos;
				cursor_flash(tty);
				goto repeat;
			default:
				break;
			}
		}

	}
	// 保存文件
	if (save) {
		return 1;
	}
	else
		return -1;
}

int sys_vim(const char* filename){
	int error_code = 1,tmp, fd, tmp_fd;
	const char* tmp_file = "@tmp_file";
	// 以读写方式打开，若文件不存在就创建
	int flag = O_RDWR | O_CREAT;
	// 若创建文件， 则文件属性设置如下
	short mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
	// 打开文件
	if((fd = sys_open(filename, flag, mode)) < 0)
		return (error_code = fd);
	// 创建临时文件用于保存文件
	if ((tmp_fd = sys_open(tmp_file, flag, mode)) < 0) {
		// 关闭文件
		if((tmp = sys_close(fd)) < 0)
			error_code = tmp;
		return error_code = tmp_fd;
	}
	// 对文件进行编辑
	error_code = vim_aux(fd, tmp_fd);
	// 关闭文件
	if ((tmp = sys_close(fd)) < 0)
		error_code = tmp;
	if ((tmp = sys_close(tmp_fd)) < 0)
		error_code = tmp;

	if (error_code  < 0) 
		// 删除临时文件
		if((tmp = sys_unlink(tmp_file)) < 0)
			error_code = tmp;
	else {
		// 删除文件
		if((tmp = sys_unlink(filename) < 0))
			error_code = tmp;
		if((sys_chname(tmp_file, filename)) < 0 )
			error_code = tmp;
	}
	return error_code;
}



int vim_aux_1(unsigned int minor, unsigned int* R_fd) {
	struct tty_window* tty = tty_dev[minor];
	int fd, mode, flag, left, count;
	char* buf = (char*)tty;

	flag = O_CREAT | O_RDWR;
	mode = S_IRWXU | S_IRWXG | S_IRWXO;
	// 保存现场
	if ((*R_fd = sys_open("tty.mix", flag, mode)) < 0)
		return *R_fd;
	left = sizeof(struct tty_window);
	while (left > 0) {
		if ((count = sys_write(*R_fd, buf, left)) < 0)
			return count;
		left -= count;
		buf += count;
	}
	if ((fd = sys_open("file_1", flag, mode)) < 0)
		return fd;
	tty->fd_set[MT_HEAD] = fd; // 0 号tty
	if ((fd = sys_open("file_2", flag, mode)) < 0)
		return fd;
	tty->fd_set[MT_TAIL] = fd; // 0 号tty
	// 清屏并重置显示缓冲区
	clc_win(minor);
	return 1;
}



void vim_aux_2(unsigned int minor, unsigned int R_fd) {
	struct tty_window* tty = tty_dev[minor];
	char* buf = (char*)tty;
	int left = sizeof(struct tty_window), count;

	// 删除文件, 忽略错误
	if(sys_close(tty->fd_set[MT_HEAD]) >= 0)
		sys_unlink("file_1");
	if(sys_close(tty->fd_set[MT_TAIL]) >= 0)
		sys_unlink("file_2");
	// 恢复现场
	// 检查 fd 是否是打开的文件描述符
	if (R_fd < 0 || R_fd >= NR_OPEN)
		return 0;
	if (!current->filp[R_fd]->f_inode)
		return 0;
	if (current->filp[R_fd]->f_pos < left) {
		if (sys_close(R_fd) >= 0)
			sys_unlink("tty.mix");
		return 0;
	}
	sys_lseek(R_fd, 0, SEEK_SET);
	while (left > 0) {
		if ((count = sys_read(R_fd, buf, left)) < 0)
			return count;
		left -= count;
		buf += count;
	}
	// 删除文件
	if (sys_close(R_fd) >= 0)
		sys_unlink("tty.mix");
	// 刷新显示缓冲区
	left = tty->buf.s_pos;
	s_flush(tty);
	tty->buf.s_pos = left;
	cursor_flash(tty);
	return 0;
}


void vim_test() {
	unsigned int R_fd;

	vim_aux_1(0, &R_fd);
	vim_aux_2(0, R_fd);
	return 0;
}