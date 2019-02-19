all: stub_dvkcall.o libdvklib.so

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 
OBJS =  stub_dvkcall.o   

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

stub_dvkcall.o: stub_dvkcall.c ../include/com/stub_dvkcall.h 
	$(CC) -g -shared -pthread -o stub_dvkcall.o  stub_dvkcall.c

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

libdvklib.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libdvklib.so $(OBJS)  $(LIBS)
	
clean:
	rm  *.o *.so

.PHONY: all clean 
.SECONDARY:

