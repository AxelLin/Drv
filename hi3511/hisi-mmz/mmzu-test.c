
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include "mmz-userdev.h"

#define DDAL_RUN_TEST(v, cond) do{ \
		printf("[1;33m%s: " #v #cond "[0;39m\n", \
				((v)cond) ? "[1;32m[ OK  ]" : "[1;31m[FAILD]"); \
		fflush(stdout); \
		usleep(10000); \
	}while(0)

static int test_mem(int fd, int cycles, struct mmb_info *pmi)
{
	static unsigned long counter = 1;
	int test_result, j, i;
	volatile unsigned long *data;

	data = pmi->mapped;

	test_result = 0;
	for(j=0; j<cycles; j++) {
		for(i=0; i<pmi->prot; i++) {
			data[i] = counter++;
		}

		test_result += ioctl(fd, IOC_MMB_TEST_CACHE, pmi);

		for(i=0; i<pmi->prot; i++) {
			unsigned long val;

			counter++;
			val = data[i];

			if(val != ~counter)
				test_result++;
		}
	}

	return test_result;
}

#define TEST_CYCLES 10000
#define MAX_MMBS 32

int main(int argc, char *argv[])
{
	struct mmb_info mmi = {0};
	int fd;
	struct mmb_info vmi[MAX_MMBS];
	int i;

	DDAL_RUN_TEST(fd=open("/dev/himedia/mmz_userdev", O_RDWR), >0);

	strlcpy(mmi.mmb_name, "buff 0", HIL_MMB_NAME_LEN);
	mmi.size = 0x100000;
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ALLOC, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	mmi.prot = PROT_READ | PROT_WRITE;
	mmi.flags = MAP_SHARED;
	printf("\n");

	/********************************************************/
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP, &mmi), ==0);
	mmi.gfp = 0;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");

	/********************************************************/
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP_CACHED, &mmi), ==0);
	mmi.gfp = 0;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), >0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");

	/********************************************************/
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP_CACHED, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP, &mmi), ==0);
	mmi.gfp = 0;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), >0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");

	/********************************************************/
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP_CACHED, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP_CACHED, &mmi), ==0);
	mmi.gfp = 0;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), >0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");

	/********************************************************/
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP_CACHED, &mmi), ==0);
	mmi.gfp = 1;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");

	/********************************************************/
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP_CACHED, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP, &mmi), ==0);
	mmi.gfp = 4;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");

	/********************************************************/
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ATTR, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP_CACHED, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP_CACHED, &mmi), ==0);
	mmi.gfp = 4+1;
	mmi.prot = 32;
	//DDAL_RUN_TEST(test_mem(fd, TEST_CYCLES, &mmi), ==0);
	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), ==0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), ==0);
	printf("\n");


	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_UNMAP, &mmi), !=0);
	//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_UNMAP, &mmi), !=0);

	DDAL_RUN_TEST(ioctl(fd, IOC_MMB_FREE, &mmi), ==0);

	memset(&vmi, 0, sizeof(vmi));
	for(i=0; i<MAX_MMBS; i++) {
		snprintf(vmi[i].mmb_name, HIL_MMB_NAME_LEN, "buffer %d", i);
		vmi[i].size = 128*1024;
		vmi[i].prot = PROT_READ | PROT_WRITE;
		vmi[i].flags = MAP_SHARED;
		DDAL_RUN_TEST(ioctl(fd, IOC_MMB_ALLOC, &vmi[i]), ==0);
		DDAL_RUN_TEST(ioctl(fd, IOC_MMB_USER_REMAP, &vmi[i]), ==0);
		//DDAL_RUN_TEST(ioctl(fd, IOC_MMB_KERN_REMAP, &vmi[i]), ==0);
		memset(vmi[i].mapped, 0, 128*1024);
	}

	DDAL_RUN_TEST(close(fd), ==0);

	fflush(stdout);
	usleep(100000);

	return 0;
}
