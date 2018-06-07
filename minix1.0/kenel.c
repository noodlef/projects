#include"kernel.h"
#include"mix_window.h"


extern int console_print(char * buf, int minor);
extern int mix_vsprintf(char* buf, const char* fmt, va_list args);
extern int M_printf(const char* fmt, ...);
static char buf[128];

unsigned int current_time()
{
	time_t raw_time = time(0);
	struct tm current_date;
	localtime_s(&current_date, &raw_time);
	return raw_time;
}

// ��unix ʱ��ת��Ϊ��׼ʱ���ʽ���
void print_date(int t)
{
	time_t tm = t;
	struct tm tm_t = { 0 };
	localtime_s(&tm_t, &tm);
	char buf[100];
	strftime(buf, 100, "%F %T", &tm_t);
	M_printf("[%s]\n", buf);
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

// suser()�ж��Ƿ�Ϊ�����û�
// �ǣ����� 1 �� �񣬷���0
int suser()
{
	if (!current->euid)
		return 1;
	return 0;
}
struct task_struct process_noodle = {1,1};
struct task_struct* current = &process_noodle;