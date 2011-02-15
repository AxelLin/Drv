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
	attr.target_id = 1;
	attr.port = 0;
	attr.priority = 0;

	for(i = 0; i < MAX_PORT; i++){
	        fd[i] = open("/dev/himedia/mcc_userdev", O_RDWR);
		attr.port = i;
		ioctl(fd[i], HI_MCC_IOC_CONNECT, &attr);
	}

	printf("data coming\n");
	for(i = 0; i < MAX_PORT; i++){
		FD_ZERO(&fds);
		FD_SET(fd[i], &fds);
		select(fd[i] + 1, &fds, NULL, NULL, NULL);

		memset(buf, 0xee, DATA_SIZE);
		ret = read(fd[i], buf, DATA_SIZE);
		if(ret < 0){
			printf("receive len %d\n",ret);
			goto out;
		}
		ret = write(fd[i], buf, DATA_SIZE);
		if(ret < 0){
			printf("send error %d \n",ret);
			goto out;
		}
#if 1
		if(*(char *)buf != i){
			printf("\nbuf %d expext %d\n", *(char *)buf , i);
			goto out;
		}
#else
		int j;
		for(j = 0; j < 8; j++){
			if((j % 8 == 0 ) && (j != 0))
				printf("\n");

			printf("0x%2x ", *((char *)buf + j));
		}
		printf("\nreceived %d\n", ret);
#endif
	}

out:
	printf("\n%d handle received buf\n", i);
	for(i = 0; i < MAX_PORT; i++){
		close(fd[i]);
	}

	free(buf);
	return 0;
}
