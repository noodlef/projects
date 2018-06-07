#pragma once

typedef int(*fn_ptr) ();

extern int sys_set_tty_color();
extern int sys_set_char_color();
extern int sys_clear();
extern int sys_mount_cmd();
extern int sys_umount_cmd();
extern int sys_ls_cmd();

extern int sys_link_cmd();
extern int sys_symlink_cmd();
extern int sys_unlink_cmd();
extern int sys_rmdir_cmd();
extern int sys_mkdir_cmd();
extern int sys_rfile_cmd();
extern int sys_blkfile_cmd();
extern int sys_creat_cmd();
extern int sys_print_inode();
extern int sys_vim_cmd();
extern int sys_format_cmd();
extern int sys_quit_cmd();
extern int sys_cd_cmd();
extern int sys_useradd_cmd();
extern int sys_printer_cmd();
extern int sys_readsuper_cmd();


extern fn_ptr sys_call_table[];
extern int NR_syscalls;
