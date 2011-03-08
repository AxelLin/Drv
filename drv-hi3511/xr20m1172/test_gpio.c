#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <linux/types.h>


int main(int argc,char **argv)
{
	int fd;
	unsigned char buf[] = {0x55};

	fd = open("/dev/i2c_gpio",O_RDWR);
	if(fd < 0)
		{
			perror("open device i2c_gpio failed 1");
			exit(1);

		}
	write(fd,buf,1);	
	printf("open device i2c_gpio succeed !");

}
