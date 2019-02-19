all: fs
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi
CFLAGS = -g -v -O0 -Wall -D_FILE_OFFSET_BITS=64

# OBJS = main.o table.o utility.o open.o read.o write.o mount.o path.o super.o link.o inode.o misc.o cache.o dmap.o device.o \
# 	../../stub_syscall.o

#Objetos a compilar
 OBJS = main.o table.o utility.o open.o read.o write.o filedes.o path.o protect.o \
		super.o link.o inode.o misc.o cache.o dmap.o device.o pipe.o stadir.o lock.o \
		time.o mount.o moldevcfg.o linux_util.o 

LIBS = 
LIBS = 	/usr/src/dvs/vos/mol/lib/syslib/libsyslib.so \
		/usr/src/dvs/vos/mol/lib/timers/libtimers.so \
		/usr/src/dvs/vos/mol/lib/mollib/libmollib.so \
		/usr/src/dvs/lib/config/libconfig.so \
		/usr/src/dvs/dvk-lib/stub_dvkcall.o 

fs: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o fs $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm fs  *.o
