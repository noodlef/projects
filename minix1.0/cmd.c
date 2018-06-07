#include"cmd.h"
#include"string.h"
#include"math.h"
#include"mix_sys.h"
#include"mix_window.h"
#include"mix_erro.h"
#define KEY_SPACE 32
#define KEY_CMD_END   45
#define KEY_COMMA     44
// 命令表		
struct cmd_table cmd[CMD_COUNT] = 
{
	{
		"setcolor",                         // name
		0,                                  // call_index
		2,                                  // n_args 
	   { MIX_STRING, MIX_STRING }           // arg_type
    },

	{
		"ttycolor",                         // name
		1,                                  // call_index
		1,                                  // n_args 
	   { MIX_STRING}                        // arg_type
    },

   {
	"clear",                               // name
		2,                                 // call_index
		0,                                 // n_args 
		{ 0 }                              // arg_type
   },
   {
	   "mount",                           // name
	   3,                                 // call_index
	   2,                                 // n_args 
	   { MIX_STRING, MIX_STRING }         // arg_type
   },

   {
	   "umount",                          // name
	   4,                                 // call_index
	   1,                                 // n_args 
	   { MIX_STRING }                     // arg_type
   },

   {
	   "ls",                              // name
	   5,                                 // call_index
	   1,                                 // n_args 
	   { MIX_STRING}                      // arg_type
   },

   {
	   "link",                            // name
	   6,                                 // call_index
	   2,                                 // n_args 
	   { MIX_STRING, MIX_STRING }         // arg_type
   },

   {
	   "mlink",                           // name
	   7,                                 // call_index
	   2,                                 // n_args 
	  { MIX_STRING, MIX_STRING }          // arg_type
   },

   {
	   "unlink",                          // name
	   8,                                 // call_index
	   1,                                 // n_args 
	   { MIX_STRING }                     // arg_type
   },

   {
	   "rmdir",                           // name
	   9,                                 // call_index
	   1,                                 // n_args 
	   { MIX_STRING }                     // arg_type
   },

   {
	   "mkdir",                            // name
	   10,                                 // call_index
	   1,                                  // n_args 
	   { MIX_STRING }                      // arg_type
   },

   {
	   "rfile",                             // name
	   11,                                  // call_index
	   1,                                   // n_args 
	   { MIX_STRING }                       // arg_type
   },

   {
	   "blkfile",                           // name
	   12,                                  // call_index
	   2,                                   // n_args 
	   { MIX_STRING, MIX_INT }              // arg_type
   },

   {
	   "creat",                             // name
	   13,                                  // call_index
	   1,                                   // n_args 
	   { MIX_STRING }                       // arg_type
   },

   {
	   "printinode",                        // name
	   14,                                  // call_index
	   1,                                   // n_args 
	   { MIX_STRING }                       // arg_type
   },
};




char MIX_STACK[STACK_SIZE];             // 堆栈											   
int S_TOP = 0;                          // 栈底， 栈顶
int S_BUTTON = 0;
// 命令格式 ： command - arg1, arg2, arg3, ...


// 提取命令 -- command
// return : 返回cmd数组中， 命令所占的字节数
// 成功返回0 ， 失败返回 -1
int get_cmd(char* cmd, int buf, int* pos, int* count)
{
	int i = *pos;
	*count = 0;
	if (*pos > buf || *pos < 0)
		return -1;
	// 找出第一个不为空格的字符
	while (i < buf) {
		if (cmd[i] == KEY_SPACE) {
			++i;
			++*pos;
			continue;
		}
		break;
	}
	if (i >= buf) {
		//mix_cerr(3, "cmd is empty");
		return -1;
	}
	++i;
	++*count;
	while (i < buf) {
		if (cmd[i] != KEY_SPACE && cmd[i] != KEY_CMD_END) {
			++*count;
			++i;
			continue;
		}
		break;
	}
	return 0;
}

// 提取 命令的参数
// para : pos -- 开始查找的位置， count -- 参数所占的字节数
// return ： 0 -- 成功， -1 -- 失败
// pos 指向参数的起始位置
int get_arg(char* cmd, int buf, int* pos, int* count)
{
	int i = buf - *pos;
	int j;
	char c;
	*count = 0;
	if (*pos > buf || *pos < 0)
		return -1;
	while (i-- > 0) {
		if (cmd[*pos] == KEY_SPACE || cmd[*pos] == KEY_CMD_END 
	      || cmd[*pos] == KEY_COMMA){
			++*pos;
			continue;
		}
		break;
	}
	// 找不到起始字符
	if (i < 0)
		return -1;
	++*count;
	j = *pos + 1;
	while (i-- > 0) {
		c = cmd[j];
		if (cmd[j] != KEY_SPACE && cmd[j] != KEY_COMMA) {
			++*count;
			++j;
			continue;
		}
		break;
	}
	return 0;
}


// 将字符转化为对应数字值
static char char_to_value(char c)
{
	return c - 48;
}
// 根据命令的名字找到相应的命令结构指针
struct cmd_table* find_cmd_table(char *buf, int len)
{
	struct cmd_table* c = cmd;
	int count = CMD_COUNT;
	if (len <= 0 )
		return 0;
	while (count-- > 0) {
		if ((strlen(c->cmd) == len) && !strncmp(c->cmd, buf, len))
			return c;
		c++;
	}
	return 0;
}

// 压栈， 出栈
// 0 - 成功， -1 -- 失败
int mix_s_push(enum ARG_TYPE type)
{
	switch (type) {
		// char string
	case MIX_STRING:
	case MIX_CHAR:
		if ((S_TOP + sizeof(char)) > STACK_SIZE) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP += sizeof(char);
		break;
		// short
	case MIX_U_SHORT:
	case MIX_SHORT:
		if ((S_TOP + sizeof(short)) > STACK_SIZE) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP += sizeof(short);
		break;
		// long
	case MIX_U_LONG:
	case MIX_LONG:
		if ((S_TOP + sizeof(long)) > STACK_SIZE) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP += sizeof(long);
		break;
		// int
	case MIX_U_INT:
	case MIX_INT:
		if ((S_TOP + sizeof(int)) > STACK_SIZE) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP += sizeof(int);
		break;
		// double
	case MIX_DOUBLE:
		if ((S_TOP + sizeof(double)) > STACK_SIZE) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP += sizeof(double);
		break;
		// float
	case MIX_FLOAT:
		if ((S_TOP + sizeof(float)) > STACK_SIZE) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP += sizeof(float);
		break;
	default:
		mix_cerr("unknown arg type!");
		return -1;
	}
	return 0;
}

//
int mix_s_pop(enum ARG_TYPE type)
{
	switch (type) {
		// char string
	case MIX_STRING:
	case MIX_CHAR:
		if ((sizeof(char) + S_BUTTON) > S_TOP) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP -= sizeof(char);
		break;
		// short
	case MIX_U_SHORT:
	case MIX_SHORT:
		if ((sizeof(short) + S_BUTTON) > S_TOP) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP -= sizeof(short);
		break;
		// long
	case MIX_U_LONG:
	case MIX_LONG:
		if ((sizeof(long) + S_BUTTON) > S_TOP) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP -= sizeof(long);
		break;
		// int
	case MIX_U_INT:
	case MIX_INT:
		if ((sizeof(int) + S_BUTTON) > S_TOP) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP -= sizeof(int);
		break;
		// double
	case MIX_DOUBLE:
		if ((sizeof(double) + S_BUTTON) > S_TOP) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP -= sizeof(double);
		break;
		// float
	case MIX_FLOAT:
		if ((sizeof(float) + S_BUTTON) > S_TOP) {
			mix_cerr("stack overflow!");
			return -1;
		}
		S_TOP -= sizeof(float);
		break;
	default:
		mix_cerr("unknown arg type!");
		return -1;
	}
	return 0;
}

// 检查命令的参数是否正确
int check_para(char* buf, int len, enum ARG_TYPE type)
{
	int i, j, k;
	if (len <= 0)
		return -1;
	switch (type) {
		// char string
	case MIX_STRING:
	case MIX_CHAR:
		return 1;
		// short , int, long
	case MIX_U_SHORT:
	case MIX_SHORT:
	case MIX_U_INT:
	case MIX_INT:
	case MIX_U_LONG:
	case MIX_LONG:
		i = 0;
		if (buf[0] == '+' || buf[0] == '-') {
			i++;
			len--;
		}
		if (!len)
			return -1;
		// '0' - 48, '9' - 57
		while (len-- > 0) {
			if (buf[i] < 48 || buf[i] > 57)
				return -1;
			i++;
		}
		break;
		// double , float
	case MIX_FLOAT:
	case MIX_DOUBLE:
		i = 0;
		if (buf[0] == '+' || buf[0] == '-') {
			i++;
			len--;
		}
		if (!len)
			return -1;
		// 找小数点
		j = i;
		k = len;
		for (; k-- > 0 && buf[j] != '.'; j++)
			;
		// 没有小数点
		if (j >= (i + len)) {
			// '0' - 48, '9' - 57
			while (len-- > 0) {
				if (buf[i] < 48 || buf[i] > 57)
					return -1;
				i++;
			}
		}
		// 有小数点
		else {
			k = i;
			if (len <= 1)
				return -1;
			while (i < j) {
				if (buf[i] < 48 || buf[i] > 57)
					return -1;
				i++;
			}
			i++;
			while (i < (k + len)) {
				if (buf[i] < 48 || buf[i] > 57)
					return -1;
				i++;
			}
		}
		break;
	default:
		return -1;
	}
	return 1;
}

// flag = 1, 成功， flag = -1 失败
int get_int_para(char* buf, int len, int* flag)
{
	int ret = 0;
	int c = 1;
	int i = 0;
	*flag = -1;
	if (check_para(buf, len, MIX_INT) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			c = -1;
		i++;
		len--;
	}
	while (len-- > 0)
		ret += char_to_value(buf[i++]) * pow(10, len);
	*flag = 1;
	ret = ret * c;
	return ret;
}

// flag = 1, 成功， flag = -1 失败
unsigned int get_uint_para(char* buf, int len, int* flag)
{
	unsigned int ret = 0;
	int i = 0;
	*flag = -1;
	if (check_para(buf, len, MIX_U_INT) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			return 0;
		i++;
		len--;
	}
	while (len-- > 0)
		ret += char_to_value(buf[i++]) * pow(10, len);
	*flag = 1;
	return ret;
}

// flag = 1, 成功， flag = -1 失败
short get_short_para(char* buf, int len, int* flag)
{
	short ret = 0;
	short c = 1;
	int i = 0;
	*flag = -1;
	if (check_para(buf, len, MIX_SHORT) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			c = -1;
		i++;
		len--;
	}
	while (len-- > 0)
		ret += char_to_value(buf[i++]) * pow(10, len);
	*flag = 1;
	ret = ret * c;
	return ret;
}

// flag = 1, 成功， flag = -1 失败
unsigned short get_ushort_para(char* buf, int len, int* flag)
{
	unsigned short ret = 0;
	int i = 0;
	*flag = -1;
	if (check_para(buf, len, MIX_U_SHORT) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			return 0;
		i++;
		len--;
	}
	while (len-- > 0)
		ret += char_to_value(buf[i++]) * pow(10, len);
	*flag = 1;
	return ret;
}

// flag = 1, 成功， flag = -1 失败
long get_long_para(char* buf, int len, int* flag)
{
	long ret = 0;
	long c = 1;
	int i = 0;
	*flag = -1;
	if (check_para(buf, len, MIX_LONG) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			c = -1;
		i++;
		len--;
	}
	while (len-- > 0)
		ret += char_to_value(buf[i++]) * pow(10, len);
	*flag = 1;
	ret = ret * c;
	return ret;
}

// flag = 1, 成功， flag = -1 失败
unsigned long get_ulong_para(char* buf, int len, int* flag)
{
	unsigned long ret = 0;
	int i = 0;
	*flag = -1;
	if (check_para(buf, len, MIX_U_LONG) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			return 0;
		i++;
		len--;
	}
	while (len-- > 0)
		ret += char_to_value(buf[i++]) * pow(10, len);
	*flag = 1;
	return ret;
}


// flag = 1, 成功， flag = -1 失败
float get_float_para(char* buf, int len, int* flag)
{
	float ret = 0;
	float c = 1;
	int i = 0, j, k;
	*flag = -1;
	if (check_para(buf, len, MIX_FLOAT) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			c = -1;
		i++;
		len--;
	}
	// 找小数点
	j = i;
	k = len;
	for (; k-- > 0 && buf[j] != '.'; j++)
		;
	if (j >= (i + len)) {
		while (len-- > 0)
			ret += char_to_value(buf[i++]) * pow(10, len);
	}
	else {
		k = len;
		len = j - i;
		j = i;
		while (len-- > 0)
			ret += char_to_value(buf[i++]) * pow(10, len);
		i++;
		len = k - (i - j);
		j = -1;
		while (len-- > 0)
			ret += char_to_value(buf[i++]) * pow(10, j--);
	}
	*flag = 1;
	ret = ret * c;
	return ret;
}

// flag = 1, 成功， flag = -1 失败
double get_double_para(char* buf, int len, int* flag)
{
	double ret = 0;
	double c = 1;
	int i = 0, j, k;
	*flag = -1;
	if (check_para(buf, len, MIX_DOUBLE) < 0)
		return 0;
	if (buf[0] == '-' || buf[0] == '+') {
		if (buf[0] == '-')
			c = -1;
		i++;
		len--;
	}
	// 找小数点
	j = i;
	k = len;
	for (; k-- > 0 && buf[j] != '.'; j++)
		;
	if (j >= (i + len)) {
		while (len-- > 0)
			ret += char_to_value(buf[i++]) * pow(10, len);
	}
	else {
		k = len;
		len = j - i;
		j = i;
		while (len-- > 0)
			ret += char_to_value(buf[i++]) * pow(10, len);
		i++;
		len = k - (i - j);
		j = -1;
		while (len-- > 0)
			ret += char_to_value(buf[i++]) * pow(10, j--);
	}
	*flag = 1;
	ret = ret * c;
	return ret;
}

// 返回字符
char get_char_para(char* buf, int len, int* flag)
{
	*flag = -1;
	if (len > 1 || check_para(buf, len, MIX_CHAR) < 0)
		return 0;
	*flag = 1;
	return buf[0];
}

// 返回字符串的长度
int get_string_para(char* buf, int len, int* flag)
{
	*flag = -1;
	if (check_para(buf, len, MIX_STRING) < 0)
		return 0;
	*flag = 1;
	return len;
}

// 将参数压栈
// 成功返回 1， 失败返回 -1
int push_stack(char* buf, int count, enum ARG_TYPE type)
{
	int flag;
	short s_short;
	unsigned s_ushort;
	int s_int;
	unsigned s_uint;
	long s_long;
	unsigned s_ulong;
	float s_float;
	double s_double;
	char s_char;
	int s_string;

	switch (type) {
	case MIX_SHORT:
		s_short = get_short_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_SHORT) < 0)
			return -1;
		*((short*)(MIX_STACK + S_TOP - sizeof(short))) = s_short;
		break;
	case MIX_U_SHORT:
		s_ushort = get_ushort_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_U_SHORT) < 0)
			return -1;
		*((unsigned short*)(MIX_STACK + S_TOP - sizeof(unsigned short))) = s_ushort;
		break;
	case MIX_INT:
		s_int = get_int_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_INT) < 0)
			return -1;
		*((int*)(MIX_STACK + S_TOP - sizeof(int))) = s_int;
		break;
	case MIX_U_INT:
		s_uint = get_uint_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_U_INT) < 0)
			return -1;
		*((unsigned int*)(MIX_STACK + S_TOP - sizeof(unsigned int))) = s_uint;
		break;
	case MIX_LONG:
		s_long = get_long_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_LONG) < 0)
			return -1;
		*((long*)(MIX_STACK + S_TOP - sizeof(long))) = s_long;
		break;
	case MIX_U_LONG:
		s_ulong = get_ulong_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_U_LONG) < 0)
			return -1;
		*((unsigned long*)(MIX_STACK + S_TOP - sizeof(unsigned long))) = s_ulong;
		break;
	case MIX_FLOAT:
		s_float = get_float_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_FLOAT) < 0)
			return -1;
		*((float*)(MIX_STACK + S_TOP - sizeof(float))) = s_float;
		break;
	case MIX_DOUBLE:
		s_double = get_double_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_DOUBLE) < 0)
			return -1;
		*((double*)(MIX_STACK + S_TOP - sizeof(double))) = s_double;
		break;
	case MIX_CHAR:
		s_char = get_char_para(buf, count, &flag);
		if (flag < 0)
			return -1;
		// 参数压栈
		if (mix_s_push(MIX_CHAR) < 0)
			return -1;
		*((char*)(MIX_STACK + S_TOP - sizeof(char))) = s_char;
		break;
		// 如果是字符串，就在末尾添加一个空字符 '\0'
	case MIX_STRING:
		// 返回字符串长度
		s_string = get_string_para(buf, count, &flag);
		if (flag < 0)
			return -1; 
		s_string += 2;
		// 对于字符串，是将每个字符分别进行压栈
		flag = s_string;
		while (flag-- >0)
			if (mix_s_push(MIX_CHAR) < 0)
				break;
		if (flag > 0) {
			// 恢复堆栈
			S_TOP -= (s_string - flag - 1);
			return -1;
		}
		*((char*)(MIX_STACK + S_TOP - s_string)) = '\0';
		--s_string;
		flag = 0;
		while (s_string > 1)
			*((char*)(MIX_STACK + S_TOP - s_string--)) = buf[flag++];
		*((char*)(MIX_STACK + S_TOP - s_string)) = '\0';
		break;
	default:
		return -1;
	}
	return 1;
}


// 
int mix_cmd()
{
	int error_code;
	int i, pos, count;
	char buf[1000];
	struct cmd_table* cmd;
	int cmd_args, k_type, stack_pos;
	while (1)
	{
		if ((i = tty_read(0, buf, 1000)) <= 0) {
			mix_cerr("tty_read : read error!");
			continue;
		}
		pos = 0;
		// 获得命令， i 为读到的字节数， 最后一个字节为 enter 的键值
		if (get_cmd(buf, i - 1, &pos, &count) < 0) {
			mix_cerr("command is empty");
			continue;
		}
		// 查找命令表 cmd_table
		if (!(cmd = find_cmd_table(buf + pos, count))) {
			mix_cerr("invalid command!");
			continue;
		}
		// 如果该命令无参数
		if (!(cmd_args = cmd->n_args)) {
			error_code = sys_call_table[cmd->sys_call_index]();
			err(0, error_code);
			continue;
		}
		// 获取命令的 各个参数 parameters
		k_type = 0;
		stack_pos = S_TOP;
		for (; cmd_args > 0; cmd_args--) {
			// 获取参数
			pos += count;
			if (get_arg(buf, i - 1, &pos, &count) < 0)
				break;
			// 参数压进堆栈
			if (push_stack(buf + pos, count, cmd->args_type[k_type++]) < 0)
				break;
		}
		if (cmd_args > 0) {
			// 恢复堆栈
			S_TOP = stack_pos;
			mix_cerr("lack of arguments in command line!");
			continue;
		}
		// 如果给的参数个数多于该系统调用的参数个数
		// 此处使用get_cmd 仅仅是为了确定在pos位置后还有没字符
		pos += count;
		if ( pos < (i - 1) ){
			get_cmd(buf, i - 1, &pos, &count);
			// 恢复堆栈
			if (count > 0) {
				S_TOP = stack_pos;
				mix_cerr("Too many arguments in command line!");
				continue;
			}
		}
		error_code = sys_call_table[cmd->sys_call_index]();
		err(0, error_code);
	}
}


