#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include"kernel.h"
#include"mix_window.h"
#include<time.h>

extern int console_print(char * buf, int minor);
extern int mix_vsprintf(char* buf, const char* fmt, va_list args);
extern int M_printf(const char* fmt, ...);
static char buf[128];

unsigned int current_time()
{
	time_t raw_time = time(0);
	struct tm current_date;
	//localtime_r(&current_date, &raw_time);
	return raw_time;
}

// 将unix 时间转化为标准时间格式输出
void print_date(int t)
{
	time_t tm = t;
	struct tm tm_t = { 0 };
	localtime_r(&tm, &tm_t);
	char buf[100];
	//strftime(buf, 100, "%F %T", &tm_t);
	strftime(buf, 100, "%Y - %m - %d %H:%M : %S", &tm_t);
	M_printf("[%s]\n", buf);
}

int M_date(int t, char* buf, int len){
	time_t tm = t;
	struct tm tm_t = { 0 };
	localtime_r(&tm, &tm_t);
	return strftime(buf, len, "%Y - %m - %d %H:%M : %S", &tm_t);
}

int panic(const char* fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = mix_vsprintf(buf, fmt, args);
	va_end(args);
	console_print(buf, 3);
	return i;
}

//
int printk(const char* fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = mix_vsprintf(buf, fmt, args);
	va_end(args);
	console_print(buf, 3);
	return i;
}

// suser()判断是否为超级用户
// 是，返回 1 ， 否，返回0
int suser()
{
	if (!current->euid)
		return 1;
	return 0;
}
struct task_struct process_noodle = {1,1};
struct task_struct* current = &process_noodle;
