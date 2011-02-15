#A simple Makefile for applications

#CC=gcc
CC=arm-linux-gcc

EXEC=key-app
OBJS=key-app.o

CFLAGS+=
LDFLAGS+=

all:$(EXEC)
$(EXEC):$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.PHONY:clean
clean:
	-rm -f $(EXEC) *.elf *.o *.gdb

