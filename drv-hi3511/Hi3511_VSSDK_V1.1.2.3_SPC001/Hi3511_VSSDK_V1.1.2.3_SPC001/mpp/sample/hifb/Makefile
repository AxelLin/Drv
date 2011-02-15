CROSS:=arm-hismall-linux-
CC=$(CROSS)gcc

INC_PATH:=-I../../include
CFLAGS:=-Wall $(INC_PATH)
LDFLAGS:= -lm -lc 

SRC = $(wildcard *.c)
DST = $(SRC:%.c=%)

all:$(DST)

$(DST): % : %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.o $(DST)
