include ../Makefile.param
CFLAGS := -Wall -g $(INC_PATH)

SRCS    :=  $(wildcard *.c)
OBJS    :=  $(SRCS:%.c=%.o)

#TARGET := sample_misc
TARGET := $(OBJS:%.o=%)
.PHONY : clean all

all: $(TARGET)
$(TARGET):%:%.o
	$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ $(LIBS) 

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)

