#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include"mix_window.h"
#include<termios.h>
#include<sys/ioctl.h>
#include<unistd.h>
enum {
	UP_ARROW = 65,
	DOWN_ARROW = 66,
	LEFT_ARROW = 68,
	RIGHT_ARROW = 67,
	C_INS = 50,
	UP_W = 65 + 0xE0,
	DOWN_W = 66 + 0xE0,
	LEFT_W = 68 +0xE0,
	RIGHT_W = 67 + 0xE0,
	INS = 50 + 0xE0
};
int getch(void)
{
     struct termios tm, tm_old;
     int fd = 0, ch;

     if (tcgetattr(fd, &tm) < 0) {
          return -1;
     }

     tm_old = tm;
     cfmakeraw(&tm);
     if (tcsetattr(fd, TCSANOW, &tm) < 0) {
          return -1;
     }

     ch = getchar();
     if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {
          return -1;
     }

     return ch;
}
// 按上下左右方向键时， getch()会返回两个值
// 32 - 126 为可打印字符的ASCII码值， 0 - 31 为控制字符， 127 -- delete
// 128 - 255  扩展字符
int get_key() {
	int ascii;
repeat:
	ascii = getch();
	// 对于可打印字符或者是 enter 直接返回 
	if (ascii > 31 && ascii < 127 //|| ascii == KEY_ESC
		|| ascii == KEY_ENTER || ascii == KEY_BACKSPACE)
		return ascii;
	// 当按下控制字符时， getch()会返回两个值，第一个为 0xE0 或 0x00
	if (ascii == 27) {
		ascii = getch();
                ascii = getch();
		if(ascii == 50)
			return ascii += 0xE0;
		switch (ascii) {
		case UP_ARROW:
		case DOWN_ARROW:
		case LEFT_ARROW:
		case RIGHT_ARROW:
			ascii += 0xE0;
			break;
		default:
			goto repeat;
		}
	}
	// 0x00
	else {
		ascii = getch();
		goto repeat;
	}
	return ascii;
}


static int move_cursor(int direc, struct tty_window* tty){
	if (tty->buf.s_head == tty->buf.s_tail)
		return 0;
	// 检查当前是否处于修改模式, 设置当前光标背景为 蓝色
	if (IS_MODIFY(tty->button)) {
		tty->button &= ~S_MODE;
		cursor_flash(tty);
		move_sbuf_cursor(direc, tty);
		tty->button |= S_MODE;
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
				tty->button |= S_MODE;
				cursor_flash(tty);
			}
			else {
				tty->button &= ~S_MODE;
				cursor_flash(tty);
			}
			break;
		default:
			// 打印机模式
			if (IS_PRINTER(tty->button)){
				if (ch == KEY_ESC)
					lock = 0;
				break;
			}
			// 只处理 enter, backspace, 和 可打印字符
			// 对其它的输入直接忽略掉了
			if (ch == KEY_ENTER  || ch == KEY_BACKSPACE || (ch > 31 && ch  < 127))
				lock = 0;
			if (ch == KEY_ESC && IS_VIM(tty->button))
				lock = 0;
			break;
		}
	}
	return ch;
}


// 用于 vim
int vim_keybord(){
	int ch, lock = 1;
	while (lock) {
		ch = get_key();
		switch (ch) {
		case LEFT_W:
			return SBUF_LEFT;
		case RIGHT_W:
			return SBUF_RIGHT;
		case KEY_ENTER:
			return ch;
		default:
			break;
		}
	}
	return 100;
}

