CC:=arm-hismall-linux-gcc

INC_PATH:=-I../../include
LIBRARY_PATH:=../../lib.rel

LDFLAGS:= -L$(LIBRARY_PATH) -ltde -lm -lc
CFLAGS:=$(INC_PATH)

SRC = $(wildcard *.c)
DST = $(SRC:%.c=%)

all:$(DST)

$(DST): % : %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBRARY_PATH)/libmpi.a

clean:
	rm -f *.o $(DST)

