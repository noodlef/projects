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
// ���������ҷ����ʱ�� _getch()�᷵������ֵ
// 32 - 126 Ϊ�ɴ�ӡ�ַ���ASCII��ֵ�� 0 - 31 Ϊ�����ַ��� 127 -- delete
// 128 - 255  ��չ�ַ�
int get_key() {
	int ascii;
repeat:
	ascii = _getch();
	// ���ڿɴ�ӡ�ַ������� enter ֱ�ӷ��� 
	if (ascii > 31 && ascii < 127 
		|| ascii == KEY_ENTER || ascii == KEY_BACKSPACE)
		return ascii;
	// �����¿����ַ�ʱ�� _getch()�᷵������ֵ����һ��Ϊ 0xE0 �� 0x00
	if (ascii == 0xE0) {
		ascii = _getch();
		switch (ascii) {
		case UP_ARROW:
		case DOWN_ARROW:
		case LEFT_ARROW:
		case RIGHT_ARROW:
		// ���� del ����
		case C_INS:
			// ��ֹ��ɴ�ӡ�ַ���ֵ��ͬ
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
	// ��鵱ǰ�Ƿ����޸�ģʽ, ���õ�ǰ��걳��Ϊ ��ɫ
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


// �ȴ�����
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
			// �����ǰ�����ģʽ����Ϊ�޸�ģʽ
			// ��֮��Ȼ
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
			// ֻ���� enter, backspace, �� �ɴ�ӡ�ַ�
			// ������������ֱ�Ӻ��Ե���
			if (ch == KEY_ENTER  || ch == KEY_BACKSPACE || (ch > 31 && ch  < 127))
				lock = 0;
			break;
		}
	}
	return ch;
}



