#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


#include "pci_trans.h"

#define isxdigit(c)	(('0' <= (c) && (c) <= '9') \
			 || ('a' <= (c) && (c) <= 'f') \
			 || ('A' <= (c) && (c) <= 'F'))

#define isdigit(c)	('0' <= (c) && (c) <= '9')
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define toupper(c)	(islower(c) ? ((c) - 'a' + 'A') : (c))



int test_dma_receive(int fd)
{
    int mfd;
    int ret, i;
    struct pcit_event pevent;
    unsigned int r_event = HI_PCIT_EVENT_DMAWR_0;
    unsigned int s_event = HI_PCIT_EVENT_DOORBELL_6;
    unsigned int event_mask = 0;
    unsigned int devid = 0;
	unsigned int *dmam;
	unsigned int *dmam_c;

    mfd = open("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
    if (mfd < 0) {
        perror("open '/dev/mem':");
        return -1;        
    }
    ret = ioctl(fd, HI_IOC_PCIT_BINDDEV, &devid);
    if (ret) {
        printf("HI_IOC_PCIT_BINDDEV failed!\n");
        goto inq_err;
    }
    dmam = mmap ((void *)0, 4096, PROT_READ | PROT_WRITE,
             MAP_SHARED, mfd, 0xe8000000);
    if (MAP_FAILED == dmam) {
        perror("mmap:");
        goto inq_err;
    }
    event_mask = 1 << r_event;
    printf("subscribe event %u\n", event_mask);
    ret = ioctl(fd, HI_IOC_PCIT_SUBSCRIBE, &event_mask);
    if(ret) {
        goto inq_err;
    }
	dmam_c = dmam;
    for (;;) {
		
        ret = ioctl(fd, HI_IOC_PCIT_LISTEN, &pevent);
        if (ret) {
            printf("HI_IOC_PCIT_LISTEN failed!\n");
            goto inq_err;
        }
		printf("\nevnet : %x\n", pevent.event_mask);
		for (i = 0; i < 16; i++) {
			if (i == 8)
				printf("\n");
			printf("%8d", *(dmam_c + i));
		}

		
		dmam_c = dmam;
        ret = ioctl(fd, HI_IOC_PCIT_DOORBELL, &s_event);
        if (ret) {
            printf("HI_IOC_PCIT_DOORBELL failed!\n");
            goto inq_err;
        }

    }
inq_err:
	ioctl(fd, HI_IOC_PCIT_UNSUBSCRIBE, &event_mask);
    close(mfd);
    return 0; 
}



int test_doorbell(int fd)
{
    struct pcit_bus_dev devs;
    char *shm;
    int mfd;
    int ret,i = 0;
    struct pcit_event pevent;
    unsigned int r_event = HI_PCIT_EVENT_DOORBELL_5;
    unsigned int s_event = HI_PCIT_EVENT_DOORBELL_6;
    unsigned int event_mask = 0;

    unsigned int devid = 0;

    mfd = open("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
    if (mfd < 0) {
        perror("open '/dev/mem':");
        return -1;        
    }
    devs.bus_nr = 0;
    ret = ioctl(fd, HI_IOC_PCIT_INQUIRE, &devs);
    if (ret) {
        printf("inquire failed!");
        goto inq_err;
    }
    printf("pf_phys_addr: %lx, pf_size = %lx\n",devs.devs[0].pf_phys_addr,
        devs.devs[0].pf_size);
    shm = mmap ((void *)0, devs.devs[0].pf_size, PROT_READ | PROT_WRITE,
             MAP_SHARED, mfd, devs.devs[0].pf_phys_addr);
    if (MAP_FAILED == shm) {
        perror("mmap:");
        goto inq_err;
    }
    ret = ioctl(fd, HI_IOC_PCIT_BINDDEV, &devid);
    if (ret) {
        printf("HI_IOC_PCIT_BINDDEV failed!\n");
        goto inq_err;
    }
    
    event_mask = 1 << r_event;
    printf("subscribe event %u\n", event_mask);
    ret = ioctl(fd, HI_IOC_PCIT_SUBSCRIBE, &event_mask);
    if(ret) {
        goto inq_err;
    }
    for (;;) {
        ret = ioctl(fd, HI_IOC_PCIT_LISTEN, &pevent);
        if (ret) {
            printf("HI_IOC_PCIT_LISTEN failed!\n");
            goto inq_err;
        }
        (*(int *)shm)++;
        printf("%8d",*(int *)shm);		
        ret = ioctl(fd, HI_IOC_PCIT_DOORBELL, &s_event);
        if (ret) {
            printf("HI_IOC_PCIT_DOORBELL failed!\n");
            goto inq_err;
        }
		if (i++ == 8){
			printf("\n");			
			i = 0;
		}
    }
inq_err:
	munmap(shm, devs.devs[0].pf_size);
    close(mfd);
    return 0; 
}

int test_window(int fd)
{
    struct pcit_bus_dev devs;
    char *shm;
    int mfd;
    int i = 0;
    int ret;
    unsigned int event = HI_PCIT_EVENT_DOORBELL_5;
    unsigned int devid = 0;

    mfd = open("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
    if (mfd < 0) {
        perror("open '/dev/mem':");
        return -1;        
    }
    devs.bus_nr = 0;
    ret = ioctl(fd, HI_IOC_PCIT_INQUIRE, &devs);
    if (ret) {
        printf("inquire failed!");
        goto inq_err;
    }
    for (i = 0; i < HI_PCIT_MAX_SLOT; i++) {
        printf("slot: %d, pf_phys_addr: %lx, pf_size = %lx\n", 
            devs.devs[i].slot, devs.devs[i].pf_phys_addr, devs.devs[i].pf_size);
    }

    shm = mmap ((void *)0, devs.devs[0].pf_size, PROT_READ | PROT_WRITE,
             MAP_SHARED, mfd, devs.devs[0].pf_phys_addr);
    if (MAP_FAILED == shm) {
        perror("mmap:");
        goto mmap_err;
    }
    memset(shm, 0xa5, 64);
    for (i = 0; i < 16; i++) {
        printf("0x%x ", *((int *)shm + i));
    }
    ret = ioctl(fd, HI_IOC_PCIT_BINDDEV, &devid);
    if (ret) {
        printf("HI_IOC_PCIT_BINDDEV failed!\n");
        goto inq_err;
    }
    
    ret = ioctl(fd, HI_IOC_PCIT_DOORBELL, &event);
    if (ret) {
        printf("HI_IOC_PCIT_DOORBELL failed!\n");
        goto inq_err;
    }
    printf("send evnent ok!\n");

inq_err:
    munmap(shm, devs.devs[0].pf_size);
mmap_err:
    close(mfd);
    return 0;
    
}


#if 0
static int cmd_get_data_trans(char* arg, int default_trans)
{
	/* Check for a transfer mode:
	.m-memory copy;.d-dma write;.r-dma read
	 */
	int len = strlen(arg);
	printf(" arg len: %d\n",len);
	if (len > 2 && arg[len-2] == '.') {
		switch(arg[len-1]) {
		case 'w':
			printf(" case window\n");
			return 1;
		case 'd':
			printf(" case dma\n");
			return 2;

		default:
			printf(" default\n");
			return -1;
		}
	}
	return default_trans;
}


/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	} else if (base == 16) {
		if (cp[0] == '0' && toupper(cp[1]) == 'X')
			cp += 2;
	}
	while (isxdigit(*cp) &&
	       (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

#endif


int main(int argc, char *argv[])
{
	int trans = 0;
    int fd;
#if 0    
	unsigned long   slot, addr_target, addr_source, len;

	if ((trans = cmd_get_data_trans(argv[0], 2)) < 1)
			return 1;

	/*slot number*/
	slot= simple_strtoul(argv[1], (void *)0, 16);
    
	/* target address */
	addr_target= simple_strtoul(argv[2], (void *)0, 16);
    
	/* source address	*/
	addr_source = simple_strtoul(argv[3], (void *)0, 16);
    
	/* length */
	len = simple_strtoul(argv[4], (void *)0, 16);
#endif

    fd = open("/dev/misc/hi3511_pcit", O_RDWR);
    if (fd < 0 ) {
        perror("open");
        return -1;
    }

    trans = 3;
    
    /* access share window. */
	if(trans == 1)
	{
        test_window(fd);
	}else if(trans == 2) {
		test_doorbell(fd);
	}else if(trans == 3) {
		test_dma_receive(fd);
	} else {
	    printf("no test cased excuted!\n");
    }

    close(fd);
    return 0;
}

