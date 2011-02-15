/* hello.c */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "hisi_pci_proto.h"
#define DATA_LEN 400
#define MAX_SEND 5000
static void vdd_testcase1(void)
{
	unsigned long handle = 0;
	struct timeval tv1,tv2;
	int target_id[16];
	int num;
	int i = 0;
	void *buf;
	int ret;
	unsigned long sendlen = 0;

	num = pci_vdd_remoteids(&target_id[0]);
	handle = (unsigned long)pci_vdd_open(target_id[0], 0, 0);
	if(!handle){
		printk("open error\n");
		goto out;
	}
	buf = kmalloc(DATA_LEN, GFP_KERNEL);
	do_gettimeofday(&tv1);
	for(; i < MAX_SEND; i++ ){
		ret = pci_vdd_sendto((void *)handle, buf, DATA_LEN, HISI_PCI_MEM_DATA);
		if(ret < 0)
			break;
		else
			sendlen += ret;
	}
	do_gettimeofday(&tv2);
	kfree(buf);
	pci_vdd_close((void*)handle);
	printk("send %lu bytes in %ld s %ld us\n",sendlen, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
out:
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
