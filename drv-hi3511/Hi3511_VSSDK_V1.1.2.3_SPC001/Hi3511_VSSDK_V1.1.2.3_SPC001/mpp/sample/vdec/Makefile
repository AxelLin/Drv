include ../Makefile.param
CFLAGS := -Wall -g $(INC_PATH)

SRCS    :=  $(wildcard *.c)
OBJS    :=  $(SRCS:%.c=%.o)

TARGET := sample_vdec
.PHONY : clean all

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ $(LIBS) 

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)

