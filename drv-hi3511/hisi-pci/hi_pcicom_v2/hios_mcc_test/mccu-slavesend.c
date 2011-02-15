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

#define DATA_SIZE  400
#define MAX_PORT  50 

int main(int argc, char *argv[])
{
	struct hi_mcc_handle_attr attr;
	void *buf;
	int fd[MAX_PORT];
	int i = 0;
	int ret;
	fd_set fds;
	buf = malloc(DATA_SIZE);
	attr.target_id = 0;
	attr.port = 0;
	attr.priority = 1;

	for(i = 0; i < MAX_PORT; i++){
	        fd[i] = open("/dev/himedia/mcc_userdev", O_RDWR);
		attr.port = i;
		ioctl(fd[i], HI_MCC_IOC_CONNECT, &attr);
	}
	for(i = 0; i < MAX_PORT; i++){
		memset(buf, i, DATA_SIZE);
		ret = write(fd[i], buf, DATA_SIZE);
		if(ret < 0)
			break;
	        FD_ZERO(&fds);
                FD_SET(fd[i], &fds);
                select(fd[i] + 1, &fds, NULL, NULL, NULL);
		ret = read(fd[i], buf, DATA_SIZE);
		if(ret < 0){
			break;
		}
		if(*(char *)buf != i){
			printf("buf 0x%x expect 0x%x\n", *(char *)buf, i);
			break;
		}
	}
	for(i = 0; i < MAX_PORT; i++){
		close(fd[i]);
	}

	free(buf);
	return 0;
}
