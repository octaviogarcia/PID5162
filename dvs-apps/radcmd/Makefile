all: radcmd
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS = radcmd.o 
	
radcmd: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o radcmd $(OBJS)  -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm radcmd  *.o
