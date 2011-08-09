#A simple Makefile for applications

#CC=gcc
CC=arm-hismall-linux-gcc

EXEC=test
OBJS=test.o

CFLAGS+=
LDFLAGS+=

all:$(EXEC)
$(EXEC):$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	cp $(EXEC) /home/bamboo/work/hi3511/rootbox
.PHONY:clean
clean:
	-rm -f $(EXEC) *.elf *.o *.gdb

