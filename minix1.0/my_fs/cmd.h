#include"mix_window.h"
#define CMD_COUNT 100            // 总命令的个数
#define STACK_SIZE 1000          // 堆栈大小
// 参数类型
enum ARG_TYPE{
	MIX_CHAR = 1,
	MIX_STRING = 2,

	MIX_SHORT = 3,
	MIX_U_SHORT = 4,

	MIX_INT = 5,
	MIX_U_INT = 6,

	MIX_LONG = 7,
	MIX_U_LONG = 8,

	MIX_FLOAT = 9,

	MIX_DOUBLE = 10
};

// 命令结构
struct cmd_table{
	char cmd[20];                              // 命令
	int sys_call_index;                        // 该命令对应的系统调用在系统调用表的下标
	int n_args;                                // 该命令包含参数个数
	enum ARG_TYPE args_type[20];               // 对应参数类型
};

extern struct cmd_table cmd[CMD_COUNT];        // 命令表											   
extern char MIX_STACK[STACK_SIZE];             // 堆栈											   
extern int S_TOP ;                             // 栈底， 栈顶
extern int S_BUTTON ;




int mix_cmd();
int get_cmd(char* cmd, int buf, int* pos, int* count);
int get_arg(char* cmd, int buf, int* pos, int* count);
struct cmd_table* find_cmd_table(char *buf, int len);
int mix_s_push(enum ARG_TYPE type);
int mix_s_pop(enum ARG_TYPE type);
int check_para(char* buf, int len, enum ARG_TYPE type);
int get_int_para(char* buf, int len, int* flag);
unsigned int get_uint_para(char* buf, int len, int* flag);
short get_short_para(char* buf, int len, int* flag);
unsigned short get_ushort_para(char* buf, int len, int* flag);
long get_long_para(char* buf, int len, int* flag);
unsigned long get_ulong_para(char* buf, int len, int* flag);
float get_float_para(char* buf, int len, int* flag);
double get_double_para(char* buf, int len, int* flag);
char get_char_para(char* buf, int len, int* flag);
int get_string_para(char* buf, int len, int* flag);






















