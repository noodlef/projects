  # ������Ҫ���ӵ���̬��̬��
  LIBS = -lpthread  # `mysql_config --libs --cflags` -lmysqlclient
  # ��ȡ���ļ����ͷ�ļ���·��
  HEAD_PATH = -I/usr/include/
  # ���ļ���ŵ�·��
  LIBS_PATH = -L/usr/lib64/
  CC = g++
  # ���븽��ѡ��
  CFLAGS = -g -std=gnu++11

  TARGET : target
	echo -e "\033[31m target is successfully compiled! \033[0m"

  # Ŀ���ļ�
  OBJECTS = muduo_mutex.o muduo_condition.o muduo_latch.o muduo_thread.o exception.o

  target : $(OBJECTS)
	#$(CC) -o target $(OBJECTS) $(LIBS_PATH) $(LIBS)  

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


  .PHONY : clean
  clean :
	rm -rf target *.o 