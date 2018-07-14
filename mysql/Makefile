
  objects = test.o DBPool.o config_file_reader.o util.o
  libs = -lpthread   `mysql_config --libs --cflags` -lmysqlclient
  HeaderPath = -I/usr/include/mysql 
  cc = g++
  CFLAGS = -g
  target : $(objects)
	$(cc) -g -o target $(objects) $(libs)  
  test.o : DBPool.h test.cpp 
	$(cc) $(CFLAGS) $(HeaderPath) -c test.cpp 
  DBPool.o : Thread.h config_file_reader.h DBPool.h DBPool.cpp
	$(cc) $(CFLAGS) $(HeaderPath) -c DBPool.cpp 
  config_file_reader.o : util.h config_file_reader.h config_file_reader.cpp
	$(cc) $(CFLAGS) -c config_file_reader.cpp 
  util.o : util.h util.cpp
	$(cc) $(CFLAGS) -c util.cpp
  .PHONY : clean
  clean :
	-rm target $(objects)
