all: dvsd dvscmd
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS1 =  dvsd.o gcsd.o 
OBJS2 = dvscmd.o 
#./libspread/libspread-core.a 

LIBS = 	../lib/timers/libtimers.so \
		../stub_syscall.o \
		/usr/local/lib/libspread.a \
		/usr/local/lib/libspread-util.a
##	/usr/local/lib/libspread-core.a \

dvsd: $(OBJS1)   
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -o dvsd $(OBJS1) $(LIBS) -I$(INCLUDE)

dvscmd: $(OBJS2)   
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -o dvscmd $(OBJS2) $(LIBS) -I$(INCLUDE)
		
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm dvsd dvscmd  *.o
