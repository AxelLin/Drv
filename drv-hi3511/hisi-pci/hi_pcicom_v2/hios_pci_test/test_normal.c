/* hello.c */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "hisi_pci_proto.h"
#define DATA_LEN 40
#define MAX_SEND 20
#define MAX_PORT 20

static void vdd_testcase1(void)
{
	unsigned long handle[MAX_PORT] = {0};
	int i = 0, j = 0, k = 0;
	void *buf;
	int ret = 0;
	int target_id[16];
	int num;
	num = pci_vdd_remoteids(&target_id[0]);
	buf = kmalloc(DATA_LEN, GFP_KERNEL);
	
	for(i = 0; i < MAX_PORT; i++){
		handle[i] = (unsigned long)pci_vdd_open(target_id[0], i, 0);
		if(!handle[i]){
			printk("open error handle %d\n", i);
			goto out;
		}
		for(j = 0; j < MAX_SEND; j++ ){
			ret = pci_vdd_sendto((void *)handle[i], buf, DATA_LEN, HISI_PCI_MEM_DATA);
			if(ret < 0)
				goto out;
		}
	}
	i = MAX_PORT - 1;
out:
	kfree(buf);
	for(k = 0; k <= i; k++){
		if(handle[k])
			pci_vdd_close((void*)handle[k]);
	}
	printk("test case 1 handle %d ret %d send times %d\n",i, ret, j);
	return ;
}

static int hello_init(void)
{
	printk("hello linux kernel world!\n");
	vdd_testcase1();
	return 0;
}

static void hello_exit(void)
{
	printk("Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);
