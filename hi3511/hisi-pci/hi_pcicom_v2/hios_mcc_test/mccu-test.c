#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include "hi_mcc_usrdev.h"

#define DDAL_RUN_TEST(v, cond) do{ \
                printf("^[[1;33m%s: " #v #cond "^[[0;39m\n", \
                                ((v)cond) ? "^[[1;32m[ OK  ]" : "^[[1;31m[FAILD]"); \
                fflush(stdout); \
                usleep(10000); \
        }while(0)
#define DATA_SIZE  100
#define MAX_PORT  1 
int main(int argc, char *argv[])
{
	struct hi_mcc_handle_attr attr;
	void *buf;
	int fd[MAX_PORT];
	int i = 0;
	int ret;
	buf = malloc(DATA_SIZE);
	attr.target_id = 1;
	attr.port = 0;
	attr.priority = 1;
	for(i = 0; i < MAX_PORT; i++){
	        fd[i] = open("/dev/himedia/mcc_userdev", O_RDWR);
		attr.port = i;
		ioctl(fd[i], HI_MCC_IOC_CONNECT, &attr);
	}
	for(i = 0; i < MAX_PORT; i++){
		ret = write(fd[i], buf, DATA_SIZE);
		if(ret < 0)
			break;
	}
	for(i = 0; i < MAX_PORT; i++){
		ioctl(fd[i], HI_MCC_IOC_DISCONNECT, &attr);
		close(fd[i]);
	}

	free(buf);
	return 0;
}
