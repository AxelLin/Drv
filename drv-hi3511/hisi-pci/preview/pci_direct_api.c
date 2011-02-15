#include <stdio.h>
#include <fcntl.h>
#include <linux/kernel.h>

#include "pci_direct_api.h"

#define HI3511_PCI_DEV      "/dev/misc/hi3511_trans"
#define HI3511_PCI_SLAVE    5 

static int pos = 0;
static int handle[HI3511_PCI_SLAVE];
static int gPCIFD;
int hi_pci_init(void)
{
	int pciFD = 0;
	int ret;
	static int flag = 0;
	if(0 < gPCIFD){
		return -1;
	}

	if(!flag){
		pciFD = open(HI3511_PCI_DEV, O_RDWR, 0);
		if (pciFD < 0){
			printf("open pci device error!!\n");
			return -1;
		}
	
		gPCIFD = pciFD;
		flag = 1;
	}

	ret = ioctl(gPCIFD,0, 0);

	if(ret < 0){
		printf("PCI open error\n");
		return -1;
	}
	memset(handle, 0 , HI3511_PCI_SLAVE*sizeof(int));
	return 0;
}

int hi_pci_close()
{
	struct PCI_TRANS trans_descriptor;
	int i = 0;
	int ret;
	
	for(i = 0; i < HI3511_PCI_SLAVE; i++){
		if(handle[i] == 1){
			trans_descriptor.target = i;
			ret = ioctl(gPCIFD, 7, (unsigned long)&trans_descriptor);
			if(ret < 0){
				printf("PCI close slot %d error\n",i);
			}
		}
	}

	close(gPCIFD);
	gPCIFD = 0;
	return 0;
}

int hi_pci_mmcpy(struct PCI_TRANS *trans_descriptor)
{
	int ret;
	ret = ioctl(gPCIFD,3,(unsigned long)trans_descriptor);
	if(ret < 0){
		printf("PCI memory copy error!!\n");
		return -1;
	}
	return 0;
}

/*print out memory resource of the slot*/
int hi_pci_check(int slot)
{
	int ret;
	ret = ioctl(gPCIFD,4,slot);
	if(ret < 0){
		printf("PCI check error!!\n");
		return -1;
	}
	return 0;
}

int hi_pci_slave_dma_write(struct PCI_TRANS *trans_descriptor)
{
	int ret;
	int target;
	target = trans_descriptor->target;

	// open slot board
	if(handle[target] == 0){
		/* open pci communication layer */
		ret = ioctl(gPCIFD, 5, (unsigned long)trans_descriptor);
		if(ret < 0){
			printf("PCI slave dma test error!\n");
			return -1;
		}
		handle[target] = 1;
	}
	ret = ioctl(gPCIFD,6,(unsigned long)trans_descriptor);
	if(ret < 0){
		printf("PCI slave dma test error!\n");
		return -1;
	}
	return 0;
}
