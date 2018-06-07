/*
 * vim 实现文件添加， 删除，查看等功能
 */
#include"mix_window.h"
#include"file_sys.h"
#include"fcntl.h"
#include"stat.h"
#include"kernel.h"
#include"mix_erro.h"
#include<string.h>

#define VIM_SAVE 6
#define VIM_DISPOSE 7
extern int M_date(int t, char* buf, int len);
extern int sys_open(const char * filename, int flag, int mode);
extern int sys_read(unsigned int fd, char * buf, int count);
extern int sys_write(unsigned int fd, char * buf, int count);
extern int sys_close(unsigned int fd);
extern int sys_lseek(unsigned int fd, mix_off_t offset, int origin);
extern void print_char(struct mix_char* m_char, struct tty_window* tty);
extern int sys_unlink(const char * name);
static char rw_buffer[BLOCK_SIZE];


// 以 txt格式将文件读出
static 
int read_txt_format(unsigned int fd, unsigned int minor){
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

static 
int write_txt_format(struct mix_char* m_char, unsigned int count,
unsigned int S_fd){
	char m_buf[100];
	int i, left = count;
	for (i = 0; i < count; i++)
		m_buf[i] = m_char[i].ch;
	while (left > 0){
		if ((count = sys_write(S_fd, m_buf, left)) < 0)
			return count;
		left -= count;
	}
	return 1;
}


// 保存文件
static 
int save_file(unsigned int minor, unsigned int S_fd){
	struct tty_window* tty = tty_dev[minor];
	struct mix_char m_char[20], m_buf[20];
	struct file* m_file;
	int count, i,j, left, error;
	int H_fd = tty->fd_set[MT_HEAD];
	int T_fd = tty->fd_set[MT_TAIL];

	// 1
	m_file = current->filp[H_fd];
	left = m_file->f_pos / sizeof(struct mix_char);
	if ((error = sys_lseek(H_fd, 0, SEEK_SET)) < 0)
		return error;
	while (left > 0){
		count = 20;
		if (left < count)
			count = left;
		// XXXXXX,要么读出 count 个字节，要么返回错误
		if ((count = sys_read(H_fd, (char*)m_buf,
			count * sizeof(struct mix_char))) < 0)
			return count;
		count = count / sizeof(struct mix_char);
		left -= count ;
		for (i = 0, j = 0; i < count; i++){
			//if (m_buf[i].ch != 0)//XXXXXX
			if (!IS_NEWLINE(m_buf[i]))
				m_char[j++] = m_buf[i];
		}
		if ((error = write_txt_format(m_char, j, S_fd)) < 0)
			return error;
	}
	// 2
	count = 0;
	i = tty->buf.s_head;
	j = tty->buf.s_tail;
	while (i != j){
		m_char[count++] = tty->buf.m_buf[i].m_char;
		i = tty->buf.m_buf[i].next;
		if (count >= 20 || (i == j)){
			if ((error = write_txt_format(m_char, count, S_fd)) < 0)
				return error;
			count = 0;
		}
	}
	// 3
	m_file = current->filp[T_fd];
	left = m_file->f_pos / sizeof(struct mix_char);
	while (left > 0){
		count = 20;
		// XXXXXX, 反序读出字节
		if (left < count)
			count = left;
		count = count * sizeof(struct mix_char);
		if ((error = sys_lseek(T_fd, -count, SEEK_CUR)) < 0)
			return error;
		// XXXXXX,要么读出 count 个字节，要么返回错误
		if ((count = sys_read(T_fd, (char*)m_buf, count )) < 0)
			return count;
		if ((error = sys_lseek(T_fd, -count, SEEK_CUR)) < 0)
			return error;
		count = count / sizeof(struct mix_char);
		left -= count ;
		for (i = 0; count > 0;)
			m_char[i++] = m_buf[--count];
		if ((error = write_txt_format(m_char, i,S_fd)) < 0)
			return error;
	}
	return 1;
}

// 该函数在tty_io.c 文件中
int read_from_fd(struct mix_char* m_buf, int count, unsigned int fd);
// 由 vim 函数调用
int vim_aux_init(unsigned int minor){
	struct tty_window* tty = tty_dev[minor];
	unsigned int fd = tty->fd_set[MT_TAIL];
	int count, left, pos, i;
	struct file* m_file = current->filp[fd];
	struct mix_char m_buf[WINDOW_WIDTH * 2];

	pos = tty->buf.free_list;
	tty->buf.cur_head = tty->buf.s_head = pos;
	left = m_file->f_pos / sizeof(struct mix_char);
	if (left > tty->win_height * tty->win_width)
		left = tty->win_height * tty->win_width + 1;
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



int vim_keybord();
int delete_buf(struct tty_window* tty);
// 本函数实现文本编辑
int vim_edit(unsigned int minor){
	struct tty_window* tty = tty_dev[minor];
	struct mix_char m_char;
	int ch, error, beg, end;
	int rows, i, j, select;
	enum { SELECT = 3, SAVE = 1, QUIT = 2, RETURN = 3};
	char str[5][20] = {{"save"}, {"quit"},{"return"}};

	// 按下 ESC 则退出
repeat:
	while ((ch = keybord(tty)) != KEY_ESC) {
		if (ch == KEY_BACKSPACE){
			delete_buf(tty);
			continue;
		}
		// 没有设置文本高亮显示
		if (!IS_THLIGHT(tty->button))
			m_char.b_set = 0;
		else {
			m_char.b_set = 1;
			m_char.b_color = tty->char_b_color;
		}
		m_char.ch = ch;
		m_char.f_color = tty->f_color;
		// 写入
		tty_write(minor, &m_char, 1);
	}

	// 等待输入命令, 最后一行
	if ((rows = get_line_ht(tty, tty->buf.m_buf[tty->buf.cur_tail].prev,
		&beg, &end)) >= (tty->win_height - 1)){
		// 清空最后一行
		tty->current_pos.x = tty->origin_pos.x;
		tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1 ;
		m_char.ch = m_char.b_set = 0;
		m_char.f_color = CFC_White;
		for (i = 0; i < tty->win_width; i++){
			print_char(&m_char, tty);
			++tty->current_pos.x;
		}
	}

	// 等待选择
	select = 1, ch = SBUF_LEFT;
	do{
		switch (ch){
		case SBUF_LEFT:
			if (select > 1)
				--select;
			break;
		case SBUF_RIGHT:
			if (select < SELECT)
				++select;
			break;
		default:
			break;
		}
		tty->current_pos.x = tty->origin_pos.x;
		tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
		for (i = 0; i < SELECT; i++){
			for (j = 0; j < strlen(str[i]); j++){
				if (tty->current_pos.x > tty->win_width)
					break;
				m_char.ch = str[i][j];
				m_char.f_color = CFC_White;
				if ((i + 1) == select){
					m_char.b_color = CBC_Blue;
					m_char.b_set = 1;
				}
				else
					m_char.b_color = m_char.b_set = 0;
				print_char(&m_char, tty);
				++tty->current_pos.x;
			}
			// 空格
			for (j = 0; j < 2; j++){
				if (tty->current_pos.x > tty->win_width)
					break;
				m_char.ch = m_char.f_color = 0;
				m_char.b_color = m_char.b_set = 0;
				print_char(&m_char, tty);
				++tty->current_pos.x;
			}
			
		}
	} while ((ch = vim_keybord()) != KEY_ENTER);

	// 根据选择做出相应的处理
	switch (select){
	case SAVE:
		error = VIM_SAVE;
		break;
	case QUIT:
		error = VIM_DISPOSE;
		break;
	case RETURN:
			// 清空最后一行
			tty->current_pos.x = tty->origin_pos.x;
			tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
			m_char.ch = m_char.b_set = 0;
			m_char.f_color = CFC_White;
			for (i = 0; i < tty->win_width; i++){
				print_char(&m_char, tty);
				++tty->current_pos.x;
			}
			// 恢复最后一行
			if (rows >= (tty->win_height - 1)){
				// 最后一行
				tty->current_pos.x = tty->origin_pos.x;
				tty->current_pos.y = tty->origin_pos.y + tty->win_height - 1;
				for (; beg != end; beg = tty->buf.m_buf[beg].next){
					print_char(&tty->buf.m_buf[beg].m_char, tty);
					++tty->current_pos.x;
				}
			}
			cursor_flash(tty);
		goto repeat;
	default:
		error = VIM_DISPOSE;
	}
	return error;
}

//
int vim_aux(unsigned int fd, unsigned int S_fd)
{
	unsigned int minor = 0;
	int error_code, tmp;
	struct tty_window* tty = tty_dev[minor];

	// 屏幕显示
	if ((error_code = read_txt_format(fd, minor)) < 0)
		return error_code;
	if ((error_code = vim_aux_init(minor)) < 0)
		return error_code;
	// 进行编辑，按下 ESC 退出编辑模式
	if ((error_code = vim_edit(minor)) < 0)
		return error_code;
    // 是否保存文件
	if (error_code == VIM_SAVE){
		if ((tmp = save_file(minor, S_fd)) < 0)
			error_code = tmp;
		return  error_code;
	}
	return error_code;
}

// 保存屏幕内容
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
	//tty->win_height -= 3;
	tty->button &= ~S_MODE; // 添加模式
	tty->button |= S_VIM;   // 处于vim模式
	tty->button &= ~S_HIGHLIGHT; // 非高亮
	tty->b_color = CBC_Black;
	tty->f_color = CFC_White;
	return 1;
}

// 恢复屏幕内容
int vim_aux_2(unsigned int minor, unsigned int R_fd) {
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
	return ;
}

int sys_vim(const char* filename){
	int error_code = 1, tmp, fd, tmp_fd, select;
	//const char* tmp_file = "@tmp_file";    
	char tmp_file[128];
	// 以读写方式打开，若文件不存在就创建
	int flag = O_RDWR | O_CREAT;
	// 若创建文件， 则文件属性设置如下
	short mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
	unsigned int R_fd, minor = 0;

	// 临时文件名等于原文件名末尾加一个@字符
	for (tmp = 0; tmp < strlen(filename) && tmp < 126; ++tmp)
		tmp_file[tmp] = *(filename + tmp);
	tmp_file[tmp] = '@';
	tmp_file[++tmp] = '\0';
	// 打开要编辑的文件和临时文件
	if ((fd = sys_open(filename, flag, mode)) < 0)
		return fd;
	// 检查是否正规文件
	if (!S_ISREG(current->filp[fd]->f_mode)){
		sys_close(fd);
		return -EPERM;
	}
	// 创建临时文件用于保存文件
	if ((tmp_fd = sys_open(tmp_file, flag, mode)) < 0)
		return tmp_fd;
	// 保存屏幕内容
	if (vim_aux_1(minor, &R_fd) < 0)
		return vim_aux_2(minor, R_fd);
	// 对文件进行编辑
	if ((select = vim_aux(fd, tmp_fd)) < 0){
		error_code = select;
		goto label;
	}
	// 关闭文件
	if ((error_code = sys_close(fd)) < 0)
		goto label;
	if ((error_code = sys_close(tmp_fd)) < 0)
		goto label;
	if (select == VIM_SAVE){
		// 删除文件
		if ((error_code = sys_unlink(filename)) < 0){
			sys_unlink(tmp_file);
			goto label;
		}
		if ((error_code = sys_chname(tmp_file, filename)) < 0)
			goto label;
	}
	else{
		// 删除临时文件
		if ((tmp = sys_unlink(tmp_file)) < 0)
			error_code = tmp;
	}
label:
	// 恢复屏幕内容
	if ((tmp = vim_aux_2(minor, R_fd)) < 0){
		clc_win(minor);
		error_code = tmp;
	}
	// 刷新高速缓冲区
	sys_sync();
	return error_code;
}


// 保存显示缓冲区的内容
int sys_save_sbuf(char* pathname, unsigned int minor) {
	int mode = S_IRWXU| S_IRWXG | S_IRWXO | S_IFREG, S_fd;
	int i, j, count, error;
	struct tty_window* tty = tty_dev[minor];
	struct mix_char m_char[32];
	char buf[32];

	if ((S_fd = sys_open(pathname, O_RDWR | O_CREAT, mode)) < 0)
		return S_fd;
	if ((error = sys_lseek(S_fd, 0, SEEK_END)) < 0)
		return error;
	if((count = M_date(CURRENT_TIME, buf + 1, 30)) < 0)
		return count;
	buf[0] = buf[++count] = KEY_ENTER;
	++count;
	for (i = 0; i < count; i++) {
		m_char[i].ch = buf[i];
		m_char[i].b_set = 0;
		m_char[i].f_color = CFC_Red;
	}
	if ((error = write_txt_format(m_char, count, S_fd)) < 0)
		return error;
	// 
	count = 0;
	i = tty->buf.s_head;
	j = tty->buf.s_tail;
	while (i != j) {
		m_char[count++] = tty->buf.m_buf[i].m_char;
		i = tty->buf.m_buf[i].next;
		if (count >= 20 || (i == j)) {
			if ((error = write_txt_format(m_char, count, S_fd)) < 0)
				return error;
			count = 0;
		}
	}
	if ((error = sys_close(S_fd)) < 0)
		return error;
	return 1;
}
