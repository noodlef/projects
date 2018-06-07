#include"mix_window.h"
// 将字符串输出到标准输出设备上 -- ttyx
int console_print(char* s, int minor)
{
	enum { SZ = 50 };
	// 2 号 tty
	minor = 2;
	int left = strlen(s), count, i = 0, j, k;
	struct tty_window* tty = tty_dev[minor];
	struct mix_char buf[SZ];
	// 换行
	if (left >= 1 && (s[left - 1] == '\n')) {
		s[left - 1] = KEY_ENTER;
	}
	while (left > 0) {
		if (left > SZ)
			count = SZ;
		else
			count = left;
		left -= count;
		k = count;
		for (j = 0; k-- > 0;) {
			buf[j].ch = s[i++];
			buf[j].f_color = tty->f_color;
			buf[j++].b_set = 0;
		}
		tty_write(minor, buf, count);
	}
	return 0;
}

// 判断是否为数字
#define is_digit(c)	((c) >= '0' && (c) <= '9')
// 将字符数字串转化为整数
static int skip_atoi(const char **s)
{
	int i = 0;
	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1		/* pad with zero */                             // 填充 0
#define SIGN	2		/* unsigned/signed long */                      // 有符号 / 无符号 长整数
#define PLUS	4		/* show plus */                                 // 显示 +
#define SPACE	8		/* space if plus */                             // 如果是 + 就置空格
#define LEFT	16		/* left justified */                            // 左调整
#define SPECIAL	32		/* 0x */                                        // 0x
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */          // 使用小写

// 除法， 返回余数
static unsigned long do_div(unsigned long* n, int base)
{
	unsigned long res;
	res = *n % base;
	*n = *n / base;
	return res;
}

// 将整数转换成指定进制的字符串
// para : num -- 要转换的整数， base -- 进制
static char * number(char* str, int num, int base,
	                 int size, int precision, int type)
{
	char c, sign, tmp[36];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type&SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (type&LEFT) type &= ~ZEROPAD;
	if (base<2 || base>36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	if (type&SIGN && num<0) {
		sign = '-';
		num = -num;
	}
	else
		sign = (type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);
	if (sign) size--;
	if (type&SPECIAL)
		if (base == 16) size -= 2;
		else if (base == 8) size--;
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else while (num != 0)
		tmp[i++] = digits[do_div(&num, base)];
	if (i>precision) precision = i;
	size -= precision;
	if (!(type&(ZEROPAD + LEFT)))
		while (size-->0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type&SPECIAL)
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	if (!(type&LEFT))
		while (size-->0)
			*str++ = c;
	while (i<precision--)
		*str++ = '0';
	while (i-->0)
		*str++ = tmp[i];
	while (size-->0)
		*str++ = ' ';
	return str;
}

// 转换浮点数
static char* float_number(char* str, double num, 
	                      int size, int precision, int type)
{
	char c, sign, tmp[36];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i,j;
	unsigned long integ;
	double fract;

	if (type&LEFT) type &= ~ZEROPAD;
	c = (type & ZEROPAD) ? '0' : ' ';
	if (type&SIGN && num<0) {
		sign = '-';
		num = -num;
	}
	else
		sign = (type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);
	if (sign) size--;
	
	
	
	// 先得到小数部分, 最少6位小数
	i = 0;
	integ = (unsigned long)num;
	fract = num - integ;
	if (precision < 6)
		precision = 6;
	integ = 1;
	for (j = 0; j < precision; j++)
		integ *= 10;
	integ = (long)(fract * integ);
	if (!integ)
		for (j = 0; j < precision; j++)
			tmp[i++] = '0';
	else while (integ != 0)
		tmp[i++] = digits[do_div(&integ, 10)];
	tmp[i++] = '.';
	// 再得到整数部分
	integ = (unsigned long)num;
	if (integ == 0)
		tmp[i++] = '0';
	else while (integ != 0)
		tmp[i++] = digits[do_div(&integ, 10)];
	// 
	if (i>precision) precision = i;
	size -= precision;
	if (!(type&(ZEROPAD + LEFT)))
		while (size-->0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (!(type&LEFT))
		while (size-->0)
			*str++ = c;
	while (i<precision--)
		*str++ = '0';
	while (i-->0)
	    *str++ = tmp[i];
	while (size-->0)
		*str++ = ' ';
	return str;
}




// % [flags] [width] [.prec] [|h |l｜Ｌ] [type]
//   flags -- c, s, o(八进制), p(pointer), x(0x), X(0X), d(十进制)
int mix_vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	int i;
	char * str;
	char *s;
	int *ip;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
						number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str = buf; *fmt; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
	repeat:
		++fmt;		/* this also skips first '%' */
		switch (*fmt) {
		case '-': flags |= LEFT; goto repeat;
		case '+': flags |= PLUS; goto repeat;
		case ' ': flags |= SPACE; goto repeat;
		case '#': flags |= SPECIAL; goto repeat;
		case '0': flags |= ZEROPAD; goto repeat;
		}

		/* get field width */
		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			/* it's the next argument */
			++fmt;
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				/* it's the next argument */
				++fmt;
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char)va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;

		case 's':
			s = va_arg(args, char *);
			len = strlen(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			break;

		case 'o':
			str = number(str, va_arg(args, unsigned long), 8,
				field_width, precision, flags);
			break;

		case 'p':
			if (field_width == -1) {
				field_width = 8;
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long)va_arg(args, void *), 16,
				field_width, precision, flags);
			break;

		case 'x':
			flags |= SMALL;
		case 'X':
			str = number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			str = number(str, va_arg(args, unsigned long), 10,
				field_width, precision, flags);
			break;

		case 'f':
			flags |= SIGN;
			str = float_number(str, va_arg(args, double),
				               field_width, precision, flags);
			break;

		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;

		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}
	*str = '\0';
	return str - buf;
}



// 
int M_printf(const char *fmt, ...)
{
	char buf[128];
	va_list args;
	int i;

	va_start(args, fmt);
	i = mix_vsprintf(buf, fmt, args);
	va_end(args);
	console_print(buf, 2);
	return i;
}