#include"mix_erro.h"
#include"mix_window.h"
// 定义全局变量 error , 用于控制台错误输出
int mix_error;
static int  aux_err(struct mix_char* buf, const char* src, int buf_size)
{
	int i;
	int count = strlen(src);
	for (i = 0; i < buf_size && i < count; i++) {
		buf[i].ch = src[i];
		buf[i].f_color = CFC_Red;
		buf[i].b_set = 0;
	}
	return i;
}
// 输出错误信息
void err(unsigned minor, int error)
{
	enum { err_size = 100 };
	struct mix_char buf[err_size];
	int count;
	error = -error;
	if (!error)
		return;
	switch (error) {
	case ERROR:
		count = aux_err(buf, "a generic error has occurred!", err_size);
		break;
	case WARGUMENT:
		count = aux_err(buf, "invalid argument!", err_size);
		break;
	case EPERM:
		count = aux_err(buf, "permission denied!", err_size);
		break;
	case ENOENT:
		count = aux_err(buf, "trying to access noexistent file or directory!", err_size);
		break;
	case EACCES:
		count = aux_err(buf, "no accessible permission!", err_size);
		break;
	case EEXIST:
		count = aux_err(buf, "the file already exists!", err_size);
		break;
	case EISDIR:
		count = aux_err(buf, "this is a directory!", err_size);
		break;
	case ENOSPC:
		count = aux_err(buf, "no space in device!", err_size);
		break;
	default:
		count = aux_err(buf, "an undefined error has occurred!", err_size);
		break;
	}
	mix_print(buf, count, 0);
	new_line(0);
}