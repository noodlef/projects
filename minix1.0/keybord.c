#include"mix_window.h"
#include"conio.h"
enum {
	UP_ARROW = 72,
	DOWN_ARROW = 80,
	LEFT_ARROW = 75,
	RIGHT_ARROW = 77,
	C_INS = 82,
	UP_W = 72 + 0xE0,
	DOWN_W = 80 + 0xE0,
	LEFT_W = 75 +0xE0,
	RIGHT_W = 77 + 0xE0,
	INS = 82 + 0xE0
};
// 按上下左右方向键时， _getch()会返回两个值
// 32 - 126 为可打印字符的ASCII码值， 0 - 31 为控制字符， 127 -- delete
// 128 - 255  扩展字符
int get_key() {
	int ascii;
repeat:
	ascii = _getch();
	// 对于可打印字符或者是 enter 直接返回 
	if (ascii > 31 && ascii < 127 
		|| ascii == KEY_ENTER || ascii == KEY_BACKSPACE)
		return ascii;
	// 当按下控制字符时， _getch()会返回两个值，第一个为 0xE0 或 0x00
	if (ascii == 0xE0) {
		ascii = _getch();
		switch (ascii) {
		case UP_ARROW:
		case DOWN_ARROW:
		case LEFT_ARROW:
		case RIGHT_ARROW:
		// 按下 del 按键
		case C_INS:
			// 防止与可打印字符的值相同
			ascii += 0xE0;
			break;
		default:
			goto repeat;
		}
	}
	// 0x00
	else {
		ascii = _getch();
		goto repeat;
	}
	return ascii;
}


static int move_cursor(int direc, struct tty_window* tty){
	if (tty->buf.s_head == tty->buf.s_tail)
		return 0;
	// 检查当前是否处于修改模式, 设置当前光标背景为 蓝色
	if (IS_MODIFY(tty->button)) {
		tty->button &= ~AM_MODE;
		cursor_flash(tty);
		move_sbuf_cursor(direc, tty);
		tty->button |= AM_MODE;
		cursor_flash(tty);
	}
	else
		move_sbuf_cursor(direc, tty);
	return 0;
}


// 等待按键
int keybord(struct tty_window* tty)
{
	struct screen_buf* src = &tty->buf;
	int ch, lock = 1;

	while (lock) {
		ch = get_key();
		switch (ch) {
		case LEFT_W:
			move_cursor(SBUF_LEFT, tty);
			break;
		case UP_W:
		    move_cursor(SBUF_UP, tty);
			break;
		case RIGHT_W:
			move_cursor(SBUF_RIGHT, tty);
			break;
		case DOWN_W:
			move_cursor(SBUF_DOWN, tty);
			break;
		case INS:
			if (src->s_tail == src->s_head)
				break;
			// 如果当前是添加模式，变为修改模式
			// 反之亦然
			if (IS_APPEND(tty->button)) {
				tty->button |= AM_MODE;
				cursor_flash(tty);
			}
			else {
				tty->button &= ~AM_MODE;
				cursor_flash(tty);
			}
			break;
		default:
			// 只处理 enter, backspace, 和 可打印字符
			// 对其它的输入直接忽略掉了
			if (ch == KEY_ENTER  || ch == KEY_BACKSPACE || (ch > 31 && ch  < 127))
				lock = 0;
			break;
		}
	}
	return ch;
}



