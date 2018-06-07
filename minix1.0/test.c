#include"cmd.h"
#include"file_sys.h"
#include"mix_window.h"


void test_tty();
int main()
{
	/*fs_init();
	sys_mkdir("/dev", d_mode);
	sys_mkdir("/home", d_mode);
	sys_mkdir("/sys", d_mode);
	sys_mknod("/dev/hd", d_mode | S_IFBLK, S_DEVICE(S_DISK, 1));
	sys_mount("/dev/hd", "/home", 0);
	sys_mknod("/home/test.c", d_mode, 0);
	ls("/home", 0);*/
	tty_init();
	fs_init();
	mix_cmd();
	test_tty();
	system("pause");
	return 0;
}