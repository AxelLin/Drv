/*copyright (c) 2006 Hisilicon Co., Ltd. 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
* This file is based on linux-2.6.14/drivers/char/watchdog/softdog.c
* 
* 20060830 Liu Jiandong <liujiandong@hisilicon.com>
* 	Support Hisilicon's chips, as Hi3511.
*/


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/arch/hardware.h>
#include <linux/kthread.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/pci.h>

#include <linux/string.h>
#include <pci_vdd.h>
#include"./pci_direct_trans.h"
#include "../drv/include/hi3511_pci.h"
#include "vdd-utils.h"

#define MDI_DMA_SIZE (100*1024)

#if 0
static struct timeval s_old_time;
static int PCI_PrintTimeDiff(const char *file, const char *fun, const int line)
{
       struct timeval cur;
       long diff = 0;  /* unit: millisecond */  
       do_gettimeofday(&cur);
       diff = (cur.tv_sec - s_old_time.tv_sec) * 1000 + (cur.tv_usec - s_old_time.tv_usec)/1000;
       printk("\nfile:%s, function:%s, line:%d, millisecond:%ld\n", file, fun, line, diff); 
       s_old_time = cur;
}
#define PCI_DBG_TM_INIT()  do_gettimeofday(&s_old_time)
#define PCI_DBG_TM_DIFF()  PCI_PrintTimeDiff(__FILE__, __FUNCTION__, __LINE__)
#endif

#define MAX_SLOT_NUM 5
struct _pci_addr_map{
	int 					 slot; /*device slot number,marks a device uniquely*/	
	unsigned long		shared_mem_end; /* end of shared memory */
	unsigned long		shared_mem_start;	/* start of shared memory */
	unsigned long		pf_base_addr;	/* prefetchable space base address of device */
	unsigned long		pf_end_addr;	/* prefetchable space end address of device */
	unsigned long		np_base_addr;	/* non_prefetchable space base address of device */
	unsigned long		np_end_addr;	/* non_prefetchable space end address of device */
	unsigned int		irq;	
	struct pci_dev       *pdev;
	void *			handle;			/* pci communication layer handle */
};
struct _pci_addr_map pci_addr_map[MAX_SLOT_NUM];


extern struct hi3511_dev hi3511_device_head;

struct   hi3511_dev  *hi3511_dev_head = &hi3511_device_head;

unsigned long set_mem, read_mem;			

#define	HI3511_TRANS_BASE  	'H'
#define	WDIOC_GETSTATUS		_IOR(HI3511_TRANS_BASE, 0, int)
#define	WDIOC_GETBOOTSTATUS	_IOR(HI3511_TRANS_BASE, 1, int)
#define	WDIOC_GETTEMP		_IOR(HI3511_TRANS_BASE, 2, int)


#define hiwdt_readl(x)		readl(IO_ADDRESS(HIWDT_REG(x)))
#define hiwdt_writel(v,x)	writel(v, IO_ADDRESS(HIWDT_REG(x)))

#define TEST_DBUG_LEVEL1	0
#define TEST_DBUG_LEVEL2	1
#define TEST_DBUG_LEVEL3	2
#define TEST_DBUG_LEVEL4	3
#define TEST_DBUG_LEVEL		3	
#define TEST_DEBUG(level, s, params...)   do{ \
	if(level >= TEST_DBUG_LEVEL)  \
		printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, ##params); \
	}while(0)

static __inline__ void * memcpy_pci(void * dest,const void *src,size_t count)
{
		
	__asm__ __volatile__(
		"stmdb sp!, {r0-r12}" "\n"		
	"mov r0, %0 " "\n"			
	"mov r1, %1" "\n"			
	"mov r2, %2" "\n"			
	"1:" "\n"			
		"ldmia r1!, {r4-r12}" "\n"		
		"stmia r0!, {r4-r12}" "\n"		
		"subs  r2, #32" "\n"		
	"bgt 1b" "\n"			
		"ldmia sp!, {r0-r12}" "\n" 		
	: 			
		:"r"(dest),"r"(src),"r"(count)		
	:"memory"			
 	);			
return 0;
}	

#ifndef CONFIG_PCI_HOST
extern struct semaphore pci_dma_sem;
static int hi3511_trans_slave_receive(void *handle, void *data)
{
	struct PCI_TRANS descriptor;
	int int_flags;
	int rvlen;
	rvlen = pci_vdd_recvfrom(handle,(void *)&descriptor, sizeof(struct PCI_TRANS), 0);//61104	
	if(rvlen <= 0){
		TEST_DEBUG(1, "rvlen <= 0:%d",rvlen);
		goto trans_receive_out;
	}
	else{
		TEST_DEBUG(1, "rvlen > 0:%d",rvlen);
	}

	int_flags = down_interruptible(&pci_dma_sem);
	if(int_flags) {
		TEST_DEBUG(1,"hi3511_trans_slave_receive pci_dma_sem interrupt %d",0);
		return -1;
	}

	hi3511_pci_dma_write((unsigned long)descriptor.dest, (unsigned long)descriptor.source,descriptor.len);
	
	up(&pci_dma_sem);	

	rvlen = pci_vdd_sendto(handle,(void *)&descriptor, sizeof(struct PCI_TRANS), 0x6);//61104	
	if(rvlen <= 0){
		TEST_DEBUG(1, "rvlen <= 0:%d",rvlen);
		goto trans_receive_out;
	}

trans_receive_out:
	
	return rvlen;
}
#endif

static int hi3511_trans_host(void)
{
	return vdd_is_host();
}

static int hi3511_trans_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int hi3511_trans_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t hi3511_trans_read(struct file *file, char *buf,
                        size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t hi3511_trans_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	return 0;
}

static int hi3511_trans_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
//	unsigned long base_addr_phy,end_addr_phy;
	int slot_num;
	int target;
	struct PCI_TRANS descriptor;
	struct PCI_TRANS *dma_descriptor = &descriptor;

//	dma_descriptor=kmalloc(sizeof(struct PCI_TRANS),GFP_KERNEL);
	int is_host ;
	switch (cmd) {
		case 0:
			{
				struct list_head *node = NULL;
				struct hi3511_dev *pNode = NULL;			
				int i = 0;
				is_host = hi3511_trans_host();
				if(is_host){	
					list_for_each (node, &(hi3511_dev_head->node)){
						pNode = list_entry(node, struct hi3511_dev, node);
						pci_addr_map[i].slot = pNode->slot;
						pci_addr_map[i].pf_base_addr = pNode->pf_base_addr;
						pci_addr_map[i].np_base_addr = pNode->np_base_addr;
						pci_addr_map[i].pdev = pNode->pdev;
					}
					i++;
				}
				break;
			}

		case 5:
			{
				void __user *argp = (void __user *)arg;
				void *handle = NULL;
				copy_from_user(dma_descriptor,argp,sizeof(struct PCI_TRANS));
				
				target = dma_descriptor->target ;
				slot_num = pci_vdd_target2slot(target);
				
				TEST_DEBUG(2, "pci_addr_map slot %d",slot_num);
				// make sure the target in slot_num exist
				if(pci_addr_map[target - 1].pf_base_addr != 0){
					TEST_DEBUG(2, "pci_addr_map slot %d",target);

					if(pci_addr_map[target - 1].handle == NULL){
						handle = pci_vdd_open(target, TRANS_MSG_ID, TRANS_PORT_ID);
						if(handle == NULL){
							TEST_DEBUG(3,"host can not open pci communication layer target %d msg %d port %d",
									target, TRANS_MSG_ID, TRANS_PORT_ID);
							return -1;
						}
						pci_addr_map[target - 1].handle = handle;
					}else{
						printk("target %d handle  is not empty\n",target);
					}
				}
				else{
					printk("target %d is not initialled\n",target);
					return -1;
				}
				break;
			}
		case 6:
			{
				void __user *argp = (void __user *)arg;
				void *handle = NULL;
				int ret;
				copy_from_user(dma_descriptor,argp,sizeof(struct PCI_TRANS));

				target = dma_descriptor->target;
				slot_num = pci_vdd_target2slot(target);
				TEST_DEBUG(2, "pci_addr_map slot %d",slot_num);
				handle = pci_addr_map[target - 1].handle;
				ret = pci_vdd_sendto(handle, (void *)dma_descriptor, sizeof(struct PCI_TRANS), 0x6);
				if(ret < 0){
					TEST_DEBUG(3,"preview dest src len package send error %d",ret);
					return -1;
				}

				TEST_DEBUG(2, "syn wait %d", 0);
				// syn wait for dma preview data finished
				ret = pci_vdd_recvfrom(handle,  (void *)dma_descriptor, sizeof(struct PCI_TRANS), 0x0);
				if(ret < 0){
					TEST_DEBUG(1,"preview data receive error %d",ret);
					return -1;
				}
				TEST_DEBUG(2, "syn wait out %d", ret);
				break;
			}
		case 7:
			{
				void __user *argp = (void __user *)arg;
				void *handle = NULL;
				copy_from_user(dma_descriptor,argp,sizeof(struct PCI_TRANS));

				target = dma_descriptor->target;
				slot_num = pci_vdd_target2slot(target);
				handle = pci_addr_map[target - 1].handle;
				if(handle)
					pci_vdd_close(handle);
				memset(&pci_addr_map[target - 1], 0, sizeof(struct _pci_addr_map));
				break;
			}
		default:
			return -ENOIOCTLCMD;
	}

	return 0;
}

/*
 *	Kernel Interfaces
 */
static struct file_operations hi3511_trans_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.read		= hi3511_trans_read,
	.write		= hi3511_trans_write,
	.ioctl		= hi3511_trans_ioctl,
	.open		= hi3511_trans_open,
	.release	= hi3511_trans_release,
};

static struct miscdevice hi3511_trans_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "hi3511_trans",
	.fops		= &hi3511_trans_fops,
};

static int __init hi3511_trans_init(void)
{
	int ret = 0;
	int is_host = 0;
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	memset(pci_addr_map, 0, MAX_SLOT_NUM * sizeof(struct _pci_addr_map));
	
	is_host = hi3511_trans_host();
	
	if(is_host){
		ret = misc_register(&hi3511_trans_miscdev);
		if(ret) {
			printk ("cannot register miscdev on minor=%d (err=%d)\n",
				MISC_DYNAMIC_MINOR, ret);
			misc_deregister(&hi3511_trans_miscdev);
			goto hi3511_trans_init_err;
		}
	}
	else{
		
#ifndef CONFIG_PCI_HOST
		void *handle = NULL;
		pci_vdd_opt_t opt;

		handle = pci_vdd_open(TRANS_HOST_ID, TRANS_MSG_ID, TRANS_PORT_ID);
		if(handle == NULL){
			printk("slave preview demon can not open\n");
			ret = -1;
		}
		pci_vdd_getopt(handle, &opt);
		opt.flags = 0x1;       //slave asyn receive
		opt.recvfrom_notify = hi3511_trans_slave_receive;
		pci_vdd_setopt(handle, &opt);
		pci_addr_map[TRANS_HOST_ID].handle = handle;
#endif		
		
	}
hi3511_trans_init_err:
	return ret;
}

static void __exit hi3511_trans_exit(void)
{
	int is_host;
	is_host = hi3511_trans_host();
#if 0
	iounmap((unsigned long *)set_mem);
	iounmap((unsigned long *)read_mem);
#endif
	if(is_host){
		int slot_num = 0;
		void *handle;

		for(; slot_num < MAX_SLOT_NUM; slot_num ++){
			handle = pci_addr_map[slot_num].handle;

			if(handle != NULL){
				pci_vdd_close(handle);
				memset(&pci_addr_map[slot_num], 0, sizeof(struct _pci_addr_map));
			}
		}
		misc_deregister(&hi3511_trans_miscdev);
		is_host = 0;
	}
	else{
		void *handle = pci_addr_map[TRANS_HOST_ID].handle;
		if(handle != NULL){
			pci_vdd_close(handle);
			memset(&pci_addr_map[TRANS_HOST_ID], 0, sizeof(struct _pci_addr_map));
		}
	}
}

module_init(hi3511_trans_init);
module_exit(hi3511_trans_exit);

MODULE_AUTHOR("c61104");
MODULE_DESCRIPTION("Hisilicon Hi3511 PCI Device Test Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);

