
  
  cc := g++ 
  CFLAGS := -g -W -pthread -lpthread

  objects   := config_file_reader.o echo_conn.o event_dispatch.o im_conn.o \
               im_pdu_base.o Lock.o netlib.o socket_base.o util.o util_pdu.o
  s_objects := server.o $(objects)
  c_objects := client.o $(objects)
 
  all : server client
	echo "compile succeed"

  server : $(s_objects)
	$(cc) $(CFLAGS) -o server $(s_objects)
  server.o : stdafx.h echo_conn.h netlib.h config_file_reader.h version.h
 
  client : $(c_objects)
	$(cc) $(CFLAGS) -o client $(c_objects) 
  client.o : stdafx.h echo_conn.h netlib.h config_file_reader.h version.h

  config_file_reader.o : stdafx.h config_file_reader.h
  echo_conn.o : stdafx.h echo_conn.h
  event_dispatch.o : event_dispatch.h Lock.h socket_base.h
  im_conn.o : im_conn.h
  im_pdu_base.o : im_pdu_base.h util.h
  Lock.o : Lock.h stdafx.h
  netlib.o : stdafx.h netlib.h socket_base.h event_dispatch.h
  socket_base.o : socket_base.h
  util.o : util.h
  util_pdu.o : util_pdu.h

  .PHONY: clean
  clean :
	rm -f server client *.o
