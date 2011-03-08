#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <asm/sizes.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>

int open_serial(const char *dev_name)
{
	int fd;
	fd = open(dev_name, O_RDWR | O_NONBLOCK);
	if (-1 == fd){ 
		perror("");
		exit(1);
	}

	return fd;
}

int set_speed(int fd)
{
	int ret = 0;
	struct  termios opt;

	tcgetattr(fd, &opt);
	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&opt,B4800);
	cfsetospeed(&opt,B4800);
	ret = tcsetattr(fd,TCSANOW,&opt);
	if(ret <0) {
		perror("set speed error.\n");
	}
	tcflush(fd,TCIOFLUSH);

	return ret;
}
															
int set_serial_rowmode(int fd)
{
	int ret = 0;
	struct termios options; 

	tcgetattr(fd, &options);

	tcflush(fd, TCIOFLUSH);
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

//	options.c_cflag &= ~PARENB;
	options.c_cflag |= PARENB;
	options.c_cflag |= PARODD;
	
//	options.c_cflag |= CSTOPB;
	options.c_cflag &= ~CSTOPB;
	options.c_oflag  &= ~OPOST;

	cfsetispeed(&options,B19200);
	cfsetospeed(&options,B19200);
	
	tcflush(fd,TCIFLUSH); 
	options.c_cc[VTIME] = 128;
	options.c_cc[VMIN] = 1;

	cfmakeraw(&options);

	ret = tcsetattr(fd,TCSANOW,&options);
	if (ret < 0) { 
		perror("setattr error. \n"); 
	} 

	return ret;
}

static int write_to_file(const char *fn, const void *buf, int len)
{
	FILE *f;

  	f = fopen(fn, "w+b");
	if(f == NULL) {
		perror("can't open written file.\n");
	}

	if(fwrite(buf,1,len,f) != len) {
		perror("");
	}

	fclose(f);

	printf("%d bytes write to file.\n", len);

	return 0;
}

static volatile int stat = 1;
static void exit_signal(int signr)
{
	stat = 0;
}

//#define MAX_BUFF_SIZE SZ_1M*8
#define MAX_BUFF_SIZE 512*5
int main(int argc, char *argv[])
{
	char *buf = NULL;
	int fd = -1;
	int wr_pos = 0;
	int rd_pos = 0;
	char w_buf[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	fd = open_serial("/dev/tts/ttyGPIO0");
	set_speed(fd);
	set_serial_rowmode(fd);

	buf = malloc(MAX_BUFF_SIZE);
	
	signal(2, exit_signal);
	while(stat && rd_pos<MAX_BUFF_SIZE) {
        	if(read(fd, buf+rd_pos, 1) != 1)
			continue;
//		else
//			write(fd, &buf[wr_pos+rd_pos], 1);
		rd_pos++;
//		sleep(1);
//		printf("read char num:%d,char:%c\n",rd_pos, *(buf+rd_pos-1));
	}
	printf("begin to close fd. \n");
	close(fd);
	
	printf("read char num:%d\n",rd_pos);
	int k;
	for(k=0;k<rd_pos;k++){
		printf("%c",*(buf+k));
	}
//	printf("\nprintf char num:%d\n", k);

//	write_to_file("0326.out", buf, rd_pos);

	free(buf);

	return 0;
}
