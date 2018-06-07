#include"mix_window.h"
// 3 ���ն���Ϊ��������ն�
int MIX_STD_CERR = 0;
int MIX_STD_OUT = 0;
int MIX_STD_IN = 0;
//extern int mix_cerr(unsigned dev, const char* s);
extern int up_line(struct tty_window* tty, int pos, int lines);
struct cursor_pos D_CURSOR_POS = { WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1 };
struct cursor_pos origin_pos_tty[4];
struct tty_window t_window[4];
struct tty_window* tty_dev[4] =
{
	t_window,                               // Ψһ�������ն�tty                 minor = 0
	t_window + 1,                           // �ն� lp ��ӡ��                
	t_window + 2,                           // �ն� ttyx
	t_window + 3,                           // �ն� unamed ������ʾ�����Ĵ���    minor = 3
};
void tty_init()
{
	int i,j,k,W, H;
	int flag;
	struct cursor_pos cur;
	// �߿����� ��ɫ���ַ������
	struct win_fram fm = { '*', FRAM_WIDTH, CFC_White, CBC_Black };
	// ����ʼλ���趨
	origin_pos_tty[0].x = DIS + FRAM_WIDTH;
	origin_pos_tty[0].y = DIS + FRAM_WIDTH;
	origin_pos_tty[1].x = DIS + 3 * FRAM_WIDTH + FRAM_GAP + TTY_WIDTH;
	origin_pos_tty[1].y = DIS + FRAM_WIDTH ;
	origin_pos_tty[2].x = DIS + FRAM_WIDTH;
	origin_pos_tty[2].y = DIS + TTY_HEIGHT +  FRAM_GAP + 3 * FRAM_WIDTH;
	origin_pos_tty[3].x = DIS + 3 * FRAM_WIDTH + FRAM_GAP + TTY_WIDTH;
	origin_pos_tty[3].y = DIS + TTY_HEIGHT +  FRAM_GAP + 3 * FRAM_WIDTH;
	// ���ĸ�tty���ڽ��г�ʼ��
	for (i = 0; i < 4; i++) {
		t_window[i].current_pos = t_window[i].origin_pos = origin_pos_tty[i];
		t_window[i].fram = fm;
		t_window[i].win_height = TTY_HEIGHT;
		t_window[i].win_width = TTY_WIDTH;
		// ���������Ĭ����ʾ��ɫ
		t_window[i].b_color = CBC_Black;
		t_window[i].f_color = CFC_White;
		t_window[i].char_b_color = CFC_White;
		t_window[i].button = 0;
		//
		t_window[i].buf.cur_head = t_window[i].buf.cur_tail = 0;
		t_window[i].buf.s_head = t_window[i].buf.s_tail =  0;
		t_window[i].buf.s_size = TTY_HEIGHT * TTY_WIDTH * 2 + 10;
	}
	// �������
	strcpy_s(t_window[0].name,9, "tty");
	strcpy_s(t_window[1].name,9, "lp");
	strcpy_s(t_window[2].name,9, "ttyx");
	strcpy_s(t_window[3].name,9, "unamed");
	// ����̨���ڳ�ʼ��
	window_init(WINDOW_WIDTH, WINDOW_HEIGHT);
	// ���Ʊ߿�
	for (i = 0; i < 4; i++) {
		cur.x = t_window[i].origin_pos.x - FRAM_WIDTH;
		cur.y = t_window[i].origin_pos.y - FRAM_WIDTH;
		W = t_window[i].win_width + 2 * FRAM_WIDTH;
		H = t_window[i].win_height + 2 * FRAM_WIDTH;
		for (j = 0; j < FRAM_WIDTH; j++)
		{
			flag = 1;
			for (k = 0; k < W; k++) {
				set_cursor_position(cur.x, cur.y);
				if (flag)
				{
					printf("%c", t_window[i].fram.shape);
					flag = 0;
				}
				else
					flag = 1;
				cur.x++;
			}
			cur.x--;
			for (k = 0; k < H; k++){
				set_cursor_position(cur.x, cur.y);
				printf("%c", t_window[i].fram.shape);
				cur.y++;
			}
			cur.y--;
			flag = 1;
			for (k = 0; k < W; k++) {
				set_cursor_position(cur.x, cur.y);
				if (flag)
				{
					printf("%c", t_window[i].fram.shape);
					flag = 0;
				}
				else
					flag = 1;
				cur.x--;
			}
			cur.x++;
			for (k = 0; k < H; k++) {
				set_cursor_position(cur.x, cur.y);
				printf("%c", t_window[i].fram.shape);
				cur.y--;
			}
			cur.y++;
			H -= 2;
			W -= 2;
			cur.x++;
			cur.y++;
		}
		// ��ʾ�ն˵�����
		k = (t_window[i].win_width - strlen(t_window[i].name)) / 2;
		cur.x += k;
		cur.y -= (FRAM_WIDTH + 1);
		set_cursor_position(cur.x, cur.y);
		set_console_color(CFC_Blue, CBC_White);
		printf(t_window[i].name);
		set_console_color_d();
	}
	set_cursor_position_d();
}

// �ظ�tty ���λ�� Ϊ��ʼλ��
static
void reset_tty_cursor(struct tty_window* tty)
{
	tty->current_pos = tty->origin_pos;
}

// �ı���Ļ��ʾ�������ɫ
int set_char_fcolor(unsigned minor, enum CF_color f_color)
{
	struct tty_window* tty = tty_dev[minor];
	tty->f_color = f_color;
	return 0;
}

// �ı���Ļ��ʾ����ı�����ɫ
int set_char_bcolor(unsigned minor, enum CB_color b_color)
{
	struct tty_window* tty = tty_dev[minor];
	tty->button = 1;
	tty->char_b_color = b_color;
	return 0;
}

// �ı���Ļ�ı�����ɫ
int set_tty_color(unsigned minor, enum CB_color b_color)
{
	struct tty_window* tty = tty_dev[minor];
	tty->b_color = b_color;
	return 0;
}


// ��ȷ - 1�� ���� - 0
static check_sbuf_pos(struct tty_window* tty, int pos)
{
	struct screen_buf* sc_buf = &(tty->buf);
	if (sc_buf->s_head == sc_buf->s_tail)
		return 0;
	if (sc_buf->s_head < sc_buf->s_tail) {
		if (pos < sc_buf->s_head || pos > sc_buf->s_tail)
			return 0;
	}
	else {
		if (pos > sc_buf->s_tail && pos < sc_buf->s_head)
			return 0;
	}
	return 1;
}

// ��鵱ǰ����λ���Ƿ���ȷ
static check_tty_cursor(struct tty_window* tty)
{
	int x = tty->current_pos.x;
	int y = tty->current_pos.y;
	if (x < tty->origin_pos.x || x >= (tty->origin_pos.x + tty->win_width)
		|| y < tty->origin_pos.y || y >= (tty->origin_pos.y + tty->win_height))
		return 0;
	return 1;
}

// ����ʾ�������� posָ�� ���� n ��
int up_line(struct tty_window* tty, int pos, int lines)
{
	// һ�������԰������ַ���
	int per_line = tty->win_width, i;
	struct screen_buf* src_buf = &(tty->buf);
	// �������
	if (!check_sbuf_pos(tty, pos))
		return -1;
	if (pos == src_buf->s_head)
		return pos;
	i = 1;
	while (lines > 0) {
		if (--pos < 0)
			pos = src_buf->s_size - 1;
		if (pos == src_buf->s_head)
			return pos;
		if (i != 1 && src_buf->s_buf[pos].ch == KEY_ENTER) {
			if (++pos >= src_buf->s_size)
				pos = 0;
			--lines;
			i = 1;
			continue;
		}
		if (i++ >= per_line) {
			--lines;
			i = 1;
		}
	}
	return pos;
}

// ��������ָ������  n ��
int down_line(struct tty_window* tty, int pos, int lines)
{
	// һ�������԰������ַ���
	int per_line = tty->win_width, k;
	struct screen_buf* src_buf = &(tty->buf);
	// �������
	if (!check_sbuf_pos(tty, pos))
		return -1;
	if (pos == src_buf->s_tail)
		return pos;
	while (lines-- > 0) {
		for (k = 0; k < tty->win_width; k++) {
			if (pos == src_buf->s_tail)
				return pos;
			if (src_buf->s_buf[pos++].ch == KEY_ENTER) {
				if (pos >= src_buf->s_size)
					pos = 0;
				break;
			}
			if (pos >= src_buf->s_size)
				pos = 0;
		}
	}
	return pos;
}


//  ����Ļ�����һ���ַ�
static
void print_char(struct mix_char* m_char, struct tty_window* tty) {
	struct cursor_pos pos = tty->current_pos;
	set_cursor_position(pos.x, pos.y);
	if (m_char->b_set)
		set_console_color(m_char->f_color, m_char->b_color);
	else
		set_console_color(m_char->f_color, tty->b_color);
	printf("%c", m_char->ch);
	return;
}
extern int s_flush(struct tty_window* tty);//A
// ����ʾ�������������������Ļ
static
int tty_print(struct tty_window* tty, int beg, int end)
{
	int ret = 0;
	int c;
	//int beg = tty->buf.cur_head;
	//int end = tty->buf.cur_tail;
	int k = tty->win_height - 1;
	struct mix_char* buf = tty->buf.s_buf;
	// �����Ļ��ǰ����λ��
	if (!check_tty_cursor(tty))
		return 0;
	while (beg != end) {
		c = (buf + beg)->ch;
		if(c != KEY_ENTER)
			print_char(buf + beg, tty);
		++ret;
		if (++beg >= tty->buf.s_size)
			beg = 0;
		// ������һ��
		if (c == KEY_ENTER || ++tty->current_pos.x >= (tty->origin_pos.x + tty->win_width)) {
			tty->current_pos.x = tty->origin_pos.x;
			if (++tty->current_pos.y >= (tty->win_height + tty->origin_pos.y)) {
				--tty->current_pos.y;
				break;
			}
			// ���������
			//if (++tty->current_pos.y >= (tty->origin_pos.y + tty->win_height))
			//	set_cursor_position(tty->current_pos.x, tty->current_pos.y - 1);
			//else
			//	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
			//set_console_color(tty->f_color, tty->f_color);
			//for (k = 0; k < tty->win_width; k++)
			//	printf(" ");
			//// ����Ѿ�������Ļ�����½ǣ� ˵����Ļ�Ѿ�û�ռ��ڴ�ӡ�ַ�
			//// ���������Ϊָ��λ��,����Ļ����������һ��
			//if (tty->current_pos.y >= (tty->origin_pos.y + tty->win_height)) {
			//	/*k = tty->win_height - 1;
			//	if (((tty->buf.cur_head = up_line(tty, beg, k)) < 0))
			//		return -1;*/

			//	k = tty->buf.cur_head;
			//	if (((tty->buf.cur_head = down_line(tty, k, 1)) < 0))
			//		return -1; 

			//	k = tty->buf.cur_tail;
			//	tty->buf.cur_tail = beg;
			//	// ��������¶�λ����Ļ��ʼ��
			//	reset_tty_cursor(tty);
			//	set_console_color(tty->f_color, tty->b_color);
			//	for (i = 0; i < tty->win_height; i++) {
			//		set_cursor_position(tty->origin_pos.x, tty->origin_pos.y + i);
			//		for (j = 0; j < tty->win_width; j++)
			//			printf(" ");
			//	}
			//	s_flush(tty);
			//	tty->buf.cur_tail = k;
			//}
		}
		// ���ù��λ��
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	}
	return ret;
}



// ˢ����ʾ������
int s_flush(struct tty_window* tty)
{
	struct screen_buf* sc_buf = &(tty->buf);
	int i, j;
	if (sc_buf->s_head == sc_buf->s_tail
		|| sc_buf->cur_head == sc_buf->cur_tail)
		return 0;
	// ��黺����
	if (!check_sbuf_pos(tty, sc_buf->cur_head) ||
		!check_sbuf_pos(tty, sc_buf->cur_tail))
		return 0;
	// ������Ļ���
	set_console_color(CFC_White, tty->b_color);
	for (i = 0; i < tty->win_height; i++) {
		set_cursor_position(tty->origin_pos.x, tty->origin_pos.y + i);
		for (j = 0; j < tty->win_width; j++)
			printf(" ");
	}
	// �ָ�����ó�ʼλ��
	reset_tty_cursor(tty);
	// ���ַ��������Ļ
	tty_print(tty, tty->buf.cur_head, tty->buf.cur_tail);
	return 0;
}

// ����ʾ��������д����
int write_screen_buf(struct mix_char* buf, int count, struct tty_window* tty)
{
	int beg,i,k, line_head, line_tail, rows;
	struct screen_buf* sc_buf = &(tty->buf);
	if (count <= 0)
		return 0;
	// ��������û����
	i = 0;
	if (sc_buf->s_head == sc_buf->s_tail) {
		sc_buf->s_buf[sc_buf->s_tail++] = buf[i++];
		count--;
		if (sc_buf->s_tail >= sc_buf->s_size)
			sc_buf->s_tail = 0;
		sc_buf->cur_tail = sc_buf->s_tail;
		// ������ַ�
		if ((beg = (sc_buf->cur_tail -1)) < 0)
			beg = 0;
		tty_print(tty, beg, sc_buf->cur_tail);

	}
	while (count-- > 0) {
		sc_buf->s_buf[sc_buf->s_tail++] = buf[i++];
		if (sc_buf->s_tail >= sc_buf->s_size)
			sc_buf->s_tail = 0;
		sc_buf->cur_tail = sc_buf->s_tail;
	    // ������ͷָ��ָ����һ��
		if (sc_buf->s_head == sc_buf->s_tail) //                   bug  12.21 --  downline ���� -1���������ʱ����
		{
			k = sc_buf->s_tail;
			if (--sc_buf->s_tail < 0)
				sc_buf->s_tail = sc_buf->s_size - 1;
			sc_buf->s_head = down_line(tty, sc_buf->s_head, 1);
			sc_buf->s_tail = k;
		}
		// ������ַ�
		if ((beg = (sc_buf->cur_tail - 1)) < 0)
			beg = 0;
		tty_print(tty, beg, sc_buf->cur_tail);
		if ((rows = get_line_ht(tty, sc_buf->s_tail, &line_head, &line_tail)) < 0)
			return -1;
		if (rows >= tty->win_height) {
			sc_buf->cur_head = down_line(tty, sc_buf->cur_head, 1);
			s_flush(tty);
		}
	}
	return 0;
}


// �ն˶�
int tty_read(unsigned minor, char * buf, int count)
{
	// ��ȡ�ն�
	int i,j, beg;
	struct mix_char c;
	struct tty_window* tty = tty_dev[minor];
	if(!check_tty_cursor(tty)){
		mix_cerr("tty_read : out of boundary!");
		return -1;
	}
	// 0 ���ն���ΪΨһ���������ն�
	i = 0;
	beg = tty->buf.s_tail;
	while (i <= count) {
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		c.ch = keybord(tty);
		// û����������ı�����ɫ
		if (!tty->button) 
			c.b_set = 0;
		else {
			c.b_set = 1;
			c.b_color = tty->char_b_color;
		}
		c.f_color = tty->f_color;
		tty_write(minor, &c, 1);
		i++;
		if (c.ch == KEY_ENTER) 
			break;
	}
	for (j = 0; j < i;) {
		buf[j++] = tty->buf.s_buf[beg++].ch;
		if (beg >= tty->buf.s_size)
			beg = 0;
	}
	return i;
}

// ��ӡ��ǰ�Ĺ���Ŀ¼
int print_cur_dir()
{
	unsigned int minor = 0;
	struct mix_char buf[50];
	int i, count = 0;
	buf[0].ch = '[';
	buf[1].ch = '@';
	buf[2].ch = 'n';
	buf[3].ch = 'o';
	buf[4].ch = 'o';
	buf[5].ch = 'd';
	buf[6].ch = 'l';
	buf[7].ch = 'e';
	buf[8].ch = ']';
	buf[9].ch = ':';
	buf[10].ch = ' ';
	count = 11;
	for (i = 0; i < count; i++) {
		buf[i].b_set = 0;
		buf[i].f_color = CFC_Purple;
	}
	tty_write(minor, buf, count);
	return 0;
}

int mix_print(struct mix_char* buf, int count, unsigned minor) {
	struct tty_window* tty = tty_dev[minor];
	write_screen_buf(buf, count, tty);
	return 0;
}
 

// �ն�д
int tty_write(unsigned minor, struct mix_char * buf, int count)
{
	// ��ȡ�ն�
	struct tty_window* tty = tty_dev[minor];
	int half_scr = tty->win_height / 2, rows = 0;
	if (!check_tty_cursor(tty)){
		mix_cerr("tty_read : out of boundary!");
		return -1;
	}
	// ����ʾ������д
	// ������ʾ
	/*if (!minor) {
		if (tty->buf.cur_head != tty->buf.cur_tail)
			if ((rows = get_line_ht(tty, tty->buf.cur_tail, &i, &j)) < 0)
				return -1;
		if (rows >= half_scr) {
			tty->buf.cur_head = down_line(tty, tty->buf.cur_head, 1);
			s_flush(tty);
		}
	}*/
	write_screen_buf(buf, count, tty);
	return 0;
}

// ����
int new_line(unsigned minor)
{
	struct tty_window* tty = tty_dev[minor];
	struct mix_char c = { KEY_ENTER, tty->f_color, tty->b_color, 0};
	tty_write(minor, &c, 1);
	return 0;
}

// �����������ʾ������
int clc_win(unsigned minor) {
	struct tty_window* tty = tty_dev[minor];
	struct screen_buf* src_buf = &(tty->buf);
	int i, j;
	set_console_color(tty->f_color, tty->b_color);
	for (i = 0; i < tty->win_height; i++) {
		set_cursor_position(tty->origin_pos.x, tty->origin_pos.y + i);
		for(j = 0; j < tty->win_width; j++)
			printf(" ");
	}
	tty->current_pos = tty->origin_pos;
	src_buf->cur_head = src_buf->cur_tail = 0;
	src_buf->s_head = src_buf->s_tail = 0;
	return 0;
}

// �������
int mix_cerr(const char* const s) {
	enum {SZ = 50};
	int minor = MIX_STD_CERR;
	int left = strlen(s),count, i = 0, j, k;
	struct tty_window* tty = tty_dev[minor];
	struct mix_char buf[SZ];
	while (left > 0) {
		if (left > SZ) 
			count = SZ;
		else 
			count = left;
		left -= count;
		k = count;
		for (j = 0; k-- > 0;) {
			buf[j].ch = s[i++];
			buf[j].f_color = CFC_Red;
			buf[j++].b_set = 0;
		}
		tty_write(minor, buf, count);
	}
	new_line(minor);
	return 0;
}

// ������������� һ���ַ��� ���� һ���ַ�
// ���� һ�� ������ һ��

// �ҳ���ǰλ�������е���β �� ��ͷ
int get_line_ht(struct tty_window* tty, int pos, int* head, int* tail)
{
	int flag, rows = -1;
	struct screen_buf* src = &tty->buf;
	if (!check_sbuf_pos(tty, pos))
		return -1;
	int line1 =  src->cur_head, line2, i;    // int line1 =  src->s_head, line2, i;
	while (((line2 = down_line(tty, line1, 1)) != src->s_tail)
		&& line2 > 0) {
		++rows;
		if (((line1 < line2) && (pos >= line1 && pos < line2)) ||
			((line1 > line2) && (pos >= line1 || pos < line2)))
		{
			*head = line1;
			if (--line2 < 0)
				line2 = src->s_size - 1;
			*tail = line2;
			return rows;
		}
		line1 = line2;
	}
	++rows;
	if (line2 < 0)
		return -1;
	*head = line1;
	*tail = line2;
	flag = 0;
	for (i = 0; line1 != line2; i++) {
		if (src->s_buf[line1].ch == KEY_ENTER) {
			flag = 1;
			break;
		}
		if (line1++ >= src->s_size)
			line1 = 0;
	}
	if (i >= tty->win_width)
		flag = 1;
	if (flag) {
		if (pos == *tail) {
			*head = *tail;
			++rows;
		}
		else
			if (--*tail < 0)
				*tail = src->s_size - 1;
	}
	return rows;
}

// ���󷵻� -1��
int move_sbuf_cursor(int direc, struct tty_window* tty, int pos) {
	struct screen_buf* src = &tty->buf;
	int head, tail, dis;
	if (!check_sbuf_pos(tty, pos))
		return -1;
	switch (direc) {
	    case SBUF_UP:
			// 
			if (get_line_ht(tty, pos, &head, &tail) < 0)
				return -1;
			if (head == src->s_head)
				break;
			// ����
			if (pos >= head)
				dis = pos - head;
			else
				dis = pos + src->s_size - head;
			if ((head = up_line(tty, head, 1)) < 0)
				return -1;
			// 
			while (dis-- > 0) {
				if (src->s_buf[head].ch == KEY_ENTER)
					break;
				if (++head >= src->s_size)
					head = 0;
			}
			pos = head;
			break;
		case SBUF_DOWN:
			//
			if (get_line_ht(tty, pos, &head, &tail) < 0)
				return -1;
			if (tail == src->s_tail)
				break;
			// ����
			if (pos >= head)
				dis = pos - head;
			else
				dis = pos + src->s_size - head;
			if ((head = down_line(tty, head, 1)) < 0)
				return -1;
			// 
			while (dis-- > 0) {
				if ((head == src->s_tail) ||(src->s_buf[head].ch == KEY_ENTER))
					break;
				if (++head >= src->s_size)
					head = 0;
			}
			pos = head;
			break;
		case SBUF_LEFT:
			if (pos == src->s_head)
				break;
			if (--pos < 0)
				pos = src->s_size - 1;
			break;
		case SBUF_RIGHT:
			if (pos == src->s_tail)
				break;
			if (++pos >= src->s_size)
				pos = 0;
			break;
		default:
			return -1;
	}
	return pos;
}



//  
struct cursor_pos
map_to_screen(struct tty_window* tty, int pos) 
{
	struct cursor_pos ret;
	// �ȵõ� pos �����е���ͷ ��β
	int line_tail, line_head, i, j;
	i = get_line_ht(tty, pos, &line_head, &line_tail);
	ret.y = tty->origin_pos.y + i;
	// �õ� pos �� x ������ ����
	for (i = line_head, j = 0; i != line_tail && i != pos; j++)
		if (++i >= tty->buf.s_size)
			i = 0;
	ret.x = tty->origin_pos.x + j;
	return ret;
}