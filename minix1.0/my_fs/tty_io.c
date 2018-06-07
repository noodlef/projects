#include"mix_window.h"
#include"kernel.h"
#include"file_sys.h"
#include"stat.h"
#include"fcntl.h"
#include<string.h>
// 3 ºÅÖÕ¶Ë×÷ÎªŽíÎóÊä³öÖÕ¶Ë
int MIX_STD_CERR = 0; // ŽíÎóÊä³ö
int MIX_STD_OUT = 0;  // ±ê×ŒÊä³ö
int MIX_STD_IN = 0;   // ±ê×ŒÊäÈë 

struct cursor_pos D_CURSOR_POS = { WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1 }; // ¹â±êÒþ²ØÎ»ÖÃ
struct cursor_pos origin_pos_tty[4];     // ËÄžötty Éè±ž¹â±êµÄ³õÊŒÎ»ÖÃ
struct tty_window t_window[4];
struct tty_window* tty_dev[4] =
{
	t_window,                               // ÎšÒ»µÄÊäÈëÖÕ¶Ëtty                 minor = 0
	t_window + 1,                           // ÖÕ¶Ë lp ŽòÓ¡»ú                
	t_window + 2,                           // ÖÕ¶Ë ttyx
	t_window + 3,                           // ÖÕ¶Ë unamed ÓÃÓÚÏÔÊŸ·¢ÉúµÄŽíÎó    minor = 3
};

int sys_read(unsigned int fd, char * buf, int count);
int sys_write(unsigned int fd, char * buf, int count);
int sys_lseek(unsigned int fd, mix_off_t offset, int origin);
int sys_open(const char * filename, int flag, int mode);
int try_to_rw(struct tty_window* tty, unsigned int index, unsigned int rw);

static void _init_tty(struct tty_window* tty, struct cursor_pos* origin_pos,
                      unsigned int width, unsigned int height)
{
        // 光标初始位置设定
        tty->current_pos = tty->origin_pos = *origin_pos;
        tty->win_height = height;
	tty->win_width = width;
	// 设置字体的默认显示颜色
        tty->b_color = CBC_Black;
        tty->f_color = CFC_Blue;
        tty->char_b_color = CFC_White;
        tty->button = 0;
        // 显示缓冲区初始化
        tty->buf.s_size = MAX_SCREEN_BUF_SIZE  - 10;
        tty->buf.cur_head = tty->buf.cur_tail = 0;
        tty->buf.s_head = tty->buf.s_tail = 0;
        // 链表初始化
        int j = 0;
        for (j = 1; j <= tty->buf.s_size; j++) {
                tty->buf.m_buf[j].prev = j - 1;
                tty->buf.m_buf[j].next = j + 1;
        }
        tty->buf.m_buf[j - 1].next = 0;
        tty->buf.free_list = 1;
        tty->buf.s_pos = 0;
        // 设置输入模式为 添加模式
        tty->button &= ~S_MODE;
        tty->button &= ~S_VIM;
        tty->button &= ~S_HIGHLIGHT;
        // 2018.1.14 XXXXXXXXXXXXXXXXX
        tty->fd_set[0] = -1; // 无效文件描述符
        tty->fd_set[1] = -1; // 无效文件描述符
        tty->HT_rw = &try_to_rw;
	return;
}

static void _draw_tty_frame(struct tty_window* tty, const char* name,
                            struct win_fram* frame)
{
	// 添加名字
        strcpy(tty->name, name);
        tty->frame = *frame;
	// 绘制边框
        struct cursor_pos pos, m_pos;
        pos.x = tty->origin_pos.x - frame->width;
        pos.y = tty->origin_pos.y - frame->height;
        // 
        set_console_color(frame->f_color, frame->b_color);
        for(int j = 0; j < frame->height; j++){
            m_pos.x = pos.x, m_pos.y = pos.y;
	    m_pos.y += j;
            for(int i = 0; i < (tty->win_width + 2 * frame->width); i++)
	    {
	       set_cursor_position(m_pos.x, m_pos.y);
               printf("%c", frame->shape);
               ++m_pos.x;
	    }
	}
        //
        for(int j = 0; j < frame->height; j++){
            m_pos.x = pos.x, m_pos.y = pos.y + tty->win_height
                      + frame->height;
	    m_pos.y += j;
            for(int i = 0; i < (tty->win_width + 2 * frame->width); i++)
	    {
	       set_cursor_position(m_pos.x, m_pos.y);
               printf("%c", frame->shape);
               ++m_pos.x;
	    }
	}
        // 
        for(int j = 0; j < frame->width; j++){
            m_pos.x = pos.x, m_pos.y = pos.y + frame->height;
	    m_pos.x += j;
            for(int i = 0; i < tty->win_height; i++)
	    {
	       set_cursor_position(m_pos.x, m_pos.y);
               printf("%c", frame->shape);
               ++m_pos.y;
	    }
	}
        // 
        for(int j = 0; j < frame->width; j++){
            m_pos.x = pos.x + frame->width + tty->win_width;
            m_pos.y = pos.y + frame->height;
	    m_pos.x += j;
            for(int i = 0; i < tty->win_height; i++)
	    {
	       set_cursor_position(m_pos.x, m_pos.y);
               printf("%c", frame->shape);
               ++m_pos.y;
	    }
	}
        // 显示终端的名字
        int mid = (tty->win_width - strlen(tty->name)) / 2;
        m_pos.x = pos.x + frame->width + mid;
        m_pos.y = pos.y + (frame->height / 2);
        set_cursor_position(m_pos.x, m_pos.y);
        set_console_color(CFC_Blue, CBC_Black);
        printf(tty->name);
        set_console_color_d();
	return;
}


// tty Éè±ž³õÊŒ»¯
void tty_init()
{
	window_init(0, 0);
	// tty 0
        struct win_fram frame = { ' ', 2, 3, CFC_Green, CBC_Green};
        int tty_width = WINDOW_WIDTH / 2 - frame.width * 2;
        int tty_height = WINDOW_HEIGHT - frame.height * 2;
        struct cursor_pos pos = {1 + frame.width, 1 + frame.height};
        _init_tty(tty_dev[0], &pos, tty_width, tty_height);
        _draw_tty_frame(tty_dev[0], "tty", &frame);
        // tty 1
        //frame = { ' ', 1, 1, CFC_Green, CBC_Black };
        
        tty_width = WINDOW_WIDTH / 2 - frame.width * 2;
        tty_height = WINDOW_HEIGHT - frame.height * 2;
        //pos = {1 + frame.width + WINDOW_WIDTH / 2, 1 + frame.height};
        pos.x = 1 + frame.width + WINDOW_WIDTH / 2 + 1;
        _init_tty(tty_dev[1], &pos, tty_width, tty_height);
        _draw_tty_frame(tty_dev[1], "@noodle_printer", &frame);
	set_cursor_position_d();
}

// »ØžŽtty ¹â±êÎ»ÖÃ Îª³õÊŒÎ»ÖÃ
static
void reset_tty_cursor(struct tty_window* tty)
{
	tty->current_pos = tty->origin_pos;
	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	return;
}

// žÄ±äÆÁÄ»ÏÔÊŸ×ÖÌåµÄÑÕÉ«
int set_char_fcolor(unsigned minor, enum CF_color f_color)
{
	struct tty_window* tty = tty_dev[minor];
	int ret = tty->f_color;
	tty->f_color = f_color;
	return ret;
}

// žÄ±äÆÁÄ»ÏÔÊŸ×ÖÌåµÄ±³Ÿ°ÑÕÉ«
int set_char_bcolor(unsigned minor, enum CB_color b_color)
{
	struct tty_window* tty = tty_dev[minor];
	int ret;
	if (b_color < 0)
		return -1;
	if (IS_THLIGHT(tty->button))
		ret = tty->char_b_color;
	else
		ret = -1;
	tty->button |= S_HIGHLIGHT;
	tty->char_b_color = b_color;
	return ret;
}

//
int reset_char_bcolor(unsigned minor){
	struct tty_window* tty = tty_dev[minor];
	tty->button &= ~S_HIGHLIGHT;
	return 0;
}

// žÄ±äÆÁÄ»µÄ±³Ÿ°ÑÕÉ«
int set_tty_color(unsigned minor, enum CB_color b_color)
{
	struct tty_window* tty = tty_dev[minor];
	int ret = tty->b_color;
	tty->b_color = b_color;
	return ret;
}

// ÕýÈ· - 1£¬ ŽíÎó - 0
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

// Œì²éµ±Ç°¹â±êµÄÎ»ÖÃÊÇ·ñÕýÈ·
// ÕýÈ·Õæ£¬ ŽíÎó ŒÙ
static int check_tty_cursor(struct tty_window* tty)
{
	int x = tty->current_pos.x;
	int y = tty->current_pos.y;
	if (x < tty->origin_pos.x || x >= (tty->origin_pos.x + tty->win_width)
		|| y < tty->origin_pos.y || y >= (tty->origin_pos.y + tty->win_height))
		return 0;
	return 1;
}

// pos ËùŽŠµÄÎ»ÖÃÒªÎ»ÓÚÒ»ÐÐµÄÐÐÍ·
// œ«»º³åÇøÖžÕëÉÏÒÆ  n ÐÐ£šXXXXXXXX£©
int up_line(struct tty_window* tty, int pos, int lines)
{
	// Ò»ÐÐ×î¶à¿ÉÒÔ°üº¬µÄ×Ö·ûÊý
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

// pos ËùŽŠµÄÎ»ÖÃÒªÎ»ÓÚÒ»ÐÐµÄÐÐÍ·
// œ«»º³åÇøÖžÕëÏÂÒÆ  n ÐÐ£šXXXXXXXX£©
int down_line(struct tty_window* tty, int pos, int lines)
{
	// Ò»ÐÐ×î¶à¿ÉÒÔ°üº¬µÄ×Ö·ûÊý
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


// start ÖžÊŸÆðÊŒÎ»ÖÃ£¬ ×¢Òâ
// pos Î»ÓÚ»º³åÇøÎ²ÇÒ»º³åÇøÎ²Î»ÓÚÒ»žöÐÂÐÐ£¬ ÔòÐÐÍ·ºÍÐÐÎ²¶ŒÎª»º³åÇøÎ²
// žÃº¯Êý·µ»ØŽÓ startÎ»ÖÃ¿ªÊŒµœ pos Ò»¹²¶àÉÙÐÐ£šŽÓ 0 ÊýÆð£©
// ÕÒ³öµ±Ç°Î»ÖÃËùŽŠÐÐµÄÐÐÎ² ºÍ ÐÐÍ·£šXXXXXXXX£©
static int get_line_ht_aux(struct tty_window* tty, int pos,
	int* head, int* tail, int start)
{
	int k, rows = -1;
	struct screen_buf* src = &tty->buf;
	int line1 = start, line2, i;  
	while (((line2 = down_line(tty, line1, 1)) != src->s_tail)) {
		// ¶šÎ» pos
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
	// line1 Óë line2 Î»ÓÚÍ¬Ò»ÐÐ
	if (i == line2) {
		*head = line1;
		*tail = line2;
		return rows;
	}
	// line1 Óë line2 Î»ÓÚ²»Í¬ÐÐ
	if (pos == line2) {
		*head = *tail = line2;
		return ++rows;
	}
	*head = line1;
	*tail = src->m_buf[line2].prev;
	return rows;
}

// ŽÓ cur_tail ÆðÊŒ
int get_line_ht(struct tty_window* tty, int pos,int* head, int* tail)
{
	return get_line_ht_aux(tty, pos, head, tail, tty->buf.cur_head);
}
// ŽÓ s_tail ÆðÊŒ
int get_line_ht_s(struct tty_window* tty, int pos, int* head, int* tail)
{
	return get_line_ht_aux(tty, pos, head, tail, tty->buf.s_head);
}

void print_char(struct mix_char* m_char, struct tty_window* tty);
void cursor_flash(struct tty_window* tty);

// ¹â±êÒÆ¶¯º¯Êý
// ŽíÎó·µ»Ø -1£»
int move_sbuf_cursor(int direc, struct tty_window* tty) {
	struct screen_buf* src = &tty->buf;
	int rows;
	int head, tail, pos, dis, t_head, t_tail;
	if (src->s_head == src->s_tail){
		tty->HT_rw(tty, MT_HEAD, MT_READ);
		tty->HT_rw(tty, MT_TAIL, MT_READ);
	}
	if (src->s_head == src->s_tail)
		return 0;
	switch (direc) {
	case SBUF_UP:
		// ¹â±êÉÏÒÆ
		// ¹â±êÉÏÒÆ
		// ÐÞžÄ 18.1.15
		get_line_ht_s(tty, src->s_pos, &head, &tail);
		// ŽÓÎÄŒþÖÐ¶Á
		if (head == src->s_head)
			tty->HT_rw(tty, MT_HEAD, MT_READ);
		get_line_ht_s(tty, src->s_pos, &head, &tail);
		if (head == src->s_head)
			break;
		/*  18.1.15 ÎŽÐÞžÄÇ°
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
		// ¹â±êÏÂÒÆ
		// ÐÞžÄ 18.1.15
		rows = get_line_ht(tty, src->s_pos, &head, &tail);
		if (down_line(tty, tty->buf.cur_tail, 1) == tty->buf.s_tail)
			tty->HT_rw(tty, MT_TAIL, MT_READ);
		rows = get_line_ht(tty, src->s_pos, &head, &tail);
		if (tail == src->s_tail)
			break;
		/*  18.1.15 ÎŽÐÞžÄÇ°
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
		// ÐÞžÄ 18.1.15
		if (src->s_pos == src->s_head)
			tty->HT_rw(tty, MT_HEAD, MT_READ);
		if (src->s_pos == src->s_head)
			break;
		/*  18.1.15 ÎŽÐÞžÄÇ°
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
		// ÐÞžÄ 18.1.15
		if (down_line(tty, tty->buf.cur_tail, 1) == tty->buf.s_tail)
			tty->HT_rw(tty, MT_TAIL, MT_READ);
		if (src->s_pos == src->s_tail)
			break;
		/*  18.1.15 ÎŽÐÞžÄÇ°
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


// œ«»º³åÇøµÄÖžÕë s_pos Î»ÖÃÓ³ÉäµœÆÁÄ»ÖÐµÄ¹â±êÎ»ÖÃ
struct cursor_pos
	map_to_screen(struct tty_window* tty, int pos)
{
	struct cursor_pos ret;
	// ÏÈµÃµœ pos ËùÔÚÐÐµÄÐÐÍ· ÐÐÎ²
	int line_tail, line_head, i, j;
	i = get_line_ht(tty, pos, &line_head, &line_tail);
	ret.y = tty->origin_pos.y + i;
	// µÃµœ pos µÄ x ×ø±êÖá ×ø±ê
	for (i = line_head, j = 0; i != pos && i != line_tail; j++)
		i = tty->buf.m_buf[i].next;
	ret.x = tty->origin_pos.x + j;
	return ret;
}


//  ÏòÆÁÄ»ÖÐÊä³öÒ»žö×Ö·û
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

// œ«ÏÔÊŸ»º³åÇøÖÐŽÓÎ»ÖÃ beg -- end µÄ×Ö·ûÊä³öµœÆÁÄ»
// ×¢Òâ £º žÃº¯Êý»ážüÐÂ cur_tail
static
int tty_print(struct tty_window* tty, int beg, int end)
{
	int ret = 0, c;
	struct cursor_pos cursor_pos;
	struct mix_char m_char = { ' ',CFC_White, 0, 0 };
	struct screen_buf* buf = &tty->buf;
	while (beg != end) {
		c = buf->m_buf[beg].m_char.ch;
		// žÃ×Ö·û²»ÊÇ enter œ¡ ŸÍÊä³öµœÆÁÄ»
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
		// žüÐÂµ±Ç°ÏÔÊŸÔÚÆÁÄ»ÖÐ×Ö·ûµÄÎ²²¿
		buf->cur_tail = beg;
		// »»µœÏÂÒ»ÐÐ
		if (c == KEY_ENTER || ++tty->current_pos.x >= (tty->origin_pos.x + tty->win_width)) {
			tty->current_pos.x = tty->origin_pos.x;
			if (++tty->current_pos.y >= (tty->win_height + tty->origin_pos.y)) {
				--tty->current_pos.y;
				break;
			}
			// œ«ÐÂµÄÒ»ÐÐÇå¿Õ
			cursor_pos = tty->current_pos;
			for (c = 0; c < tty->win_width; c++){
				print_char(&m_char, tty);
				++tty->current_pos.x;
			}
			tty->current_pos = cursor_pos;
		}
		// ÉèÖÃ¹â±êÎ»ÖÃ
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	}
	return ret;
}

// ÇåÆÁ
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

// Ë¢ÐÂÏÔÊŸ»º³åÇø, œ«ÏÔÊŸ»º³åÇøÖÐŽÓ 
// cur_head - cur_tail µÄ×Ö·ûÈ«²¿Êä³öµœÆÁÄ»
// žüÐÂ s_pos ÖÁ»º³åÇøÎ²
int s_flush(struct tty_window* tty) {
	int count = 0;
	struct screen_buf* sc_buf = &(tty->buf);

	// ÕûžöÆÁÄ»Çå¿Õ
	clear_window(tty);
	// »ÖžŽ¹â±êÖÃ³õÊŒÎ»ÖÃ
	reset_tty_cursor(tty);
	if (sc_buf->cur_head == sc_buf->cur_tail)
		return 0;
	// œ«×Ö·ûÊä³öµœÆÁÄ»
	count = tty_print(tty, tty->buf.cur_head, tty->buf.cur_tail);
	// ÉèÖÃµ±Ç°¹â±êµÄÎ»ÖÃ¶ÔÓŠ»º³åÇøÖÐµÄÎ»ÖÃ
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

// ÏòÏÔÊŸ»º³åÇøÖÐÐŽÊýŸÝ, ÌíŒÓÊýŸÝ£¬²»¹ÜÊÇŽÓ
// »º³åÇøÎ²²¿ÌíŒÓ£¬ »¹ÊÇŽÓÆäËûÈÎÒâÎ»ÖÃ²åÈë×Ö·û
// ×¢Òâ £º ÊýŸÝ²åÈëµÄÎ»ÖÃÓÉ buf.s_pos žø³ö
int write_screen_buf(struct mix_char* buf, int count, struct tty_window* tty)
{
	int i, k, line_head, line_tail;
	struct screen_buf* sc_buf = &(tty->buf);
	if (count <= 0)
		return 0;
	i = 0;
	// Èç¹û»º³åÇøŽËÊ±ÊÇ¿ÕµÄ
	if(sc_buf->s_head == sc_buf->s_tail) {
		if (!sc_buf->free_list)
			return -1;
		// ÏÈŽÓ×ÔÓÉÁŽ±íÖÐÈ¡³öÁœžö»º³å¿é
		sc_buf->s_head = sc_buf->free_list;
		sc_buf->s_tail = sc_buf->m_buf[sc_buf->free_list].next;
		// ×ÔÓÉÁŽ±íÖžÏòÏÂÒ»Ïî
		sc_buf->free_list = sc_buf->m_buf[sc_buf->s_tail].next;
		// ³õÊŒ»¯
		sc_buf->cur_head = sc_buf->s_head;
		sc_buf->cur_tail = sc_buf->s_tail;
		sc_buf->s_pos = sc_buf->cur_head;
		// ÐŽÈë²¢Êä³öžÃ×Ö·û
		sc_buf->m_buf[sc_buf->s_pos].m_char = buf[i++];
		count--;
		tty_print(tty, sc_buf->s_pos, sc_buf->cur_tail);
		sc_buf->s_pos = sc_buf->cur_tail;
	}
	// ÐŽÈëÊ£ÏÂµÄ×Ö·û
	// ÅÐ¶ÏÊÇŽÓ»º³åÇøÎ²²¿ÌíŒÓ£¬»¹ÊÇŽÓÖÐŒä²åÈë×Ö·û
	while (count-- > 0) {
		// ŽÓ»º³åÇøÎ²²¿ÌíŒÓ
		if (sc_buf->s_pos == sc_buf->s_tail) {
			sc_buf->m_buf[sc_buf->s_pos].m_char = buf[i++];
			// ŒÓÈëÒ»ÐÐµœ×ÔÓÉÁŽ±í

			// 18.1.15 ÌíŒÓ
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
			// ÆÁÄ»ÒÑÂú
			if (get_line_ht(tty, sc_buf->cur_tail, &line_head, &line_tail)
				>= tty->win_height) {
				sc_buf->cur_head = down_line(tty, sc_buf->cur_head, 1);
				s_flush(tty);
			}
			else {
				tty_print(tty, sc_buf->s_pos, sc_buf->cur_tail);
				sc_buf->s_pos = sc_buf->cur_tail;
			}
			// 18.1.15ÌíŒÓ
			tty->current_pos = map_to_screen(tty, sc_buf->s_pos);
			set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		}
		else {
			//  ŽÓÖÐŒä²åÈë×Ö·û
			// Èç¹ûµ±Ç°²åÈëµÄÎ»ÖÃÎª»»ÐÐ·û--ENTER,ÔòÔÚµ±Ç°Î»ÖÃÖ®Ç°²åÈë×Ö·û
			// ·ñÔòÔÚµ±Ç°Î»ÖÃÖ®ºó²åÈë×Ö·û
			if (buf[i].ch != KEY_ENTER && sc_buf->m_buf[sc_buf->s_pos].m_char.ch == KEY_ENTER)
				move_sbuf_cursor(SBUF_LEFT, tty);
			// 18.1.15 ÌíŒÓ
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
				k = sc_buf->s_pos;// 18.1.22 ÌíŒÓ
				sc_buf->cur_head = down_line(tty, sc_buf->cur_head, 1);
				s_flush(tty);
				sc_buf->s_pos = k;// 18.1.22 ÌíŒÓ
			}
			else
				tty_print(tty, sc_buf->m_buf[sc_buf->s_pos].prev, sc_buf->cur_tail);
			tty->current_pos = map_to_screen(tty, sc_buf->s_pos);
			set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		}
	}
	return 0;
}


// Ë¢ÐÂµ±Ç°¹â±êÎ»ÖÃµÄ×Ö·û
void cursor_flash(struct tty_window* tty)
{
	struct screen_buf* src = &tty->buf;
	struct mix_char m_char = src->m_buf[src->s_pos].m_char;
	if (src->s_head == src->s_tail){
		reset_tty_cursor(tty);
		return;
	}
	tty->current_pos = map_to_screen(tty, tty->buf.s_pos); // 18.1.23 ÌíŒÓ
	// Èç¹ûÊÇ»»ÐÐ·û£¬Ôò·µ»Ø
	if (src->m_buf[src->s_pos].m_char.ch == KEY_ENTER)
		return;
	if (src->s_pos == src->s_tail)
		m_char.ch = '\0';
	if (IS_APPEND(tty->button)) {
		print_char(&m_char, tty);
		return ;
	}
	// Œì²éµ±Ç°ÊÇ·ñŽŠÓÚÐÞžÄÄ£Êœ, ÉèÖÃµ±Ç°¹â±ê±³Ÿ°Îª À¶É«
	if (IS_MODIFY(tty->button)) {
		m_char.b_set = 1;
		m_char.b_color = CBC_Blue;
		print_char(&m_char, tty);
		return;
	}
	return;
}

// ÐÞžÄÓÉ¹â±êËù¶šÎ»µÄ×Ö·û
// Èç¹û¹â±êÎ»ÓÚ»»ÐÐ·ûÎ»ÖÃ£¬ ÔòÐÞžÄÄ£ÊœÎªÌíŒÓÄ£Êœ
int modify_buf(struct mix_char* m_char, struct tty_window* tty){
	int mode = tty->button;
	if (!IS_MODIFY(mode))
		return 0;
	//if (m_char->ch == KEY_ENTER)         // 2018.1.14
	//     return 0;
	if (tty->buf.s_head == tty->buf.s_tail) {
		tty->button &= ~S_MODE;
		write_screen_buf(m_char, 1, tty);
		return 0;
	}
	if (tty->buf.s_pos == tty->buf.cur_tail) {
		tty->button &= ~S_MODE;
		write_screen_buf(m_char, 1, tty);
		return 0;
	}
	if (m_char->ch == KEY_ENTER)         // 2018.1.14
		return 0;
	// ²»ÄÜÐÞžÄ»»ÐÐ·û
	if (tty->buf.m_buf[tty->buf.s_pos].m_char.ch == KEY_ENTER){
		tty->button &= ~S_MODE;
		write_screen_buf(m_char, 1, tty);
		return 0;
	}
	tty->buf.m_buf[tty->buf.s_pos].m_char = *m_char;
	cursor_flash(tty);
	//print_char(m_char, tty);
	return 0;
}


// œ«ÆÁÄ»ÖÐµ±Ç°¹â±êËùŽŠÎ»ÖÃŒ°Ö®ºóÎ»ÖÃÈ«²¿Çå¿Õ
// ²¢»ÖžŽ¹â±êµœÔ­Î»ÖÃ
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
	// Çå¿Õ×îºóÒ»ÐÐ
	if (tty->current_pos.y < (tty->win_height + tty->origin_pos.y))
		for (; tty->current_pos.x < cursor_end.x; ++tty->current_pos.x)
			print_char(&m_char, tty);
	tty->current_pos = cursor_pos;
	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
}

// ÉŸ³ý¹â±êËù¶šÎ»µÄ×Ö·û
// Èç¹ûµ±Ç°ÊÇÐÞžÄÄ£Êœ£¬ Ôò±äÎªÌíŒÓÄ£Êœ
int delete_buf(struct tty_window* tty) {
	int pos, next, prev;	
	struct cursor_pos cursor_pos;
	if (tty->buf.s_head == tty->buf.s_tail)
		return 0;
	// Èç¹ûÊÇÐÞžÄÄ£Êœ£¬ ±äÎªÌíŒÓÄ£Êœ
	if (IS_MODIFY(tty->button)){
		tty->button &= ~S_MODE;
		cursor_flash(tty);
	}
	// 18.1.15 ÌíŒÓ
	if (down_line(tty, tty->buf.cur_tail, 1) == tty->buf.s_tail)
		tty->HT_rw(tty, MT_TAIL, MT_READ);
	if (tty->buf.s_pos == tty->buf.s_head)
		tty->HT_rw(tty, MT_HEAD, MT_READ);
	// end
	if (tty->buf.s_pos == tty->buf.s_tail) {
		pos = tty->buf.s_pos;
		move_sbuf_cursor(SBUF_LEFT, tty);
		// ŒÓÈë×ÔÓÉÁŽ±í
		tty->buf.m_buf[pos].next = tty->buf.free_list;
		if (tty->buf.free_list)
			tty->buf.m_buf[tty->buf.free_list].prev = pos;
		tty->buf.free_list = pos;

		clear_pos_tty(tty);
		// žüÐÂÎ²²¿ÖžÕë
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
		// ÒÆ¶¯¹â±êÎ»ÖÃ
		move_sbuf_cursor(SBUF_RIGHT, tty);
		// ŒÓÈë×ÔÓÉÁŽ±í
		tty->buf.m_buf[pos].next = tty->buf.free_list;
		if (tty->buf.free_list)
			tty->buf.m_buf[tty->buf.free_list].prev = pos;
		tty->buf.free_list = pos;
		// 
		pos = tty->buf.s_pos;
		tty->buf.s_head = tty->buf.cur_head = next;
		tty->buf.cur_tail = tty->buf.s_tail; //bug--2018.1.22, ÌíŒÓ
		s_flush(tty);
		tty->buf.s_pos = pos;
		tty->current_pos = map_to_screen(tty, tty->buf.s_pos);
		set_cursor_position(tty->current_pos.x, tty->current_pos.y);
		return 0;
	}
	// ÉŸ³ý¹â±êÎ»ÖÃºóµÄËùÓÐ×Ö·û
	clear_pos_tty(tty);
	// ŒÓÈë×ÔÓÉÁŽ±í
	prev = tty->buf.m_buf[tty->buf.s_pos].prev;
	next = tty->buf.m_buf[tty->buf.s_pos].next;
	pos = tty->buf.s_pos;
	// ÒÆ¶¯¹â±êÎ»ÖÃ
	move_sbuf_cursor(SBUF_LEFT, tty);
	// ŒÓÈë×ÔÓÉÁŽ±í
	tty->buf.m_buf[pos].next = tty->buf.free_list;
	if (tty->buf.free_list)
		tty->buf.m_buf[tty->buf.free_list].prev = pos;
	tty->buf.free_list = pos;
	// œšÁ¢Á¬œÓ
	tty->buf.m_buf[prev].next = next;
	tty->buf.m_buf[next].prev = prev;
	// Èç¹ûÉŸ³ýµÄ×Ö·ûÊÇµ±Ç°ÆÁÄ»ÏÔÊŸµÄÊ××Ö·û
	if (tty->buf.cur_head == pos)
		tty->buf.cur_head = tty->buf.s_pos;
	//clear_pos_tty(tty);
	// ÖØÐÂÊä³öÉŸ³ýÎ»ÖÃºóµÄËùÓÐ×Ö·û
	//pos = tty->buf.s_pos;
	cursor_pos = tty->current_pos;
	tty_print(tty, tty->buf.s_pos, tty->buf.s_tail);
	//tty->buf.s_pos = pos;
	tty->current_pos = cursor_pos;
	set_cursor_position(tty->current_pos.x, tty->current_pos.y);
	return 0;
}

extern int sys_print_pwd(char* buf, int size);
extern char M_user[];
// ŽòÓ¡µ±Ç°µÄ¹€×÷Ä¿ÂŒ
int print_cur_dir()
{
	int minor = 0, i, j, count;
	char dir[1024];
	char* user = M_user;//"noodle";
	struct mix_char m_char[1024];
	struct mix_char brack[4] = { { '[', CFC_Purple, 0, 0 },
	{ ']', CFC_Purple, 0, 0 }, { '>', CFC_Purple, 0, 0 }, 
	{ '>', CFC_Purple, 0, 0 } };

	tty_write(minor, brack, 1);
	if ((count = sys_print_pwd(dir, 1024)) < 0)
		return count;
	i = 1024 - count;
	j = 0;
	while (count-- > 0){
		m_char[j].ch = dir[i++];
		m_char[j].b_set = 0;
		m_char[j++].f_color = CFC_Purple;
	}
	tty_write(minor, m_char, j);

	i = j = 0;
	m_char[j].ch = '@';
	m_char[j].b_set = 0;
	m_char[j++].f_color = CFC_Cyan;
	count = strlen(user);
	while (count-- > 0){
		m_char[j].ch = user[i++];
		m_char[j].b_set = 0;
		m_char[j++].f_color = CFC_Blue;
	}
	tty_write(minor, m_char, j);
	tty_write(minor, brack + 1, 1);
	tty_write(minor, brack + 2, 2);
	return 1;
}


// ŽíÎóÊä³ö
int mix_print(struct mix_char* buf, int count, unsigned minor) {
	struct tty_window* tty = tty_dev[minor];
	write_screen_buf(buf, count, tty);
	return 0;
}

// »»ÐÐ
int new_line(unsigned minor)
{
	struct tty_window* tty = tty_dev[minor];
	struct mix_char c = { KEY_ENTER, tty->f_color, tty->b_color, 0 };
	tty_write(minor, &c, 1);
	return 0;
}

// ÇåÆÁ²¢Çå¿ÕÏÔÊŸ»º³åÇø
int clc_win(unsigned minor) {
	struct tty_window* tty = tty_dev[minor];
	struct screen_buf* src_buf = &(tty->buf);
	int i, j;
	// ÇåÆÁ
	clear_window(tty);
	reset_tty_cursor(tty);
	// Çå¿ÕÏÔÊŸ»º³åÇø
	for (j = 1; j <= tty->buf.s_size; j++) {
		tty->buf.m_buf[j].prev = j - 1;
		tty->buf.m_buf[j].next = j + 1;

	}
	tty->buf.m_buf[j - 1].next = 0;
	tty->buf.free_list = 1;
	// ÖžÕëÖØÖÃ
	tty->buf.s_pos = 0;
	src_buf->cur_head = src_buf->cur_tail = 0;
	src_buf->s_head = src_buf->s_tail = 0;
	return 0;
}

// ŽíÎóÊä³ö
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

// ÖÕ¶Ë¶Á
int get_key();
int tty_read(unsigned minor, char * buf, int count)
{
	// »ñÈ¡ÖÕ¶Ë
	int i, j, s_pos;
	struct mix_char m_char;
	struct tty_window* tty = tty_dev[minor];
	if (!check_tty_cursor(tty)) {
		clc_win(minor);
		mix_cerr("tty_read : tty-%d cursor out of boundary!");
		return -1;
	}
	// 0 ºÅÖÕ¶Ë×÷ÎªÎšÒ»µÄÃüÁîÐÐÖÕ¶Ë
	if (minor == 0) {
		// ŽòÓ¡µ±Ç°¹€×÷Ä¿ÂŒ
		print_cur_dir();
		// ŒÇÂŒµ±Ç°¹â±êÎ»ÖÃ
		s_pos = tty->buf.m_buf[tty->buf.s_pos].prev;
		// 
		do{
			m_char.ch = keybord(tty);
			// Èç¹û¹â±êÎ»ÖÃÎ»ÓÚ [@noodle] Ç°Ãæ£¬ÔòÊ²ÃŽÒ²²»×ö
			if (!find_back_pos(tty, tty->buf.m_buf[s_pos].next, tty->buf.s_pos))
				continue;
			// Èç¹ûÒªÉŸ³ý
			if (m_char.ch == KEY_BACKSPACE) {
				if (tty->buf.s_tail != tty->buf.m_buf[s_pos].next)
					delete_buf(tty);
				continue;
			}
			// Ã»ÓÐÉèÖÃÎÄ±ŸžßÁÁÏÔÊŸ
			if (!IS_THLIGHT(tty->button))
				m_char.b_set = 0;
			else {
				m_char.b_set = 1;
				m_char.b_color = tty->char_b_color;
			}
			m_char.f_color = tty->f_color;
			// ÐŽÈë
			tty_write(minor, &m_char, 1);
		} while (m_char.ch != KEY_ENTER || tty->buf.s_pos != tty->buf.s_tail);
		// È¡³öÃüÁî, ŽÓ s_pos ÖÁ s_tail Ö®ŒäµÄËùÓÐ×Ö·û
		// Èç¹ûÓöµœ»»ÐÐ·ûÔòÌø¹ý
		s_pos = tty->buf.m_buf[s_pos].next;
		for (i = 0; s_pos != tty->buf.s_tail; s_pos = tty->buf.m_buf[s_pos].next) {
			if (tty->buf.m_buf[s_pos].m_char.ch != KEY_ENTER)
				buf[i++] = tty->buf.m_buf[s_pos].m_char.ch;
		}
		return ++i;
	}
	// Ö±œÓ»ñÈ¡ŒüÅÌÊäÈëµÄ×Ö·û
	i = 0;
	while (count-- > 0) 
		buf[i++] = get_key();
	return i;
}


// µÈŽý¿ØÖÆÌšÊäÈëÒ»ÐÐ
// mask ÑÚÂë£¬µ±mask²»µÈÓÚ0Ê±ÔÚÆÁÄ»ÏÔÊŸ maskËù±íÊŸµÄ×Ö·û
int wait_for_line(unsigned int minor, char* m_buf,
	unsigned int count, char mask){
	int i, j, s_pos;
	struct mix_char m_char;
	struct tty_window* tty = tty_dev[minor];

	minor = 0;
	// count = 0, ±íÊŸÖ±ÖÁ°ŽÏÂenter²Å·µ»Ø
	if (!count)
		count = -1;
	// ŒÇÂŒµ±Ç°¹â±êÎ»ÖÃ
	s_pos = tty->buf.m_buf[tty->buf.s_pos].prev;
	// 
	do{
		m_char.ch = keybord(tty);
		// Èç¹û¹â±êÎ»ÖÃÎ»ÓÚ [@noodle] Ç°Ãæ£¬ÔòÊ²ÃŽÒ²²»×ö
		if (!find_back_pos(tty, tty->buf.m_buf[s_pos].next, tty->buf.s_pos))
			continue;
		// Èç¹ûÒªÉŸ³ý
		if (m_char.ch == KEY_BACKSPACE) {
			if (tty->buf.s_tail != tty->buf.m_buf[s_pos].next){
				delete_buf(tty);
				++count;
			}
			continue;
		}
		// 
		if (count <= 0 && IS_APPEND(tty->button))
			continue;
		if (count <= 0 && IS_MODIFY(tty->button) &&
			(tty->buf.s_pos == tty->buf.s_tail))
			continue;
		// Ã»ÓÐÉèÖÃÎÄ±ŸžßÁÁÏÔÊŸ
		if (!IS_THLIGHT(tty->button))
			m_char.b_set = 0;
		else {
			m_char.b_set = 1;
			m_char.b_color = tty->char_b_color;
		}
		m_char.f_color = tty->f_color;
		// ÐŽÈë
		if (m_char.ch != KEY_ENTER){
			// * * * * * *
			/*if (mask)
				m_char.ch = mask;*/
			tty_write(minor, &m_char, 1);
			if (IS_APPEND(tty->button))
				--count;
		}
	} while (m_char.ch != KEY_ENTER || tty->buf.s_pos != tty->buf.s_tail);
	// È¡³öÃüÁî, ŽÓ s_pos ÖÁ s_tail Ö®ŒäµÄËùÓÐ×Ö·û
	// Èç¹ûÓöµœ»»ÐÐ·ûÔòÌø¹ý
	s_pos = tty->buf.m_buf[s_pos].next;
	for (i = 0; s_pos != tty->buf.s_tail; s_pos = tty->buf.m_buf[s_pos].next) 
			m_buf[i++] = tty->buf.m_buf[s_pos].m_char.ch;
	return i;
}

// ÖÕ¶ËÐŽ, ×¢ÒâÖ»ÄÜÐŽÈë¿ÉÏÔÊŸ×Ö·û
// ²»ÄÜÐŽÈë¿ØÖÆ×Ö·û
int tty_write(unsigned minor, struct mix_char * buf, int count)
{
	// »ñÈ¡ÖÕ¶Ë
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
		return ;
	}
	while (1) {
		m_char.ch = keybord(tty);

		// Èç¹ûÒªÉŸ³ý
		if (m_char.ch == KEY_BACKSPACE) {
			delete_buf(tty);
			continue;
		}
		// Ã»ÓÐÉèÖÃÎÄ±ŸžßÁÁÏÔÊŸ
		if (!IS_THLIGHT(tty->button))
			m_char.b_set = 0;
		else {
			m_char.b_set = 1;
			m_char.b_color = tty->char_b_color;
		}
		m_char.f_color = tty->f_color;
		// ÐŽÈë
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
//				// ÐŽÈë
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
//		// ¶ÁÈë»º³åÇø
//		for (beg = 0; beg < count; beg++) {
//			f_pos -= sizeof(struct mix_char);
//			m_char[beg] = *((struct mix_char*)(buf + f_pos));
//		}
//		// ÐŽÈë
//		for (beg = 0; beg < count; beg++) {
//			sc_buf->m_buf[start].m_char = m_char[beg];
//			start = sc_buf->m_buf[start].next;
//		}
//	}
//	else if (rw == MT_READ && fd == 0) {
//		count = (tty->win_width + 1) * 2; // XXXXXXXXXX
//		if ((f_pos / sizeof(struct mix_char)) < count)
//			count = (f_pos / sizeof(struct mix_char));
//		// ¶ÁÈë»º³åÇø
//		for (beg = 0; beg < count; beg++) {
//			f_pos -= sizeof(struct mix_char);
//			m_char[beg] = *((struct mix_char*)(buf + f_pos));
//		}
//		// ·ÖÐÐ
//		if (count == (tty->win_width + 1) * 2) {
//			for (beg = 0, k = 0; beg != count; beg++) {
//				if (m_char[beg].ch == line.ch)
//					k = beg;
//			}
//			f_pos = f_pos + (beg - k) * sizeof(struct mix_char);
//		}
//		else
//			k = count;
//		// ÐŽÈë
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
	// ·ŽË³ÐòÊä³ö
	tmp = 0, index = count;
	while (index-- > 0)
		m_buf[tmp++] = tmp_buf[index];
	return count ;
}
// ¶ÔÎÄŒþÃèÊö·ûfd¶ÔÓŠµÄÎÄŒþŸ¡ÐÄ¶ÁÐŽ
int rw_to_fd(struct tty_window* tty, unsigned int fd, int rw,
	int start, int end) {
	struct screen_buf* sc_buf = &tty->buf;
	int count, k, beg, error_code;
	struct mix_char m_char[WINDOW_WIDTH * 5];
	struct mix_char L_char = { 0,0,0,M_NEWLINE}; // ÕŒÎ»·û£¬Ã¿ÐÐµÄœáÎ²ÌíŒÓÒ»žöXXXXXX
	
	// Í·²¿ÐŽ
	if (rw == MT_WRITE && fd == tty->fd_set[MT_HEAD]) {
		for (beg = 0, count = 0; start != end; count++) {
			m_char[beg++] = sc_buf->m_buf[start].m_char;
			if ((beg == tty->win_width) ||
				(sc_buf->m_buf[start].m_char.ch == KEY_ENTER)) {
				m_char[beg++] = L_char;
				// œ«Ò»ÐÐ×Ö·ûÐŽÈë fd 
				if ((error_code = write_to_fd(m_char, beg, fd)) < 0)
					return error_code;
				beg = 0;
			}
			start = sc_buf->m_buf[start].next;
		}
	}
	// Î²²¿ÐŽ
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
	// Î²²¿¶Á
	else if (rw == MT_READ && fd == tty->fd_set[MT_TAIL]) {
		count = tty->win_width * 2;
		// ¶Á³ö
		if ((count = read_from_fd(m_char, count, fd)) < 0)
			return count;
		// ÐŽÈë
		for (beg = 0; beg < count; beg++) {
			sc_buf->m_buf[start].m_char = m_char[beg];
			start = sc_buf->m_buf[start].next;
		}
	}
	// Í·²¿¶Á
	else if (rw == MT_READ && fd == tty->fd_set[MT_HEAD]) {
		count = (tty->win_width + 1) * 2; // ¶Á³öÁœÐÐ  XXXXXXXXXX
	    // ¶Á³ö
		if ((count = read_from_fd(m_char, count, fd)) < 0)
			return count;
		// ·ÖÐÐ
		if (count == (tty->win_width + 1) * 2) {
			for (beg = 0, k = 0; beg != count; beg++) {
				//if (m_char[beg].ch == L_char.ch)
				if (IS_NEWLINE(m_char[beg]))
					k = beg;
			}
			if((error_code = sys_lseek(fd, (count - k) 
				             * sizeof(struct mix_char), SEEK_CUR)) < 0)
				return error_code;
		}
		else
			k = count;
		// ÐŽÈë
		count = 0;
		while (k-- > 0) {
			//if (m_char[k].ch != L_char.ch)
			if (!IS_NEWLINE(m_char[k])){
				sc_buf->m_buf[start].m_char = m_char[k];
				start = sc_buf->m_buf[start].next;
				++count;
			}
		}
	}
	return count;
}


// ŽÓÎÄŒþÖÐÐŽ£¬ Ê§°Ü·µ»ØŽíÎóÂë
int try_to_write_aux(struct tty_window* tty, unsigned int hd)
{
	int error_code = 1, tmp, start, end;
	struct screen_buf *sc_buf = &tty->buf;
	unsigned int fd = tty->fd_set[hd];
	int reserve = 3, s_ht;
	int head, tail, lines;
	// Œì²é fd ÊÇ·ñÊÇŽò¿ªµÄÎÄŒþÃèÊö·û
	if (fd < 0 || fd >= NR_OPEN)
	return (error_code = -EVLDFD);
	if (!current->filp[fd]->f_inode)
	return (error_code = -EVLDFD);
	// Í·²¿ºÍÎ²²¿ž÷±£ÁôÖÁÉÙ reserve ÐÐ
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

// ŽÓÎÄŒþÖÐ¶Á£¬ Ê§°Ü·µ»ØŽíÎóÂë
int try_to_read_aux(struct tty_window* tty, unsigned int hd)
{
	int error_code = 1, tmp, start, end;
	struct screen_buf *sc_buf = &tty->buf;
	unsigned int fd = tty->fd_set[hd];
	// Œì²é fd ÊÇ·ñÊÇŽò¿ªµÄÎÄŒþÃèÊö·û
	if (fd < 0 || fd >= NR_OPEN)
		return (error_code = -EVLDFD);
	if (!current->filp[fd]->f_inode)
		return (error_code = -EVLDFD);
	// ŽÓ»º³åÇøÍ·²¿¶Á
	if (hd == MT_HEAD) {
		// Ô€Áô 3 * win_width žö×Ö·ûµÄ¿ÕŒä
		int count = tty->win_width * 3;
		tmp = sc_buf->free_list;
		while (count-- > 0)
			if (tmp != 0)
				tmp = sc_buf->m_buf[tmp].next;
			else
				break;
		// ÐŽÈë×Ö·û£¬ÊÍ·Å¿ÕŒä
		if (count > 0)
			if ((tmp = try_to_write_aux(tty, hd)) < 0)
				return (tmp = error_code);
		// ŽÓÎÄŒþÃèÊö·û fd ÖÐ ¶Á³öÖÁÉÙÁœÐÐ×Ö·û »òÕß µœŽïÎÄŒþÍ·
		start = sc_buf->free_list;
		end = start;
		if ((tmp = rw_to_fd(tty, fd, MT_READ, start, end)) < 0)
			return (error_code = tmp);
		// ¶Á 0 žö ×Ö·û
		if (!tmp)
			return error_code;
		// ¶Á tmp žö ×Ö·û
		while (--tmp > 0)
			end = sc_buf->m_buf[end].next;
		// ×ÔÓÉÁŽ±í
		sc_buf->free_list = sc_buf->m_buf[end].next;
		// ÊýŸÝÌíŒÓµœÊ×²¿
		sc_buf->m_buf[end].next = sc_buf->s_head;
		sc_buf->m_buf[sc_buf->s_head].prev = end;
		sc_buf->s_head = start;
	}
	// ŽÓ»º³åÇøÎ²²¿¶Á
	else if (hd == MT_TAIL) {
		// Ô€Áô 2 * win_width žö×Ö·ûµÄ¿ÕŒä
		int count = tty->win_width * 3;
		tmp = sc_buf->free_list;
		while (count-- > 0)
			if (tmp != 0)
				tmp = sc_buf->m_buf[tmp].next;
			else
				break;
		// ÐŽÈë×Ö·û£¬ÊÍ·Å¿ÕŒä
		if (count > 0)
			if ((tmp = try_to_write_aux(tty, hd)) < 0)
				return (tmp = error_code);
		// ŽÓÎÄŒþÃèÊö·û fd ÖÐ ¶Á³öÖÁÉÙÁœÐÐ×Ö·û »òÕß µœŽïÎÄŒþÎ²
		start = sc_buf->free_list;
		end = start;
		if ((tmp = rw_to_fd(tty, fd, MT_READ, start, end)) < 0)
			return (tmp = error_code);
		// ¶Á 0 žö ×Ö·û
		if (!tmp)
			return error_code;
		// ¶Á tmp žö ×Ö·û
		while (--tmp > 0)
			end = sc_buf->m_buf[end].next;
		// ×ÔÓÉÁŽ±í
		sc_buf->free_list = sc_buf->m_buf[end].next;
		// ÊýŸÝÌíŒÓµœÎ²²¿
		tmp = sc_buf->m_buf[sc_buf->s_tail].prev;
		sc_buf->m_buf[tmp].next = start;
		sc_buf->m_buf[start].prev = tmp;
		sc_buf->m_buf[end].next = sc_buf->s_tail;
		sc_buf->m_buf[sc_buf->s_tail].prev = end;
	}
	return error_code;
}

// ŽÓÎÄŒþÖÐ¶ÁÐŽ
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



