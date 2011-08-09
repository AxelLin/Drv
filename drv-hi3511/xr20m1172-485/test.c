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
#include <linux/delay.h>
// 定义引脚输入输出方向
#define I2C_GPIO_INPUT                      0
#define I2C_GPIO_OUTPUT                     1
// 定义引脚高低电平
#define I2C_GPIO_PIN_LOW                    0
#define I2C_GPIO_PIN_HIGH                   1

#define	GPIODRV_IOCTL_BASE	'G'

#define GPIOIOC_SETDIRECTION    _IOW(GPIODRV_IOCTL_BASE, 0, int)
#define GPIOIOC_SETPINVALUE     _IOW(GPIODRV_IOCTL_BASE, 1, int)
#define GPIOIOC_SETTRIGGERMODE  _IOW(GPIODRV_IOCTL_BASE, 2, int)
#define GPIOIOC_SETTRIGGERIBE   _IOW(GPIODRV_IOCTL_BASE, 3, int)
#define GPIOIOC_SETTRIGGERIEV   _IOW(GPIODRV_IOCTL_BASE, 4, int)
#define GPIOIOC_SETIRQALL       _IOW(GPIODRV_IOCTL_BASE, 5, int)
#define GPIOIOC_SETCALLBACK     _IOW(GPIODRV_IOCTL_BASE, 6, int)
#define GPIOIOC_ENABLEIRQ       _IO(GPIODRV_IOCTL_BASE,  7)
#define GPIOIOC_DISABLEIRQ      _IO(GPIODRV_IOCTL_BASE,  8)
#define GPIOIOC_GETPININFO      _IOR(GPIODRV_IOCTL_BASE, 9, GPIO_INFO_T)
#define GPIOIOC_GETCALLBACK     _IOR(GPIODRV_IOCTL_BASE, 10, int)
#define GPIOIOC_GETPINVALUE           _IOR(GPIODRV_IOCTL_BASE, 11, int)
#define GPIOIOC_SETBAUDERATE	_IOR(GPIODRV_IOCTL_BASE, 12, int)

int main(int argc,char **argv)
{
	int fd;
	int value;
	unsigned char buf[100] ="abcdefghijklmnopqrstuvwxyz";
	unsigned char buf1[200];
	int i=0;
	printf("test for i2c_485 !");
	fd = open("/dev/i2c_485",O_RDWR);
	if(fd < 0)
		{
			perror("open device i2c_gpio failed 1");
			exit(1);

		}
	printf (" I am coming !\n");
	value = 9600;
	ioctl(fd,GPIOIOC_SETBAUDERATE,&value);
/*	while(1)
	{
	
	printf ("Now write the com the 1st time\n");	
	sleep(1);
	write(fd,buf,8);

	} 
*/
	while(1)
	{
		write(fd,buf,22);
		sleep(1);
		memset(buf1,'\0', 200);
		if(read(fd,buf1,100) > 0)
		{
		printf ("read  buf1 is %s\n",buf1);
		}
		else {
	//	printf("read no chars");
		}
		
	}
return 0;
	printf("open device i2c_gpio succeed !\n");
	ioctl(fd,GPIOIOC_SETPINVALUE,0x55);
	printf("i2c_gpio test !\n");
	while(1)
	{
	ioctl(fd,GPIOIOC_SETPINVALUE,i++);
	printf("test  i count  %02x\n",i);
	ioctl(fd,GPIOIOC_GETPINVALUE,&value);
	printf("test get value %02x\n",value);
	sleep(1);
	}


}
