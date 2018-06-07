#include"mix_window.h"
#include"kernel.h"
#include"file_sys.h"
#include"stat.h"
#include"fcntl.h"
// 3 ���ն���Ϊ��������ն�
int MIX_STD_CERR = 0; // �������
int MIX_STD_OUT = 0;  // ��׼���
int MIX_STD_IN = 0;   // ��׼���� 

struct cursor_pos D_CURSOR_POS = { WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1 }; // �������λ��
struct cursor_pos origin_pos_tty[4];     // �ĸ�tty �豸���ĳ�ʼλ��
struct tty_window t_window[4];
struct tty_window* tty_dev[4] =
{
	t_window,                               // Ψһ�������ն�tty                 minor = 0
	t_window + 1,                           // �ն� lp ��ӡ��                
	t_window + 2,                           // �ն� ttyx
	t_window + 3,                           // �ն� unamed ������ʾ�����Ĵ���    minor = 3
};

int sys_read(unsigned int fd, char * buf, int count);
int sys_write(unsigned int fd, char * buf, int count);
int sys_lseek(unsigned int fd, off_t offset, int origin);
int sys_open(const char * filename, int flag, int mode);
int try_to_rw(struct tty_window* tty, unsigned int index, unsigned int rw);

// console init

void console_init(){
        // �߿����� ��ɫ���ַ������
	struct win_fram fm = { '*', FRAM_WIDTH, CFC_White, CBC_Black };
        // ����ʼλ���趨
	origin_pos_tty[0].x = DIS + FRAM_WIDTH;
	origin_pos_tty[0].y = DIS + FRAM_WIDTH;
	origin_pos_tty[1].x = DIS + 3 * FRAM_WIDTH + FRAM_GAP + TTY_WIDTH;
	origin_pos_tty[1].y = DIS + FRAM_WIDTH;
	
        for (i = 0; i < 2; i++) {
		t_window[i].current_pos = t_window[i].origin_pos = origin_pos_tty[i];
		t_window[i].fram = fm;
		t_window[i].win_height = TTY_HEIGHT;
		t_window[i].win_width = TTY_WIDTH;
		// ���������Ĭ����ʾ��ɫ
		t_window[i].b_color = CBC_Black;
		t_window[i].f_color = CFC_White;
		t_window[i].char_b_color = CFC_White;
		t_window[i].button = 0;
		// ��ʾ��������ʼ��
		t_window[i].buf.s_size = MAX_SCREEN_BUF_SIZE  - 10;
		t_window[i].buf.cur_head = t_window[i].buf.cur_tail = 0;
		t_window[i].buf.s_head = t_window[i].buf.s_tail = 0;
		//�����ʼ��
		for (j = 1; j <= t_window[i].buf.s_size; j++) {
			t_window[i].buf.m_buf[j].prev = j - 1;
			t_window[i].buf.m_buf[j].next = j + 1;

		}
		t_window[i].buf.m_buf[j - 1].next = 0;
		t_window[i].buf.free_list = 1;
		t_window[i].buf.s_pos = 0;
		// ��������ģʽΪ ���ģʽ
		t_window[i].button &= ~AM_MODE;
		// 2018.1.14 XXXXXXXXXXXXXXXXX
		t_window[i].fd_set[0] = -1; // ��Ч�ļ�������
		t_window[i].fd_set[1] = -1; // ��Ч�ļ�������
		t_window[i].HT_rw = try_to_rw;
	}
	// �������
	strcpy_s(t_window[0].name, 9, "tty");
	strcpy_s(t_window[1].name, 9, "lp");
        return;
}

void draw_console_window(){
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
			for (k = 0; k < H; k++) {
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

}


// tty �豸��ʼ��
void tty_init()
{
	int i, j, k, W, H;
	int flag;
	struct cursor_pos cur;
	// �߿����� ��ɫ���ַ������
	struct win_fram fm = { '*', FRAM_WIDTH, CFC_White, CBC_Black };
	// ����ʼλ���趨
	origin_pos_tty[0].x = DIS + FRAM_WIDTH;
	origin_pos_tty[0].y = DIS + FRAM_WIDTH;
	origin_pos_tty[1].x = DIS + 3 * FRAM_WIDTH + FRAM_GAP + TTY_WIDTH;
	origin_pos_tty[1].y = DIS + FRAM_WIDTH;
	origin_pos_tty[2].x = DIS + FRAM_WIDTH;
	origin_pos_tty[2].y = DIS + TTY_HEIGHT + FRAM_GAP + 3 * FRAM_WIDTH;
	origin_pos_tty[3].x = DIS + 3 * FRAM_WIDTH + FRAM_GAP + TTY_WIDTH;
	origin_pos_tty[3].y = DIS + TTY_HEIGHT + FRAM_GAP + 3 * FRAM_WIDTH;
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
		// ��ʾ��������ʼ��
		t_window[i].buf.s_size = MAX_SCREEN_BUF_SIZE  - 10;
		t_window[i].buf.cur_head = t_window[i].buf.cur_tail = 0;
		t_window[i].buf.s_head = t_window[i].buf.s_tail = 0;
		//�����ʼ��
		for (j = 1; j <= t_window[i].buf.s_size; j++) {
			t_window[i].buf.m_buf[j].prev = j - 1;
			t_window[i].buf.m_buf[j].next = j + 1;

		}
		t_window[i].buf.m_buf[j - 1].next = 0;
		t_window[i].buf.free_list = 1;
		t_window[i].buf.s_pos = 0;
		// ��������ģʽΪ ���ģʽ
		t_window[i].button &= ~AM_MODE;
		// 2018.1.14 XXXXXXXXXXXXXXXXX
		t_window[i].fd_set[0] = -1; // ��Ч�ļ�������
		t_window[i].fd_set[1] = -1; // ��Ч�ļ�������
		t_window[i].HT_rw = try_to_rw;
	}
	// �������
	strcpy_s(t_window[0].name, 9, "tty");
	strcpy_s(t_window[1].name, 9, "lp");
	strcpy_s(t_window[2].name, 9, "ttyx");
	strcpy_s(t_window[3].name, 9, "unamed");
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
			for (k = 0; k < H; k++) {
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
	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	return;
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
static int check_sbuf_pos(struct tty_window* tty, int pos)
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
// ��ȷ�棬 ���� ��
static check_tty_cursor(struct tty_window* tty)
{
	int x = tty->current_pos.x;
	int y = tty->current_pos.y;
	if (x < tty->origin_pos.x || x >= (tty->origin_pos.x + tty->win_width)
		|| y < tty->origin_pos.y || y >= (tty->origin_pos.y + tty->win_height))
		return 0;
	return 1;
}

// pos ������λ��Ҫλ��һ�е���ͷ
// ��������ָ������  n �У�XXXXXXXX��
int up_line(struct tty_window* tty, int pos, int lines)
{
	// һ�������԰������ַ���
	int per_line = tty->win_width, k;
	struct screen_buf* src_buf = &(tty->buf);
	if (pos == src_buf->s_head)
		return pos;
	while (lines-- > 0) {
		for (k = 0; k < tty->win_width; k++) {
			pos = src_buf->m_buf[pos].prev;
			if (pos == src_buf->s_head)
				return pos;
			if (src_buf->m_buf[pos].m_char.ch == KEY_ENTER && k) {
				pos = src_buf->m_buf[pos].next;
				break;
			}
		}
	}
	return pos;
}

// pos ������λ��Ҫλ��һ�е���ͷ
// ��������ָ������  n �У�XXXXXXXX��
int down_line(struct tty_window* tty, int pos, int lines)
{
	// һ�������԰������ַ���
	int per_line = tty->win_width, k;
	struct screen_buf* src_buf = &(tty->buf);
	if (pos == src_buf->s_tail)
		return pos;
	while (lines-- > 0) {
		for (k = 0; k < tty->win_width; k++) {
			if (pos == src_buf->s_tail)
				return pos;
			if (src_buf->m_buf[pos].m_char.ch == KEY_ENTER) {
				pos = src_buf->m_buf[pos].next;
				break;
			}
			pos = src_buf->m_buf[pos].next;
		}
	}
	return pos;
}


// start ָʾ��ʼλ�ã� ע��
// pos λ�ڻ�����β�һ�����βλ��һ�����У� ����ͷ����β��Ϊ������β
// �ú������ش� startλ�ÿ�ʼ�� pos һ�������У��� 0 ����
// �ҳ���ǰλ�������е���β �� ��ͷ��XXXXXXXX��
static int get_line_ht_aux(struct tty_window* tty, int pos,
	int* head, int* tail, int start)
{
	int k, rows = -1;
	struct screen_buf* src = &tty->buf;
	int line1 = start, line2, i;  
	while (((line2 = down_line(tty, line1, 1)) != src->s_tail)) {
		// ��λ pos
		++rows;
		for (i = line1; i != line2; i = src->m_buf[i].next) {
			if (pos == i) {
				*head = line1;
				*tail = src->m_buf[line2].prev;
				return rows;
			}
		}
		line1 = line2;
	}
	++rows;
	for (i = line1, k = 1; i != line2; k++,i = src->m_buf[i].next) {
		if (k >= tty->win_width || src->m_buf[i].m_char.ch == KEY_ENTER)
			break;
	}
	// line1 �� line2 λ��ͬһ��
	if (i == line2) {
		*head = line1;
		*tail = line2;
		return rows;
	}
	// line1 �� line2 λ�ڲ�ͬ��
	if (pos == line2) {
		*head = *tail = line2;
		return ++rows;
	}
	*head = line1;
	*tail = src->m_buf[line2].prev;
	return rows;
}

// �� cur_tail ��ʼ
int get_line_ht(struct tty_window* tty, int pos,int* head, int* tail)
{
	return get_line_ht_aux(tty, pos, head, tail, tty->buf.cur_head);
}
// �� s_tail ��ʼ
int get_line_ht_s(struct tty_window* tty, int pos, int* head, int* tail)
{
	return get_line_ht_aux(tty, pos, head, tail, tty->buf.s_head);
}

void print_char(struct mix_char* m_char, struct tty_window* tty);
void cursor_flash(struct tty_window* tty);

// ����ƶ�����
// ���󷵻� -1��
int move_sbuf_cursor(int direc, struct tty_window* tty) {
	struct screen_buf* src = &tty->buf;
	int rows;
	int head, tail, pos, dis, t_head, t_tail;
	if (src->s_head == src->s_tail)
		return 0;
	switch (direc) {
	case SBUF_UP:
		// �������
		// �������
		// �޸� 18.1.15
		get_line_ht_s(tty, src->s_pos, &head, &tail);
		// ���ļ��ж�
		if (head == src->s_head)
			tty->HT_rw(tty, MT_HEAD, MT_READ);
		get_line_ht_s(tty, src->s_pos, &head, &tail);
		if (head == src->s_head)
			break;
		/*  18.1.15 δ�޸�ǰ
		get_line_ht_s(tty, src->s_pos, &head, &tail);
		if (head == src->s_head)
		break;
		*/
		// end 
		get_line_ht_s(tty, src->m_buf[head].prev, &t_head, &t_tail);
		for (dis = t_head, pos = head; pos != src->s_pos; pos = src->m_buf[pos].next){
			if (src->m_buf[dis].m_char.ch == KEY_ENTER)
				break;
			dis = src->m_buf[dis].next;
		}
		if (head == src->cur_head){
			src->cur_head = t_head;
			s_flush(tty);
		}
		src->s_pos = dis;
		tty->current_pos = map_to_screen(tty, src->s_pos);
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		break;
	case SBUF_DOWN:
		// �������
		// �޸� 18.1.15
		rows = get_line_ht(tty, src->s_pos, &head, &tail);
		if (down_line(tty, tty->buf.cur_tail, 1) == tty->buf.s_tail)
			tty->HT_rw(tty, MT_TAIL, MT_READ);
		rows = get_line_ht(tty, src->s_pos, &head, &tail);
		if (tail == src->s_tail)
			break;
		/*  18.1.15 δ�޸�ǰ
		rows = get_line_ht(tty, src->s_pos, &head, &tail);
		if (tail == src->s_tail)
		break;
		*/
		// end
		get_line_ht_s(tty, src->m_buf[tail].next, &t_head, &t_tail);
		for (dis = t_head, pos = head; pos != src->s_pos; pos = src->m_buf[pos].next){
			if (dis == src->s_tail || src->m_buf[dis].m_char.ch == KEY_ENTER)
				break;
			dis = src->m_buf[dis].next;
		}
		if (tail == src->m_buf[src->cur_tail].prev){
			if (++rows >= tty->win_height){
				src->cur_tail = src->s_tail;
				src->cur_head = down_line(tty, src->cur_head, 1);
				s_flush(tty);
			}
		}
		src->s_pos = dis;
		tty->current_pos = map_to_screen(tty, src->s_pos);
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		break;
	case SBUF_LEFT:
		// �޸� 18.1.15
		if (src->s_pos == src->s_head)
			tty->HT_rw(tty, MT_HEAD, MT_READ);
		if (src->s_pos == src->s_head)
			break;
		/*  18.1.15 δ�޸�ǰ
		if (src->s_pos == src->s_head)
		break;
		*/
		// END
		pos = src->s_pos;
		pos = src->m_buf[pos].prev;
		if (src->s_pos == src->cur_head){
			get_line_ht_s(tty, src->m_buf[src->s_pos].prev, &head, &tail);
			src->cur_head = head;
			s_flush(tty);
		}
		src->s_pos = pos;
		tty->current_pos = map_to_screen(tty, src->s_pos);
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		break;
	case SBUF_RIGHT:
		// �޸� 18.1.15
		if (down_line(tty, tty->buf.cur_tail, 1) == tty->buf.s_tail)
			tty->HT_rw(tty, MT_TAIL, MT_READ);
		if (src->s_pos == src->s_tail)
			break;
		/*  18.1.15 δ�޸�ǰ
		if (src->s_pos == src->s_tail)
		break;
		*/
		// END
		pos = src->s_pos;
		pos = src->m_buf[pos].next;
		if (src->s_pos == src->m_buf[src->cur_tail].prev
			&& src->cur_tail != src->s_tail){
			get_line_ht_s(tty, src->m_buf[src->s_pos].next, &head, &tail);
			src->cur_tail = src->s_tail;
			src->cur_head = down_line(tty, src->cur_head, 1);
			s_flush(tty);
		}
		src->s_pos = pos;
		tty->current_pos = map_to_screen(tty, src->s_pos);
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		break;
	default:
		return -1;
	}
	return 0;
}


// ����������ָ�� s_pos λ��ӳ�䵽��Ļ�еĹ��λ��
struct cursor_pos
	map_to_screen(struct tty_window* tty, int pos)
{
	struct cursor_pos ret;
	// �ȵõ� pos �����е���ͷ ��β
	int line_tail, line_head, i, j;
	i = get_line_ht(tty, pos, &line_head, &line_tail);
	ret.y = tty->origin_pos.y + i;
	// �õ� pos �� x ������ ����
	for (i = line_head, j = 0; i != pos && i != line_tail; j++)
		i = tty->buf.m_buf[i].next;
	ret.x = tty->origin_pos.x + j;
	return ret;
}


//  ����Ļ�����һ���ַ�
void print_char(struct mix_char* m_char, struct tty_window* tty) {
	struct cursor_pos pos = tty->current_pos;
	set_cursor_position(pos.x, pos.y);
	if (m_char->b_set)
		set_console_color(m_char->f_color, m_char->b_color);
	else
		set_console_color(m_char->f_color, tty->b_color);
	printf("%c", m_char->ch);
	set_cursor_position(pos.x, pos.y);
	return;
}

// ����ʾ�������д�λ�� beg -- end ���ַ��������Ļ
// ע�� �� �ú�������� cur_tail
static
int tty_print(struct tty_window* tty, int beg, int end)
{
	int ret = 0, c;
	struct cursor_pos cursor_pos;
	struct mix_char m_char = { ' ',CFC_White, 0, 0 };
	struct screen_buf* buf = &tty->buf;
	while (beg != end) {
		c = buf->m_buf[beg].m_char.ch;
		// ���ַ����� enter �� ���������Ļ
		if (c != KEY_ENTER)
			print_char(&(buf->m_buf[beg].m_char), tty);
		else{
			cursor_pos = tty->current_pos;
			while (tty->current_pos.x < (tty->origin_pos.x + tty->win_width)){
				print_char(&m_char, tty);
				++tty->current_pos.x;
			}
			tty->current_pos = cursor_pos;
		}
		++ret;
		beg = buf->m_buf[beg].next;
		// ���µ�ǰ��ʾ����Ļ���ַ���β��
		buf->cur_tail = beg;
		// ������һ��
		if (c == KEY_ENTER || ++tty->current_pos.x >= (tty->origin_pos.x + tty->win_width)) {
			tty->current_pos.x = tty->origin_pos.x;
			if (++tty->current_pos.y >= (tty->win_height + tty->origin_pos.y)) {
				--tty->current_pos.y;
				break;
			}
			// ���µ�һ�����
			cursor_pos = tty->current_pos;
			for (c = 0; c < tty->win_width; c++){
				print_char(&m_char, tty);
				++tty->current_pos.x;
			}
			tty->current_pos = cursor_pos;
		}
		// ���ù��λ��
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	}
	return ret;
}

// ����
static void clear_window(struct tty_window* tty)
{
	int i, j;
	set_console_color(CFC_White, tty->b_color);
	for (i = 0; i < tty->win_height; i++) {
		set_cursor_position(tty->origin_pos.x, tty->origin_pos.y + i);
		for (j = 0; j < tty->win_width; j++)
			printf(" ");
	}
	return;
}

// ˢ����ʾ������, ����ʾ�������д� 
// cur_head - cur_tail ���ַ�ȫ���������Ļ
// ���� s_pos ��������β
int s_flush(struct tty_window* tty) {
	int count = 0;
	struct screen_buf* sc_buf = &(tty->buf);

	// ������Ļ���
	clear_window(tty);
	if (sc_buf->cur_head == sc_buf->cur_tail)
		return 0;
	// �ָ�����ó�ʼλ��
	reset_tty_cursor(tty);
	// ���ַ��������Ļ
	count = tty_print(tty, tty->buf.cur_head, tty->buf.cur_tail);
	// ���õ�ǰ����λ�ö�Ӧ�������е�λ��
	if (tty->buf.s_tail == tty->buf.cur_tail)
		tty->buf.s_pos = tty->buf.cur_tail;
	else
		tty->buf.s_pos = tty->buf.m_buf[tty->buf.cur_tail].prev;
	return count;
}



static void s_link(struct screen_buf* buf, int prev, int next)
{
	buf->m_buf[prev].next = next;
	buf->m_buf[next].prev = prev;
}

// ����ʾ��������д����, ������ݣ������Ǵ�
// ������β����ӣ� ���Ǵ���������λ�ò����ַ�
// ע�� �� ���ݲ����λ���� buf.s_pos ����
int write_screen_buf(struct mix_char* buf, int count, struct tty_window* tty)
{
	int i, k, line_head, line_tail;
	struct screen_buf* sc_buf = &(tty->buf);
	if (count <= 0)
		return 0;
	i = 0;
	// �����������ʱ�ǿյ�
	if(sc_buf->s_head == sc_buf->s_tail) {
		if (!sc_buf->free_list)
			return -1;
		// �ȴ�����������ȡ�����������
		sc_buf->s_head = sc_buf->free_list;
		sc_buf->s_tail = sc_buf->m_buf[sc_buf->free_list].next;
		// ��������ָ����һ��
		sc_buf->free_list = sc_buf->m_buf[sc_buf->s_tail].next;
		// ��ʼ��
		sc_buf->cur_head = sc_buf->s_head;
		sc_buf->cur_tail = sc_buf->s_tail;
		sc_buf->s_pos = sc_buf->cur_head;
		// д�벢������ַ�
		sc_buf->m_buf[sc_buf->s_pos].m_char = buf[i++];
		count--;
		tty_print(tty, sc_buf->s_pos, sc_buf->cur_tail);
		sc_buf->s_pos = sc_buf->cur_tail;
	}
	// д��ʣ�µ��ַ�
	// �ж��Ǵӻ�����β����ӣ����Ǵ��м�����ַ�
	while (count-- > 0) {
		// �ӻ�����β�����
		if (sc_buf->s_pos == sc_buf->s_tail) {
			sc_buf->m_buf[sc_buf->s_pos].m_char = buf[i++];
			// ����һ�е���������

			// 18.1.15 ���
			if (!sc_buf->free_list)
				tty->HT_rw(tty, MT_HEAD, MT_WRITE);
			// end
			if (!sc_buf->free_list) {
				get_line_ht_s(tty, sc_buf->s_head, &line_head, &line_tail);
				sc_buf->free_list = line_head;
				sc_buf->s_head = sc_buf->m_buf[line_tail].next;
				sc_buf->m_buf[line_tail].next = 0;
			}
			s_link(sc_buf, sc_buf->s_pos, sc_buf->free_list);
			sc_buf->cur_tail = sc_buf->s_tail = sc_buf->free_list;
			sc_buf->free_list = sc_buf->m_buf[sc_buf->free_list].next;
			// ��Ļ����
			if (get_line_ht(tty, sc_buf->cur_tail, &line_head, &line_tail)
				>= tty->win_height) {
				sc_buf->cur_head = down_line(tty, sc_buf->cur_head, 1);
				s_flush(tty);
			}
			else {
				tty_print(tty, sc_buf->s_pos, sc_buf->cur_tail);
				sc_buf->s_pos = sc_buf->cur_tail;
			}
			// 18.1.15���
			tty->current_pos = map_to_screen(tty, sc_buf->s_pos);
			set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		}
		else {
			//  ���м�����ַ�
			// �����ǰ�����λ��Ϊ���з�--ENTER,���ڵ�ǰλ��֮ǰ�����ַ�
			// �����ڵ�ǰλ��֮������ַ�
			if (buf[i].ch != KEY_ENTER && sc_buf->m_buf[sc_buf->s_pos].m_char.ch == KEY_ENTER)
				move_sbuf_cursor(SBUF_LEFT, tty);
			// 18.1.15 ���
			if (!sc_buf->free_list) {
				if (sc_buf->cur_head == sc_buf->s_head)
					tty->HT_rw(tty, MT_TAIL, MT_WRITE);
				else
					tty->HT_rw(tty, MT_HEAD, MT_WRITE);
			}
			// end
			if (!sc_buf->free_list) {
				if (sc_buf->cur_head == sc_buf->s_head) {
					get_line_ht_s(tty, sc_buf->s_tail, &line_head, &line_tail);
					sc_buf->free_list = line_head;
					sc_buf->s_tail = sc_buf->m_buf[line_tail].prev;
					sc_buf->m_buf[line_tail].next = 0;
				}
				else {
					get_line_ht_s(tty, sc_buf->s_head, &line_head, &line_tail);
					sc_buf->free_list = line_head;
					sc_buf->s_head = sc_buf->m_buf[line_tail].next;
					sc_buf->m_buf[line_tail].next = 0;
				}
			}
			k = sc_buf->m_buf[sc_buf->free_list].next;
			s_link(sc_buf, sc_buf->free_list, sc_buf->m_buf[sc_buf->s_pos].next);
			s_link(sc_buf, sc_buf->s_pos, sc_buf->free_list);
			sc_buf->s_pos = sc_buf->free_list;
			sc_buf->free_list = k;
			//
			sc_buf->m_buf[sc_buf->s_pos].m_char = buf[i++];
			if (get_line_ht(tty, sc_buf->s_pos, &line_head, &line_tail)
				>= tty->win_height) {
				k = sc_buf->s_pos;// 18.1.22 ���
				sc_buf->cur_head = down_line(tty, sc_buf->cur_head, 1);
				s_flush(tty);
				sc_buf->s_pos = k;// 18.1.22 ���
			}
			else
				tty_print(tty, sc_buf->m_buf[sc_buf->s_pos].prev, sc_buf->cur_tail);
			tty->current_pos = map_to_screen(tty, sc_buf->s_pos);
			set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		}
	}
	return 0;
}


// ˢ�µ�ǰ���λ�õ��ַ�
void cursor_flash(struct tty_window* tty)
{
	struct screen_buf* src = &tty->buf;
	struct mix_char m_char = src->m_buf[src->s_pos].m_char;
	if (src->s_head == src->s_tail)
		return;
	tty->current_pos = map_to_screen(tty, tty->buf.s_pos); // 18.1.23 ���
	// ����ǻ��з����򷵻�
	if (src->m_buf[src->s_pos].m_char.ch == KEY_ENTER)
		return;
	if (src->s_pos == src->s_tail)
		m_char.ch = '\0';
	if (IS_APPEND(tty->button)) {
		print_char(&m_char, tty);
		return ;
	}
	// ��鵱ǰ�Ƿ����޸�ģʽ, ���õ�ǰ��걳��Ϊ ��ɫ
	if (IS_MODIFY(tty->button)) {
		m_char.b_set = 1;
		m_char.b_color = CBC_Blue;
		print_char(&m_char, tty);
		return;
	}
	return;
}

// �޸��ɹ������λ���ַ�
// ������λ�ڻ��з�λ�ã� ���޸�ģʽΪ���ģʽ
int modify_buf(struct mix_char* m_char, struct tty_window* tty){
	int mode = tty->button;
	if (!IS_MODIFY(mode))
		return 0;
	//if (m_char->ch == KEY_ENTER)         // 2018.1.14
	//     return 0;
	if (tty->buf.s_head == tty->buf.s_tail) {
		tty->button &= ~AM_MODE;
		write_screen_buf(m_char, 1, tty);
		return 0;
	}
	if (tty->buf.s_pos == tty->buf.cur_tail) {
		tty->button &= ~AM_MODE;
		write_screen_buf(m_char, 1, tty);
		return 0;
	}
	if (m_char->ch == KEY_ENTER)         // 2018.1.14
		return 0;
	// �����޸Ļ��з�
	if (tty->buf.m_buf[tty->buf.s_pos].m_char.ch == KEY_ENTER){
		tty->button &= ~AM_MODE;
		write_screen_buf(m_char, 1, tty);
		return 0;
	}
	tty->buf.m_buf[tty->buf.s_pos].m_char = *m_char;
	cursor_flash(tty);
	//print_char(m_char, tty);
	return 0;
}


// ����Ļ�е�ǰ�������λ�ü�֮��λ��ȫ�����
// ���ָ���굽ԭλ��
void clear_pos_tty(struct tty_window* tty)
{
	struct mix_char m_char = { ' ', CFC_White, 0, 0 };
	struct cursor_pos cursor_pos, cursor_end;
	int X = (tty->win_width + tty->origin_pos.x), Y;
	tty->current_pos = map_to_screen(tty, tty->buf.s_pos);
	cursor_end = map_to_screen(tty, tty->buf.cur_tail);
	Y = cursor_end.y;
	cursor_pos = tty->current_pos;
	// 
	for (; tty->current_pos.y < Y; ++tty->current_pos.y) {
		for (; tty->current_pos.x < X; ++tty->current_pos.x)
			print_char(&m_char, tty);
		tty->current_pos.x = tty->origin_pos.x;
	}
	// ������һ��
	if (tty->current_pos.y < (tty->win_height + tty->origin_pos.y))
		for (; tty->current_pos.x < cursor_end.x; ++tty->current_pos.x)
			print_char(&m_char, tty);
	tty->current_pos = cursor_pos;
	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
}

// ɾ���������λ���ַ�
// �����ǰ���޸�ģʽ�� ���Ϊ���ģʽ
int delete_buf(struct tty_window* tty) {
	int pos, next, prev;	
	struct cursor_pos cursor_pos;
	if (tty->buf.s_head == tty->buf.s_tail)
		return 0;
	// ������޸�ģʽ�� ��Ϊ���ģʽ
	if (IS_MODIFY(tty->button)){
		tty->button &= ~AM_MODE;
		cursor_flash(tty);
	}
	// 18.1.15 ���
	if (down_line(tty, tty->buf.cur_tail, 1) == tty->buf.s_tail)
		tty->HT_rw(tty, MT_TAIL, MT_READ);
	if (tty->buf.s_pos == tty->buf.s_head)
		tty->HT_rw(tty, MT_HEAD, MT_READ);
	// end
	if (tty->buf.s_pos == tty->buf.s_tail) {
		pos = tty->buf.s_pos;
		move_sbuf_cursor(SBUF_LEFT, tty);
		// ������������
		tty->buf.m_buf[pos].next = tty->buf.free_list;
		if (tty->buf.free_list)
			tty->buf.m_buf[tty->buf.free_list].prev = pos;
		tty->buf.free_list = pos;

		clear_pos_tty(tty);
		// ����β��ָ��
		tty->buf.cur_tail = tty->buf.s_tail
			= tty->buf.s_pos;
		// 18.1.15
		if (tty->buf.cur_head == tty->buf.cur_tail)
			move_sbuf_cursor(SBUF_LEFT, tty);
		//print_char(&m_char, tty);
		return 0;
	}
	if (tty->buf.s_pos == tty->buf.cur_tail)
		return 0;
	if (tty->buf.s_pos == tty->buf.s_head) {
		next = tty->buf.m_buf[tty->buf.s_pos].next;
		pos = tty->buf.s_pos;
		// �ƶ����λ��
		move_sbuf_cursor(SBUF_RIGHT, tty);
		// ������������
		tty->buf.m_buf[pos].next = tty->buf.free_list;
		if (tty->buf.free_list)
			tty->buf.m_buf[tty->buf.free_list].prev = pos;
		tty->buf.free_list = pos;
		// 
		pos = tty->buf.s_pos;
		tty->buf.s_head = tty->buf.cur_head = next;
		tty->buf.cur_tail = tty->buf.s_tail; //bug--2018.1.22, ���
		s_flush(tty);
		tty->buf.s_pos = pos;
		tty->current_pos = map_to_screen(tty, tty->buf.s_pos);
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		return 0;
	}
	// ɾ�����λ�ú�������ַ�
	clear_pos_tty(tty);
	// ������������
	prev = tty->buf.m_buf[tty->buf.s_pos].prev;
	next = tty->buf.m_buf[tty->buf.s_pos].next;
	pos = tty->buf.s_pos;
	// �ƶ����λ��
	move_sbuf_cursor(SBUF_LEFT, tty);
	// ������������
	tty->buf.m_buf[pos].next = tty->buf.free_list;
	if (tty->buf.free_list)
		tty->buf.m_buf[tty->buf.free_list].prev = pos;
	tty->buf.free_list = pos;
	// ��������
	tty->buf.m_buf[prev].next = next;
	tty->buf.m_buf[next].prev = prev;
	// ���ɾ�����ַ��ǵ�ǰ��Ļ��ʾ�����ַ�
	if (tty->buf.cur_head == pos)
		tty->buf.cur_head = tty->buf.s_pos;
	//clear_pos_tty(tty);
	// �������ɾ��λ�ú�������ַ�
	//pos = tty->buf.s_pos;
	cursor_pos = tty->current_pos;
	tty_print(tty, tty->buf.s_pos, tty->buf.s_tail);
	//tty->buf.s_pos = pos;
	tty->current_pos = cursor_pos;
	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	return 0;
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
	count = 10;
	for (i = 0; i < count; i++) {
		buf[i].b_set = 0;
		buf[i].f_color = CFC_Purple;
	}
	tty_write(minor, buf, count);
	return count;
}


// �������
int mix_print(struct mix_char* buf, int count, unsigned minor) {
	struct tty_window* tty = tty_dev[minor];
	write_screen_buf(buf, count, tty);
	return 0;
}

// ����
int new_line(unsigned minor)
{
	struct tty_window* tty = tty_dev[minor];
	struct mix_char c = { KEY_ENTER, tty->f_color, tty->b_color, 0 };
	tty_write(minor, &c, 1);
	return 0;
}

// �����������ʾ������
int clc_win(unsigned minor) {
	struct tty_window* tty = tty_dev[minor];
	struct screen_buf* src_buf = &(tty->buf);
	int i, j;
	// ����
	clear_window(tty);
	reset_tty_cursor(tty);
	// �����ʾ������
	for (j = 1; j <= tty->buf.s_size; j++) {
		tty->buf.m_buf[j].prev = j - 1;
		tty->buf.m_buf[j].next = j + 1;

	}
	tty->buf.m_buf[j - 1].next = 0;
	tty->buf.free_list = 1;
	// ָ������
	tty->buf.s_pos = 0;
	src_buf->cur_head = src_buf->cur_tail = 0;
	src_buf->s_head = src_buf->s_tail = 0;
	return 0;
}

// �������
int mix_cerr(const char* const s) {
	enum { SZ = 50 };
	int minor = MIX_STD_CERR;
	int left = strlen(s), count, i = 0, j, k;
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

// 
static int find_back_pos(struct tty_window* tty,
	int start, int pos) {
	int index;
	struct screen_buf* src_buf = &tty->buf;
	if (src_buf->s_head == src_buf->s_tail)
		return 0;
	index = start;
	while (index != src_buf->s_tail) {
		if (index == pos)
			return 1;
		index = src_buf->m_buf[index].next;
	}
	if (index == pos)
		return 1;
	return 0;
}

// �ն˶�
int get_key();
int tty_read(unsigned minor, char * buf, int count)
{
	// ��ȡ�ն�
	int i, j, s_pos;
	struct mix_char m_char;
	struct tty_window* tty = tty_dev[minor];
	if (!check_tty_cursor(tty)) {
		clc_win(minor);
		mix_cerr("tty_read : tty-%d cursor out of boundary!");
		return -1;
	}
	// 0 ���ն���ΪΨһ���������ն�
	if (minor == 0) {
		// ��ӡ��ǰ����Ŀ¼
		print_cur_dir();
		// ��¼��ǰ���λ��
		s_pos = tty->buf.m_buf[tty->buf.s_pos].prev;
		// 
		do{
			m_char.ch = keybord(tty);
			// ������λ��λ�� [@noodle] ǰ�棬��ʲôҲ����
			if (!find_back_pos(tty, tty->buf.m_buf[s_pos].next, tty->buf.s_pos))
				continue;
			// ���Ҫɾ��
			if (m_char.ch == KEY_BACKSPACE) {
				if (tty->buf.s_tail != tty->buf.m_buf[s_pos].next)
					delete_buf(tty);
				continue;
			}
			// û�������ı�������ʾ
			if (!IS_THLIGHT(tty->button))
				m_char.b_set = 0;
			else {
				m_char.b_set = 1;
				m_char.b_color = tty->char_b_color;
			}
			m_char.f_color = tty->f_color;
			// д��
			tty_write(minor, &m_char, 1);
		} while (m_char.ch != KEY_ENTER || tty->buf.s_pos != tty->buf.s_tail);
		// ȡ������, �� s_pos �� s_tail ֮��������ַ�
		// ����������з�������
		s_pos = tty->buf.m_buf[s_pos].next;
		for (i = 0; s_pos != tty->buf.s_tail; s_pos = tty->buf.m_buf[s_pos].next) {
			if (tty->buf.m_buf[s_pos].m_char.ch != KEY_ENTER)
				buf[i++] = tty->buf.m_buf[s_pos].m_char.ch;
		}
		return ++i;
	}
	// ֱ�ӻ�ȡ����������ַ�
	i = 0;
	while (count-- > 0) 
		buf[i++] = get_key();
	return i;
}

// �ն�д, ע��ֻ��д�����ʾ�ַ�
// ����д������ַ�
int tty_write(unsigned minor, struct mix_char * buf, int count)
{
	// ��ȡ�ն�
	int i = 0;
	struct tty_window* tty = tty_dev[minor];
	if (!check_tty_cursor(tty)) {
		clc_win(minor);
		mix_cerr("tty_write : out of boundary!");
		return -1;
	}
	if (IS_APPEND(tty->button))
		write_screen_buf(buf, count, tty);
	else {
		if (IS_APPEND(tty->button))
			write_screen_buf(buf + i++, 1, tty);
		else
			modify_buf(buf + i++, tty);
	}
	return 0;
}


// for test
void vim_test();
void test_tty() {
	int minor = 0, count = 0;
	struct mix_char m_char;
	struct tty_window* tty = tty_dev[minor];
	if (!check_tty_cursor(tty)) {
		clc_win(minor);
		return -1;
	}
	while (1) {
		m_char.ch = keybord(tty);

		// ���Ҫɾ��
		if (m_char.ch == KEY_BACKSPACE) {
			delete_buf(tty);
			continue;
		}
		// û�������ı�������ʾ
		if (!IS_THLIGHT(tty->button))
			m_char.b_set = 0;
		else {
			m_char.b_set = 1;
			m_char.b_color = tty->char_b_color;
		}
		m_char.f_color = tty->f_color;
		// д��
		tty_write(minor, &m_char, 1);
		if (count++ > 120)
			vim_test();
	}
}


//char FD0[1000 * 16];
//int f_pos0 = 0;
//char FD1[1000 * 16];
//int f_pos1 = 0;
//int M_printf(const char *fmt, ...);
//
//
//int rw_to_fd(struct tty_window* tty, unsigned int fd, int rw,
//	int start, int end) {
//	struct screen_buf* sc_buf = &tty->buf;
//	int count, f_pos, k, beg;
//	struct mix_char m_char[100];
//	struct mix_char line = { 0,0,0,0 };
//	char* buf;
//	if (fd == 0) {
//		buf = FD0;
//		f_pos = f_pos0;
//	}
//	else {
//		buf = FD1;
//		f_pos = f_pos1;
//	}
//	if (rw == MT_WRITE && fd == 0) {
//		for (k = 0, beg = 0, count = 0; start != end; count++) {
//			m_char[beg++] = sc_buf->m_buf[start].m_char;
//			if ((++k == tty->win_width) ||
//				(sc_buf->m_buf[start].m_char.ch == KEY_ENTER)) {
//				m_char[beg++] = line;
//				k = beg;
//				beg = 0;
//				// д��
//				while (k-- > 0) {
//					*((struct mix_char*)(buf + f_pos)) = m_char[beg++];
//					f_pos += sizeof(struct mix_char);
//				}
//				k = 0;
//				beg = 0;
//			}
//			start = sc_buf->m_buf[start].next;
//		}
//	}
//	else if (rw == MT_WRITE && fd == 1) {
//		for (count = 0; end != start; ) {
//			end = sc_buf->m_buf[end].prev;
//			*((struct mix_char*)(buf + f_pos)) = sc_buf->m_buf[end].m_char;
//			f_pos += sizeof(struct mix_char);
//			++count;
//		}
//	}
//	else if (rw == MT_READ && fd == 1) {
//		count = tty->win_width * 2;
//		if ((f_pos / sizeof(struct mix_char)) < count)
//			count = (f_pos / sizeof(struct mix_char));
//		// ���뻺����
//		for (beg = 0; beg < count; beg++) {
//			f_pos -= sizeof(struct mix_char);
//			m_char[beg] = *((struct mix_char*)(buf + f_pos));
//		}
//		// д��
//		for (beg = 0; beg < count; beg++) {
//			sc_buf->m_buf[start].m_char = m_char[beg];
//			start = sc_buf->m_buf[start].next;
//		}
//	}
//	else if (rw == MT_READ && fd == 0) {
//		count = (tty->win_width + 1) * 2; // XXXXXXXXXX
//		if ((f_pos / sizeof(struct mix_char)) < count)
//			count = (f_pos / sizeof(struct mix_char));
//		// ���뻺����
//		for (beg = 0; beg < count; beg++) {
//			f_pos -= sizeof(struct mix_char);
//			m_char[beg] = *((struct mix_char*)(buf + f_pos));
//		}
//		// ����
//		if (count == (tty->win_width + 1) * 2) {
//			for (beg = 0, k = 0; beg != count; beg++) {
//				if (m_char[beg].ch == line.ch)
//					k = beg;
//			}
//			f_pos = f_pos + (beg - k) * sizeof(struct mix_char);
//		}
//		else
//			k = count;
//		// д��
//		count = 0;
//		while (k-- > 0) {
//			if (m_char[k].ch != line.ch) {
//				sc_buf->m_buf[start].m_char = m_char[k];
//				start = sc_buf->m_buf[start].next;
//				++count;
//			}
//		}
//	}
//	if (fd == 0)
//		f_pos0 = f_pos;
//	else
//		f_pos1 = f_pos;
//	return count;
//}


#define EVLDFD 101
int write_to_fd(struct mix_char* m_buf, int count, unsigned int fd) {
	struct file* m_file = current->filp[fd];
	int error_code = 1;
	char* buf = (char*)m_buf;
	count = count * sizeof(struct mix_char);
	if (!m_file)
		return EVLDFD;
	while (count > 0) {
		if ((error_code = sys_write(fd, buf, count)) < 0)
			return error_code;
		count -= error_code;
		buf += error_code;
	}
	return error_code;
}

int read_from_fd(struct mix_char* m_buf, int count, unsigned int fd) {
	struct file* m_file = current->filp[fd];
	int error_code = 1, tmp, index = 0;
	char* buf = (char*)m_buf;
	struct mix_char tmp_buf[WINDOW_WIDTH * 5];
	if ((m_file->f_pos / sizeof(struct mix_char)) < count)
		count = m_file->f_pos / sizeof(struct mix_char);
	tmp = count * sizeof(struct mix_char);
	sys_lseek(fd, -count * sizeof(struct mix_char), SEEK_CUR);
	if (!m_file)
		return EVLDFD;
	while (tmp > 0) {
		if ((error_code = sys_read(fd, ((char*)tmp_buf) + index, tmp)) < 0)
			return error_code;
		tmp -= error_code;
		index += error_code;
	}
	sys_lseek(fd, -count * sizeof(struct mix_char), SEEK_CUR);
	// ��˳�����
	tmp = 0, index = count;
	while (index-- > 0)
		m_buf[tmp++] = tmp_buf[index];
	return count ;
}
// ���ļ�������fd��Ӧ���ļ����Ķ�д
int rw_to_fd(struct tty_window* tty, unsigned int fd, int rw,
	int start, int end) {
	struct screen_buf* sc_buf = &tty->buf;
	int count, k, beg, error_code;
	struct mix_char m_char[WINDOW_WIDTH * 5];
	struct mix_char L_char = { 0,0,0,0 }; // ռλ����ÿ�еĽ�β���һ��XXXXXX
	
	// ͷ��д
	if (rw == MT_WRITE && fd == tty->fd_set[MT_HEAD]) {
		for (beg = 0, count = 0; start != end; count++) {
			m_char[beg++] = sc_buf->m_buf[start].m_char;
			if ((beg == tty->win_width) ||
				(sc_buf->m_buf[start].m_char.ch == KEY_ENTER)) {
				m_char[beg++] = L_char;
				// ��һ���ַ�д�� fd 
				if ((error_code = write_to_fd(m_char, beg, fd)) < 0)
					return error_code;
				beg = 0;
			}
			start = sc_buf->m_buf[start].next;
		}
	}
	// β��д
	else if (rw == MT_WRITE && fd == tty->fd_set[MT_TAIL]) {
		for (count = 0, beg = 0; end != start; ++count) {
			end = sc_buf->m_buf[end].prev;
			m_char[beg++] = sc_buf->m_buf[end].m_char;
			if (beg == tty->win_width) {
				if ((error_code = write_to_fd(m_char, beg, fd)) < 0)
					return error_code;
				beg = 0;
			}
		}
	}
	// β����
	else if (rw == MT_READ && fd == tty->fd_set[MT_TAIL]) {
		count = tty->win_width * 2;
		// ����
		if ((count = read_from_fd(m_char, count, fd)) < 0)
			return count;
		// д��
		for (beg = 0; beg < count; beg++) {
			sc_buf->m_buf[start].m_char = m_char[beg];
			start = sc_buf->m_buf[start].next;
		}
	}
	// ͷ����
	else if (rw == MT_READ && fd == tty->fd_set[MT_HEAD]) {
		count = (tty->win_width + 1) * 2; // ��������  XXXXXXXXXX
	    // ����
		if ((count = read_from_fd(m_char, count, fd)) < 0)
			return count;
		// ����
		if (count == (tty->win_width + 1) * 2) {
			for (beg = 0, k = 0; beg != count; beg++) {
				if (m_char[beg].ch == L_char.ch)
					k = beg;
			}
			if((error_code = sys_lseek(fd, (count - k) 
				             * sizeof(struct mix_char), SEEK_CUR)) < 0)
				return error_code;
		}
		else
			k = count;
		// д��
		count = 0;
		while (k-- > 0) {
			if (m_char[k].ch != L_char.ch) {
				sc_buf->m_buf[start].m_char = m_char[k];
				start = sc_buf->m_buf[start].next;
				++count;
			}
		}
	}
	return count;
}


// ���ļ���д�� ʧ�ܷ��ش�����
int try_to_write_aux(struct tty_window* tty, unsigned int hd)
{
	int error_code = 1, tmp, start, end;
	struct screen_buf *sc_buf = &tty->buf;
	unsigned int fd = tty->fd_set[hd];
	int reserve = 3, s_ht;
	int head, tail, lines;
	// ��� fd �Ƿ��Ǵ򿪵��ļ�������
	if (fd < 0 || fd >= NR_OPEN)
	return (error_code = -EVLDFD);
	if (!current->filp[fd]->f_inode)
	return (error_code = -EVLDFD);
	// ͷ����β������������ reserve ��
	s_ht = sc_buf->cur_head;
	lines = get_line_ht_s(tty, tty->buf.cur_head, &head, &tail);
	if ((lines -= reserve) > 0)
		s_ht = down_line(tty, sc_buf->s_head, lines);
	if (s_ht != sc_buf->cur_head) {
		fd = tty->fd_set[MT_HEAD];
		start = sc_buf->s_head;
		end = s_ht;
		if ((tmp = rw_to_fd(tty, fd, MT_WRITE, start, end)) < 0)
			return (error_code = tmp);
		sc_buf->s_head = s_ht;
		end = sc_buf->m_buf[end].prev;
		sc_buf->m_buf[end].next = sc_buf->free_list;
		if (sc_buf->free_list)
			sc_buf->m_buf[sc_buf->free_list].prev = end;
		sc_buf->free_list = start;
	}
	s_ht = down_line(tty, sc_buf->cur_tail, reserve);
	if (s_ht != sc_buf->s_tail) {
		fd = tty->fd_set[MT_TAIL]; 
		start = s_ht;
		end = sc_buf->s_tail;
		if ((tmp = rw_to_fd(tty, fd, MT_WRITE, start, end)) < 0)
			return (error_code = tmp);
		sc_buf->s_tail = s_ht;
		start = sc_buf->m_buf[start].next;
		sc_buf->m_buf[end].next = sc_buf->free_list;
		if (sc_buf->free_list)
			sc_buf->m_buf[sc_buf->free_list].prev = end;
		sc_buf->free_list = start;
	}
	return error_code;
}

// ���ļ��ж��� ʧ�ܷ��ش�����
int try_to_read_aux(struct tty_window* tty, unsigned int hd)
{
	int error_code = 1, tmp, start, end;
	struct screen_buf *sc_buf = &tty->buf;
	unsigned int fd = tty->fd_set[hd];
	// ��� fd �Ƿ��Ǵ򿪵��ļ�������
	if (fd < 0 || fd >= NR_OPEN)
		return (error_code = -EVLDFD);
	if (!current->filp[fd]->f_inode)
		return (error_code = -EVLDFD);
	// �ӻ�����ͷ����
	if (hd == MT_HEAD) {
		// Ԥ�� 3 * win_width ���ַ��Ŀռ�
		int count = tty->win_width * 3;
		tmp = sc_buf->free_list;
		while (count-- > 0)
			if (tmp != 0)
				tmp = sc_buf->m_buf[tmp].next;
			else
				break;
		// д���ַ����ͷſռ�
		if (count > 0)
			if ((tmp = try_to_write_aux(tty, hd)) < 0)
				return (tmp = error_code);
		// ���ļ������� fd �� �������������ַ� ���� �����ļ�ͷ
		start = sc_buf->free_list;
		end = start;
		if ((tmp = rw_to_fd(tty, fd, MT_READ, start, end)) < 0)
			return (error_code = tmp);
		// �� 0 �� �ַ�
		if (!tmp)
			return error_code;
		// �� tmp �� �ַ�
		while (--tmp > 0)
			end = sc_buf->m_buf[end].next;
		// ��������
		sc_buf->free_list = sc_buf->m_buf[end].next;
		// ������ӵ��ײ�
		sc_buf->m_buf[end].next = sc_buf->s_head;
		sc_buf->m_buf[sc_buf->s_head].prev = end;
		sc_buf->s_head = start;
	}
	// �ӻ�����β����
	else if (hd == MT_TAIL) {
		// Ԥ�� 2 * win_width ���ַ��Ŀռ�
		int count = tty->win_width * 3;
		tmp = sc_buf->free_list;
		while (count-- > 0)
			if (tmp != 0)
				tmp = sc_buf->m_buf[tmp].next;
			else
				break;
		// д���ַ����ͷſռ�
		if (count > 0)
			if ((tmp = try_to_write_aux(tty, hd)) < 0)
				return (tmp = error_code);
		// ���ļ������� fd �� �������������ַ� ���� �����ļ�β
		start = sc_buf->free_list;
		end = start;
		if ((tmp = rw_to_fd(tty, fd, MT_READ, start, end)) < 0)
			return (tmp = error_code);
		// �� 0 �� �ַ�
		if (!tmp)
			return error_code;
		// �� tmp �� �ַ�
		while (--tmp > 0)
			end = sc_buf->m_buf[end].next;
		// ��������
		sc_buf->free_list = sc_buf->m_buf[end].next;
		// ������ӵ�β��
		tmp = sc_buf->m_buf[sc_buf->s_tail].prev;
		sc_buf->m_buf[tmp].next = start;
		sc_buf->m_buf[start].prev = tmp;
		sc_buf->m_buf[end].next = sc_buf->s_tail;
		sc_buf->m_buf[sc_buf->s_tail].prev = end;
	}
	return error_code;
}

// ���ļ��ж�д
int try_to_rw(struct tty_window* tty, unsigned int ht, unsigned int rw) {
	int error_code = 1, tmp;
	if (rw != MT_READ && rw != MT_WRITE)
		return error_code;
	if (ht != MT_HEAD && ht != MT_TAIL)
		return error_code;
	if (rw == MT_READ) {
		if ((tmp = try_to_read_aux(tty, ht)) < 0)
			error_code = tmp;
	}
	else if (rw == MT_WRITE) {
		if ((tmp = try_to_write_aux(tty, ht)) < 0)
			error_code = tmp;
	}
	return error_code;
}
