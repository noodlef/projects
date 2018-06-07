
  objects = main.o tool.o
  libs = -lm
  cc = gcc
  CFLAGS = -g
  target : $(objects)
	$(cc) $(CFLAGS) -o target $(objects) #$(libs)
  main.o : main.c tool.h
  tool.o : tool.c tool.h

  .PHONY : clean
  clean :
	-rm target $(objects)
