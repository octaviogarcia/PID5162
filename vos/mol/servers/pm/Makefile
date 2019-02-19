all: pm
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS = pm.o table.o utility.o getset.o forkexit.o \
	timers.o signal.o misc.c time.c rexec.o
	

LIBS = 	/usr/src/dvs/vos/mol/lib/syslib/libsyslib.so \
		/usr/src/dvs/vos/mol/lib/timers/libtimers.so \
		/usr/src/dvs/dvk-lib/stub_dvkcall.o 
	

pm: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o pm $(OBJS) $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm pm  *.o
