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


int main(int argc, char *argv[])
{
	int fd;
	char buf[1024]={3,5,7,9,11};
	fd = open("/dev/gpiokey",O_RDONLY);
	if (fd < 0)
		{
			perror("open device leds");
			exit(1);
		}
	//printf("%d\n",*buf);

	while (1)
	{
		read(fd,buf,1);
		//printf("111111");
		//usleep();
		printf("The user key is %d\n",*buf);	
	}
	close(fd);
	return 0;
}
