#minix filesystem
  CC = gcc
  CFLAGS = -std=c99 -g 
  INCLUDE_LIB = -lm

  objects = bitmap.o bit_map.o block_dev.o buffer.o char_dev.o cmd.o dev_init.o fcntl.o \
            file_dev.o file_table.o inode.o ioctl.o kenel.o keybord.o ll_rw_blk.o\
            mix_error.o M_printf.o namei.o noodle_printer.o open.o pipe.o read_write.o \
            stat.o super.o sys_calls.o test.o tool.o truncate.o tty_io.o vim.o
  
  target: $(objects)
	$(CC) $(CFLAGS) -o target $(objects) $(INCLUDE_LIB)
  bitmap.o: file_sys.h kernel.h
  bit_map.o: kernel.h
  block_dev.o: file_sys.h mix_erro.h
  buffer.o: file_sys.h kernel.h
  char_dev.o:file_sys.h kernel.h mix_erro.h
  cmd.o: cmd.h mix_sys.h mix_window.h mix_erro.h mix_type.h fcntl.h kernel.h
  dev_init.o: file_sys.h kernel.h stat.h fcntl.h mix_erro.h
  fcntl.o: file_sys.h fcntl.h mix_erro.h kernel.h
  file_dev.o: file_sys.h kernel.h mix_erro.h stat.h fcntl.h
  file_table.o: file_sys.h
  inode.o: file_sys.h kernel.h stat.h
  ioctl.o: file_sys.h
  kenel.o: kernel.h mix_window.h
  keybord.o: mix_window.h
  ll_rw_blk.o: file_sys.h kernel.h stat.h
  mix_error.o: mix_erro.h mix_window.h
  M_printf.o:  mix_window.h
  namei.o: file_sys.h fcntl.h stat.h mix_erro.h kernel.h mix_const.h
  noodle_printer.o: mix_window.h file_sys.h fcntl.h stat.h kernel.h
  open.o: file_sys.h kernel.h mix_erro.h utime.h fcntl.h stat.h
  read_write.o: file_sys.h kernel.h mix_erro.h stat.h fcntl.h 
  stat.o: file_sys.h stat.h tool.h mix_erro.h kernel.h 
  super.o: file_sys.h kernel.h mix_type.h mix_erro.h stat.h
  sys_calls.o: mix_sys.h cmd.h stat.h file_sys.h kernel.h mix_erro.h
  test.o: cmd.h file_sys.h mix_window.h
  tool.o: tool.h tool.c 
  truncate.o: file_sys.h kernel.h stat.h
  tty_io.o: mix_window.h file_sys.h kernel.h fcntl.h stat.h 
  vim.o: mix_window.h file_sys.h kernel.h fcntl.h stat.h mix_erro.h
  
  .PHONE: clean
  clean:
	-rm target $(objects)   
