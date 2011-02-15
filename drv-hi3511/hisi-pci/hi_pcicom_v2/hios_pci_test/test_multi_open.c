/* hello.c */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "hisi_pci_proto.h"
#define DATA_LEN 400
#define MAX_SEND 5
unsigned long handle[HISI_MAX_PCI_PORT];
static void vdd_testcase1(void)
{
	int ret, i = 0;
	int target_id[16];
	int num;
	void *buf;
	num = pci_vdd_remoteids(&target_id[0]);
        buf = kmalloc(DATA_LEN, GFP_KERNEL);
	if(!buf){
		printk("kmalloc error\n");
		return;
	}
	printk("buf 0x%8lx \n",(unsigned long)buf);
	for(i = 0; i < HISI_MAX_PCI_PORT; i++){
		handle[i] = (unsigned long)pci_vdd_open(target_id[0], i, 0);
		if(!handle[i]){
			printk("open error %d \n", i);
			goto out;
		}
	}
	for(i = 0; i < HISI_MAX_PCI_PORT; i++){
		ret = pci_vdd_sendto((void *)handle[i], buf, DATA_LEN, HISI_PCI_MEM_DATA);
		if(ret < 0){
			printk("send error %d \n", i);
			break;
		}
	}
out:
        kfree(buf);
	for(i = 0; i < HISI_MAX_PCI_PORT; i++){
		pci_vdd_close((void *)handle[i]);
	}
	printk("test case 1 ok\n");
	return ;
}
static void vdd_testcase2(void)
{
}
static int hello_init(void)
{
	printk("hello linux kernel world!\n");
	vdd_testcase1();
	vdd_testcase2();
	return 0;
}

static void hello_exit(void)
{
	printk("Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
