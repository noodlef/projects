#include"cmd.h"
#include"file_sys.h"
#include"mix_window.h"

int main()
{
	tty_init();
	fs_init();
	mix_cmd();
	//test_tty();
	//make_root();
	//system("pause");
	return 0;
}
