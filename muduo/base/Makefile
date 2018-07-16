  # 编译需要连接到动态或静态库
  LIBS = -lpthread  # `mysql_config --libs --cflags` -lmysqlclient
  # 存取库文件相关头文件的路径
  HEAD_PATH = -I/usr/include/
  # 库文件存放的路径
  LIBS_PATH = -L/usr/lib64/
  CC = g++
  # 编译附加选项
  CFLAGS = -g -std=gnu++11

  TARGET : target
	echo -e "\033[31m target is successfully compiled! \033[0m"

  # 目标文件
OBJECTS = muduo_mutex.o muduo_condition.o muduo_latch.o muduo_thread.o exception.o \
		  thread_pool.o time_stamp.o process_info.o file_util.o date.o log_file.o  \
		  log_stream.o logger.o  test1.o async_logging.o log_test.o current_thread.o
			
  target : $(OBJECTS)
	$(CC) -o target $(OBJECTS) $(LIBS_PATH) $(LIBS)  

  muduo_mutex.o : muduo_mutex.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c muduo_mutex.cpp 
  
  muduo_condition.o : muduo_condition.h muduo_mutex.h
	$(CC) $(CFLAGS) $(HEAD_PATH) -c muduo_condition.cpp 
  
  muduo_latch.o : muduo_latch.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c muduo_latch.cpp 

  muduo_thread.o : muduo_thread.h current_thread.h exception.h #logger.h  
	$(CC) $(CFLAGS) $(HEAD_PATH) -c muduo_thread.cpp 

  exception.o : exception.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c exception.cpp 
  
  thread_pool.o : thread_pool.h exception.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c thread_pool.cpp 

  process_info.o : process_info.h current_thread.h file_util.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c process_info.cpp 
  
  time_stamp.o : time_stamp.h  
	$(CC) $(CFLAGS) $(HEAD_PATH) -c time_stamp.cpp 

  file_util.o : file_util.h #logger.h  
	$(CC) $(CFLAGS) $(HEAD_PATH) -c file_util.cpp 

  date.o : date.h  
	$(CC) $(CFLAGS) $(HEAD_PATH) -c date.cpp 

  log_file.o : log_file.h process_info.h file_util.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c log_file.cpp  
  
  log_stream.o : log_stream.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c log_stream.cpp 

  logger.o : logger.h current_thread.h time_stamp.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c logger.cpp 

  async_logging.o : async_logging.h log_file.h time_stamp.h async_logging.cpp 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c async_logging.cpp 

  test1.o : test1.cpp 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c test1.cpp 

  log_test.o : log_test.cpp 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c log_test.cpp 

 current_thread.o : current_thread.cpp current_thread.h 
	$(CC) $(CFLAGS) $(HEAD_PATH) -c current_thread.cpp 




  .PHONY : clean
  clean :
	rm -rf target *.o 
