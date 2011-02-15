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

#define LIGHT_ON	1
#define LIGHT_OFF	0

int main(int argc, char *argv[])
{
	int fd;
	fd = open("/dev/gpio_led0",O_RDWR);
	if (fd < 0)
		{
			perror("open device leds");
			exit(1);
		}
	while (1)
	{
		ioctl(fd,LIGHT_ON);
		usleep(100000);
		ioctl(fd,LIGHT_OFF);
		usleep(100000);
		
	}
	close(fd);
	return 0;
}
