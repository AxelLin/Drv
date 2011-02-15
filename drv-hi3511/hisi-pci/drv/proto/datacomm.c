#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm-arm/io.h>
#include <linux/list.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "../../include/pci_vdd.h"
#include "proto.h"
#include "datacomm.h"
#include "../include/hi3511_pci.h"

#ifdef CONFIG_PCI_HOST
struct Hi3511_pci_slave2host_mmap_info Hi3511_slave2host_mmap_info;
extern void *host_recv_buffer;
extern unsigned long host_sharebuffer_start;
extern atomic_t   host2slave_irq[HI3511_PCI_SLAVE_NUM];
#else

struct Hi3511_pci_slave_mmap_info Hi3511_slave_mmap_info;
extern unsigned long slave_sharebuffer_start;
extern void *datarelay;
extern unsigned long settings_addr_physics;
extern unsigned long hi3511_slave_interrupt_flag; 
unsigned long host_fifo_stuff_physics;
//#define slave_fifo_stuff_physics  (slave_sharebuffer_start + PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE)
extern atomic_t   slave2host_irq;
#endif

#ifdef  CONFIG_PCI_TEST
	unsigned long prewake;
	static unsigned long afterwake;
#endif

extern void *exchange_buffer_addr;
extern struct semaphore pci_dma_sem;

extern struct hi3511_pci_data_transfer_handle_table *hi3511_pci_handle_table;

extern struct pci_vdd_info hi3511_pci_vdd_info;

extern struct   hi3511_dev  *hi3511_dev_head;

#define HI_PCI_DEVICE_NUMBER 5
extern struct mem_addr_couple hi3511_mem_map[HI_PCI_DEVICE_NUMBER];


extern struct task_struct *precvthread;

extern atomic_t   lostpacknum;
extern atomic_t   irq_num;
extern atomic_t   readpack_num;
extern atomic_t   threadexitflag;



/* 64 bit orderlines. for DMA */
/* 64 bit ¶ÔÆëDMA*/
#define SHARE_MEMORY_128BIT_ORDERLINES    24
#define SHARE_MEMORY_64BIT_ORDERLINES      8

// lock normal pack 
int hi3511_pci_locksend_normal_buffer(int target)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[target - 1]; // target id -1 = mmapinfo number
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif 
	int ret;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_locksend_normal_buffer target %d",target);
	
	/* lock */
	ret = down_interruptible(&pmmapinfo->send.normalsemaphore);
	if(ret){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",1);	
		return -ERESTARTSYS;
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"down_interruptible end %d",target);

	return ret;
}

// release lock normal pack
void hi3511_pci_unlocksend_normal_buffer(int target)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[target - 1]; // target id -1 = mmapinfo number
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif 	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_unlocksend_normal_buffer target %d",target);

	up(&pmmapinfo->send.normalsemaphore);
}

// lock priority pack 
int hi3511_pci_locksend_priority_buffer(int target)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[target - 1]; // target id -1 = mmapinfo number
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif 
	int ret = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_locksend_priority_buffer target %d",target);

	/* lock */
	ret = down_interruptible(&pmmapinfo->send.prisemaphore);
	if(ret){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);	
		return -ERESTARTSYS;
	}
	return ret;
}

// release lock priority pack
void hi3511_pci_unlocksend_priority_buffer(int target)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[target - 1]; // target id -1 = mmapinfo number
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif 	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_unlocksend_priority_buffer target %d",target);

	up(&pmmapinfo->send.prisemaphore);
}

// get the readpointer of send buffer
int hi3511_pci_getsend_priority_rwpointer(int target)
{

	int	ret;
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[target - 1];
	ret =  __raw_readw((char *)pmmapinfo->send.pripackageflagaddr);
#else
	unsigned long int_flags;
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo

	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getsend_priority_rwpointer target %d",target);
	//local_irq_save(int_flags);

	/* lock dma */
	int_flags = down_interruptible(&pci_dma_sem);
	if(int_flags) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
		return -1;
	}

	hi3511_pci_dma_read(pmmapinfo->send.pripackageflagaddr, (settings_addr_physics+pci_writeable_pripackage_flag_r), 4);
#if 0
	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.pripackageflagaddr);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics+pci_writeable_pripackage_flag_r));

	hi3511_bridge_ahb_writel(RDMA_CONTROL, 0x00000461);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);

#endif

	hi3511_pci_dma_read(pmmapinfo->send.pripackageflagaddr, slave_fifo_stuff_physics,0x100);

#if 0
	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.pripackageflagaddr);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, slave_fifo_stuff_physics);

	hi3511_bridge_ahb_writel(RDMA_CONTROL, 0x00010061);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
#endif

	ret = (*(volatile unsigned long *)(datarelay+pci_writeable_pripackage_flag_r))&0xffff;

	//local_irq_restore(int_flags);
	up(&pci_dma_sem);

#endif 

	return ret;

}


int hi3511_pci_write_priorityhead(struct Hi3511_pci_data_transfer_head *head,void *mmapinfo)
{
	const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);

#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)mmapinfo ;
	memcpy((char *)(pmmapinfo->send.pripackageaddr - headsize),head,headsize);

#else
	unsigned long int_flags;
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)mmapinfo;
	
	//local_irq_save(int_flags);
	/* lock dma */
	int_flags = down_interruptible(&pci_dma_sem);
	if(int_flags) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
		return -1;
	}

	memcpy(datarelay+write_head, head , headsize);
	
	//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);	//from running ko. lock for host-slave dma resource

	hi3511_pci_dma_write((pmmapinfo->send.pripackageaddr-headsize), (settings_addr_physics+write_head),headsize);

#if 0	
	control = (headsize<<8)| 0x71;
	//printk("content of head is: \n");
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave write the host address is 0x%lx \n",(unsigned long)pmmapinfo->send.pripackageflagaddr-headsize);
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmapinfo->send.pripackageaddr-headsize);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"at the same time hi3511 slave ddr address is 0x%lx \n",(unsigned long)settings_addr_physics);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_head));

	hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);	
#endif

	hi3511_pci_dma_write(host_fifo_stuff_physics, (settings_addr_physics+write_head), 0x40);
#if 0
	control = (0x40<<8)| 0x71;
	
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, host_fifo_stuff_physics);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_head));

	hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);	
#endif
	//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
	//local_irq_restore(int_flags);
	up(&pci_dma_sem);

#endif 
	return 0;
}

// host and slave use it with MACRO CONFIG_PCI_HOST
int hi3511_pci_add2buffer_priority(struct Hi3511_pci_data_transfer_head *head, char *buffer, int len,int flags)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[head->target - 1]; // target id -1 = mmapinfo number
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
	unsigned long int_flags;
#endif

	unsigned int writeflag = 0x1;
	int ret = -1;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_add2buffer_priority target %d",head->target);

	if(pmmapinfo->mapped != PCI_SLAVE_BEEN_MAPPED) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"target %d memory not mapped yet",head->target & 0xF );
		return -1;
	}
	
	 if(head->pri != HI3511_PCI_PACKAGE_PRI_LEVEL_1) {

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"PRI package %d",0);
	 	
		// use api for usr write head
		(void) hi3511_pci_write_priorityhead(head,(void *)pmmapinfo);


		/* i guess if package larger than max size , i just need to let higher layer know it */
		if(len > PCI_HIGHPRI_PACKAGE_SIZE){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"buffer len %d larger than pri package max size",len);
			
			ret = PCI_PRI_SHARE_MEMORY_NOTENOUGH;

			goto hi3511_pci_add2buffer_priority_releaseprilock;
		}

#ifdef CONFIG_PCI_HOST
		ret = hi3511_pci_writepack(head->target,hi3511_mem_map[head->target - 1].base_addr_phy+(char *)pmmapinfo->send.pripackageaddr-hi3511_mem_map[head->target - 1].base_addr_virt,buffer,len,flags);
#else
		ret = hi3511_pci_writepack(head->target,(char *)pmmapinfo->send.pripackageaddr,buffer,len,flags);
#endif
		if(ret <= 0){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"hi3511_pci_writepack error %d",0);		
			goto hi3511_pci_add2buffer_priority_releaseprilock;			
		}

#ifdef 	CONFIG_PCI_HOST	
		__raw_writew(writeflag,(char *)pmmapinfo->send.pripackageflagaddr);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"write pri wirteable %d",writeflag);

		udelay(20);
		
		//writeflag =  __raw_readw((char *)pmmapinfo->send.pripackageflagaddr);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pri wirteable %d",writeflag);
#else
//		local_irq_save(int_flags);

		/* lock dma */
		int_flags = down_interruptible(&pci_dma_sem);
		if(int_flags) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
			return -1;
		}

		//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);

	       *(volatile unsigned long *)(datarelay+pci_writeable_pripackage_flag) = writeflag;

		hi3511_pci_dma_write(pmmapinfo->send.pripackageflagaddr, (settings_addr_physics+pci_writeable_pripackage_flag), 2);
#if 0
	       control = (2<<8)| 0x71;

	       hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmapinfo->send.pripackageflagaddr);

	       hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+pci_writeable_pripackage_flag));

	       hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	       while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif
		hi3511_pci_dma_write(host_fifo_stuff_physics, (settings_addr_physics+pci_writeable_pripackage_flag), 0x40);
#if 0
	       control = (0x40 << 8)| 0x71;

	       hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, host_fifo_stuff_physics);

	       hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+pci_writeable_pripackage_flag));

	       hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	       while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif
		//local_irq_restore(int_flags);
		//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);

		up(&pci_dma_sem);
#endif

#ifdef 	CONFIG_PCI_HOST	
		// enable interrupt to send
		//printk("Value of head->target-1 is %x in priority",head->target - 1+2);
		//master_to_slave_irq(head->target - 1+2);//c61104-071011//20071113

		//pci_bus_write_config_dword(hi3511_mem_map[head->target-1+2].pdev->bus, hi3511_mem_map[head->target-1+2].pdev->devfn, PCI_ICMD, 0x000f0000);
	
		master_to_slave_irq(pmmapinfo->slot);
#else
		slave_to_master_irq(0);
#endif
		ret = len;

	}
hi3511_pci_add2buffer_priority_releaseprilock:	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_add2buffer_priority end %d",0);

	return ret;
}

int hi3511_pci_write_normalhead(struct Hi3511_pci_data_transfer_head *head,void *mmapinfo,int writepointer)
{
	const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);

#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)mmapinfo;
	memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer - headsize),head,headsize);
#else
	unsigned long int_flags;	
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)mmapinfo;
	
	//local_irq_save(int_flags);
	/* lock dma */
	int_flags = down_interruptible(&pci_dma_sem);
	if(int_flags) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
		return -1;
	}

	//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);

	memcpy(datarelay+write_head, head , headsize);

	hi3511_pci_dma_write((pmmapinfo->send.normalpackageaddr + writepointer - headsize), (settings_addr_physics+write_head),headsize);

#if 0	
	control = (headsize<<8)| 0x71;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave write the host address is 0x%lx \n",(unsigned long)pmmapinfo->send.normalpackageaddr + writepointer - headsize);
	
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmapinfo->send.normalpackageaddr + writepointer - headsize);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"at the same time hi3511 slave ddr address is 0x%lx\n",(unsigned long)settings_addr_physics);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_head));

	hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif

	hi3511_pci_dma_write(host_fifo_stuff_physics, (settings_addr_physics+write_head), 0x40);
#if 0
#if pci_dma_push_fifo

	control = (0x40<<8)| 0x71;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, host_fifo_stuff_physics);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_head));

	hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); 	

#endif
#endif

//	if(i >= 5000)
	//	return -1;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"**write head finished** \n");

// by chanjinn 20071017
#if  0
	control = (8 << 8)| 0x61;

	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.normalpackageaddr + writepointer - headsize);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics + write_head_r));

	hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);

	control = (0x100<<8)| 0x61;

	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.normalpackageaddr + writepointer - headsize);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, slave_fifo_stuff_physics);

	hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"**read head finished** \n");
	{

	int cmp = 0;
	cmp=memcmp(head,(char *)(datarelay+ write_head_r),headsize);
	if(cmp!=0)
	{
		char *phead = (char *)head;
		char *pdata = (char *)datarelay + write_head_r;
		int i;
		__raw_writel(0xfacebeee, datarelay+write_head);
		control = (0x4<<8)| 0x71;
		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, host_fifo_stuff_physics);
		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_head));
		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);
		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); 	

              printk("first error data :");
                for (i = 0 ; i < headsize ; i ++)
                {
                        printk ("%x ", *(pdata+i));
                }
                printk("\n");

		memset(pdata,0,headsize);
		
		control = (headsize<<8)| 0x61;

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.normalpackageaddr + writepointer - headsize);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics + write_head_r));

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);

		control = (0x100<<8)| 0x61;

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.normalpackageaddr + writepointer - headsize);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, slave_fifo_stuff_physics);
	
		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"!!!!!!!before and after DMA	, head content is difference !!!!!!!! \n ");

		printk("head :");
		for (i = 0 ; i < headsize ; i ++)
		{
			printk ("%x ", *(phead+i));
		}
		printk("\n");

		printk("second data :");
                for (i = 0 ; i < headsize ; i ++)
                {
                        printk ("%x ", *(pdata+i));
                }
                printk("\n");
	}
	}
#endif
	//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
	//local_irq_restore(int_flags);
	
	up(&pci_dma_sem);
#endif 

	return 0;
}

// host and slave use it with MACRO CONFIG_PCI_HOST
int hi3511_pci_add2buffer_normal(struct Hi3511_pci_data_transfer_head *head, char *buffer, int len,int flags)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[head->target - 1]; // target id -1 = mmapinfo number
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif 

	const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	int readpointer = 0, writepointer = 0;
	int steplen1,steplen2;
	int ret = -1;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_add2buffer_normal target %d",head->target);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pmmapinfo->mapped %d",(int) pmmapinfo->mapped);	

	if((int)pmmapinfo->mapped != PCI_SLAVE_BEEN_MAPPED) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"target %d memory not mapped yet",head->target & 0xF );		
		return -1;
	}

	if(len > pmmapinfo->send.normalpackagelength) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"len %d larger than buffer %d",len,pmmapinfo->send.normalpackagelength);
		return PCI_NOR_SHARE_MEMORY_NOTENOUGH;
	}

	/* not used by chanjinn 20071017
	if(len == 0) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"len = %d",0);
		return 0;
	}*/

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"head->pri = %d",head->pri);	
	

	/* normal package */
	if(head->pri == HI3511_PCI_PACKAGE_PRI_LEVEL_1){
#ifndef 	CONFIG_PCI_HOST	
		unsigned long int_flags = 0;
#endif
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"write normal package %d",0);

		// get read write pointer from share memory
#ifdef 	CONFIG_PCI_HOST	
		readpointer = __raw_readl((char *)pmmapinfo->send.readpointeraddr);
		writepointer = __raw_readl((char *)pmmapinfo->send.writepointeraddr);
#else
		//local_irq_save(int_flags);

		/* lock dma */
		int_flags = down_interruptible(&pci_dma_sem);
		if(int_flags) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
			return -1;
		}
		
		hi3511_pci_dma_read(pmmapinfo->send.readpointeraddr,(settings_addr_physics+read_pointer_physics_r) , 4);
#if 0		
		control = (4<<8)| 0x61;

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave read the host readpointer address is 0x%lx \n",(unsigned long)pmmapinfo->send.readpointeraddr);

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.readpointeraddr);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics+read_pointer_physics_r));

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
#endif
		hi3511_pci_dma_read(pmmapinfo->send.readpointeraddr, slave_fifo_stuff_physics, 0x100);
#if 0
#if pci_dma_push_fifo
		control = (0x100<<8)| 0x61;

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.readpointeraddr);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, slave_fifo_stuff_physics);

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	

#endif
#endif
		readpointer=__raw_readl((volatile char *)(datarelay+read_pointer_physics_r));

		hi3511_pci_dma_read(pmmapinfo->send.writepointeraddr,  (settings_addr_physics+write_pointer_physics_r), 4);
#if 0		
		control = (0x4<<8)| 0x61;

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave read the host writepointer address is 0x%lx \n",(unsigned long)pmmapinfo->send.writepointeraddr);

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.writepointeraddr);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics+write_pointer_physics_r));

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
#endif
		hi3511_pci_dma_read( pmmapinfo->send.readpointeraddr, slave_fifo_stuff_physics, 0x100);
#if 0
#if pci_dma_push_fifo

		control = (0x100<<8)| 0x61;

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.readpointeraddr);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, slave_fifo_stuff_physics);

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
	
#endif		
#endif
		writepointer=__raw_readl((volatile char *)(datarelay+write_pointer_physics_r));			

		//local_irq_restore(int_flags);
		up(&pci_dma_sem);
#endif

		if(readpointer > writepointer) {
			
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read %d > write %d",readpointer,writepointer);

			if((len + SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7) ) <  (readpointer - writepointer)) {

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->send.normalpackageaddr + writepointer -headsize));
				//ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr + writepointer),buffer,len,flags);//20071126 61104
#ifdef CONFIG_PCI_HOST
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"base address phy is 0x%8lX, base address virt is %8lX", hi3511_mem_map[head->target-1+2].base_addr_phy, hi3511_mem_map[head->target-1+2].base_addr_virt);

				ret = hi3511_pci_writepack(head->target,hi3511_mem_map[head->target-1].base_addr_phy+(char *)pmmapinfo->send.normalpackageaddr + writepointer-hi3511_mem_map[head->target-1].base_addr_virt,buffer,len,flags);
#else
				ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr + writepointer),buffer,len,flags);
#endif
				if(ret <= 0){
					ret = -1;
					goto hi3511_pci_add2buffer_normal_releasenorlock;
				}

				/* enough space */
				// test memcpy first	
				//memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer -headsize ),head,headsize);
				//memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer -headsize ),head,headsize);
				// usr api
				hi3511_pci_write_normalhead(head,(void *)pmmapinfo,writepointer);

#ifndef CONFIG_PCI_HOST
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head has been writen ,((struct Hi3511_pci_data_transfer_head *)(datarelay+20))->reserved is %x writepointer %d", ((volatile struct Hi3511_pci_data_transfer_head *)(datarelay+20))->reserved,writepointer);		
#endif
				// change write pointer ,SHARE_MEMORY_128BIT_ORDERLINES means give package head space
				writepointer +=  len + SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7);
			}
			else	 {
				hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"share buffer full !!%d,writer pointer is 0x%8X,read pointer is 0x%8Xh",0,writepointer,readpointer);
				/*space not enough */
				ret = PCI_NOR_SHARE_MEMORY_FULL;
			}
			
		}
		//readpointer <= writepointer
		else {
					
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read %d <= write %d",readpointer,writepointer);

			/* buffer enough ?*/
			if((len +  SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7) ) >= (pmmapinfo->send.normalpackagelength - (writepointer - readpointer))) {
				/* package larger than buffer */
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"normal package larger than buffer %d",0);	

				ret = PCI_NOR_SHARE_MEMORY_FULL;

				goto hi3511_pci_add2buffer_normal_releasenorlock;
			}

			if( (writepointer + len) <= pmmapinfo->send.normalpackagelength ) {

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"write + len < pack len %d",0);

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->send.normalpackageaddr + writepointer -headsize));

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head target %d msg %d port %d length %d",head->target,head->msg,head->port,head->length);
				
		
				//ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr + writepointer),buffer,len,flags);
#ifdef CONFIG_PCI_HOST
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"base address phy is 0x%8lX, base address virt is %8lX", hi3511_mem_map[head->target-1].base_addr_phy, hi3511_mem_map[head->target-1].base_addr_virt);
				ret = hi3511_pci_writepack(head->target,hi3511_mem_map[head->target-1].base_addr_phy+(char *)pmmapinfo->send.normalpackageaddr + writepointer - hi3511_mem_map[head->target-1].base_addr_virt,buffer,len,flags);
#else
				ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr + writepointer),buffer,len,flags);
#endif
				if(ret <= 0){
					ret = -1;
					goto hi3511_pci_add2buffer_normal_releasenorlock;
				}
				
				/* remain memory enough in buttom */
				//memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer -headsize ),head,headsize);
				//memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer -headsize),head,headsize);
				// usr api

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"r<w, package has been writen ,head->reserved is %x",head->reserved);
				hi3511_pci_write_normalhead(head,(void *)pmmapinfo,writepointer);

#ifndef CONFIG_PCI_HOST
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"r<w, head has been writen ,((struct Hi3511_pci_data_transfer_head *)(datarelay+20))->reserved is %x", ((volatile struct Hi3511_pci_data_transfer_head *)(datarelay+20))->reserved);

#endif
				/* change write pointer ,SHARE_MEMORY_128BIT_ORDERLINES means give package head space */
				/*  if writepointer at the end of send memory,go to top */
				/*    need test here!*/
				if((writepointer + len + SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7))  < pmmapinfo->send.normalpackagelength){
					writepointer = writepointer + len + SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7);
					hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"writepointer %d",writepointer);
				}
				else	{
					hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"writepointer to %d",0);					
					writepointer = 0;
				}
				
			}
			else {

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"write + len > pack len %d",0);

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->send.normalpackageaddr + writepointer -headsize));

				/* first step */
				steplen1 = pmmapinfo->send.normalpackagelength- writepointer;

				steplen2 = len -steplen1;

				//ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr + writepointer),buffer,steplen1,flags);
#ifdef CONFIG_PCI_HOST
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"base address phy is 0x%8lX, base address virt is %8lX", hi3511_mem_map[head->target-1].base_addr_phy, hi3511_mem_map[head->target-1].base_addr_virt);
				ret = hi3511_pci_writepack(head->target,hi3511_mem_map[head->target-1].base_addr_phy+(char *)pmmapinfo->send.normalpackageaddr + writepointer -hi3511_mem_map[head->target-1].base_addr_virt,buffer,steplen1,flags);
#else
				ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr + writepointer),buffer,steplen1,flags);
#endif
				if(ret <= 0){
					ret = -1;
					goto hi3511_pci_add2buffer_normal_releasenorlock;
				}
				//ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr),(buffer + steplen1),steplen2,flags);
#ifdef CONFIG_PCI_HOST
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"base address phy is 0x%8lX, base address virt is %8lX", hi3511_mem_map[head->target-1].base_addr_phy, hi3511_mem_map[head->target-1].base_addr_virt);
				ret = hi3511_pci_writepack(head->target,hi3511_mem_map[head->target-1].base_addr_phy+(char *)pmmapinfo->send.normalpackageaddr - hi3511_mem_map[head->target-1].base_addr_virt,(buffer + steplen1),steplen2,flags);
#else
				ret = hi3511_pci_writepack(head->target,(char *)(pmmapinfo->send.normalpackageaddr),(buffer + steplen1),steplen2,flags);
#endif
				if(ret <= 0){
					ret = -1;
					goto hi3511_pci_add2buffer_normal_releasenorlock;
				}
				
				ret = len;
				
				/* remain memory not enough in buttom */
				//memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer -headsize ),head,headsize);
				//memcpy((char *)(pmmapinfo->send.normalpackageaddr + writepointer -headsize),head,headsize);
				// usr api
				hi3511_pci_write_normalhead(head,(void *)pmmapinfo,writepointer);

				// change write pointer ,SHARE_MEMORY_128BIT_ORDERLINES means give package head space
				writepointer = steplen2 + SHARE_MEMORY_128BIT_ORDERLINES - 	(steplen2&0x7);

			}
			
		}

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"write in writepointer %d",writepointer);		
	
#ifdef 	CONFIG_PCI_HOST	
		__raw_writel( writepointer, (char *)pmmapinfo->send.writepointeraddr);
#else
		//local_irq_save(int_flags);
		/* lock dma */
		int_flags = down_interruptible(&pci_dma_sem);
		if(int_flags) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
			return -1;
		}

		//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);

		//*(unsigned long *)datarelay = writepointer;
		__raw_writel( writepointer, (volatile char *)(datarelay+write_pointer_physics));

		hi3511_pci_dma_write(pmmapinfo->send.writepointeraddr, (settings_addr_physics+write_pointer_physics), 4);

#if 0		
		control = (4<<8)| 0x71;

		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmapinfo->send.writepointeraddr);

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_pointer_physics));

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); 	
#endif
		hi3511_pci_dma_write(host_fifo_stuff_physics, (settings_addr_physics+write_pointer_physics), 0x40);
#if 0
#if pci_dma_push_fifo

		control = (0x40<<8)| 0x71;

		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, host_fifo_stuff_physics);

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_pointer_physics));

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); 	

#endif
#endif 
		//local_irq_restore(int_flags);
		//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);		
		up(&pci_dma_sem);
#endif

		
	}// end of normal package	
	
hi3511_pci_add2buffer_normal_releasenorlock:	

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"writepointer  %d readpointer %d after add",writepointer,readpointer);
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_add2buffer_normal end %d",ret);

	return ret;
}

// write to package for DMA or not 
// not used yet
int hi3511_pci_writepack(int target,char *dst, char * src,int len,int flags)
{
	int ret = -1;
//	DECLARE_WAITQUEUE(wait, current);		

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_writepack %d",0);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"dest 0x%8lX",(unsigned long)dst);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"src 0x%8lX",(unsigned long)src);

	if(flags&0x4) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"dma write %d",0);
		while(hi3511_dma_check() != NOTBUSY);
		ret = 0;

		if(ret <= HI3511_PCI_WAIT_TIMEOUT){

			unsigned long int_flags;

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"memcpy Node write %d",len);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"ret %d",ret);
			
#ifdef CONFIG_PCI_HOST
			//pNode->write(dest,src,len,pNode->slot);
			//udelay(20);
			/* lock dma */
#if 1
			int_flags = down_interruptible(&pci_dma_sem);
			if(int_flags) {
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
				return -1;
			}

			hi3511_pci_dma_write((unsigned long)dst, (unsigned long)src,len);
			
			up(&pci_dma_sem);
#else
			unsigned int control;
			local_irq_save(int_flags);
			control = (len<<8)| 0x71;
		
			hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, dst);

			hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, src);

			hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

			while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
			local_irq_restore(int_flags);
#endif
	
#else
			// slave 
			//pNode->write(dest,src,len,0);
			//local_irq_save(int_flags);

			/* lock dma */
			int_flags = down_interruptible(&pci_dma_sem);
			if(int_flags) {
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
				return -1;
			}
			
			//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
			
			hi3511_pci_dma_write((unsigned long)dst,(unsigned long)src,len);
#if 0
		       control = (len<<8)| 0x71;
		
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave write the host address is 0x%lx \n",(unsigned long)dest);

			hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, dest);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"at the same time hi3511 slave ddr address is 0x%lx \n",(unsigned long)src);

			hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, src);

			hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

			while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif 		
			//local_irq_restore(int_flags);
			//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
			up(&pci_dma_sem);
#endif
			ret = len;
		}
		else{
			ret = -1;
		}
	}
	else{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"memcpy write 0x%d",len);
#ifdef CONFIG_PCI_HOST
hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"where is bug %d",1);

		memcpy(dst,src,len);
		
hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"where is bug %d",2);

		ret = len;
#else
          	printk("Only DMA should be used to write package on simple bridged device! \n");
		printk("Please set the third bit of transfer flag. \n");
		ret=-1;
#endif
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_writepack end %d",ret);

	return ret;
}

// write to package flage 1: DMA; 0: not 
int hi3511_pci_readpack(int target,char *dest, char * src,int len,int flags)
{
	struct hi3511_dev *pNode = hi3511_dev_head;
	signed long ret = -1;	
	DECLARE_WAITQUEUE(wait, current);

#ifdef CONFIG_PCI_HOST
	struct list_head *p;
	int i = 0;		

	list_for_each(p,&pNode->node){
		pNode = list_entry(p, struct hi3511_dev, node);	
		
		if(i == (target - 1))
			break;
		i++;
	}
#else
	/* hi3511_dev_head is a head */
	pNode = list_entry(hi3511_dev_head->node.next, struct hi3511_dev, node);

#endif

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readpack target %d flags %d",target,flags);

	if(flags&0x4){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"dma read %d",0);
		if(hi3511_dma_check() != NOTBUSY){
			
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"dma read busy %d",0);

			add_wait_queue_exclusive(&hi3511_pci_handle_table->dmawaitqueue,&wait);

			/* wait for dma ready */
			// sleep wait for data come wake up
			ret = interruptible_sleep_on_timeout(&hi3511_pci_handle_table->dmawaitqueue,HI3511_PCI_WAIT_TIMEOUT);
		}
		else{
			ret = 0;
		}

		if(ret <= HI3511_PCI_WAIT_TIMEOUT){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"memcpy Node read 0x%d",len);
			
#ifdef CONFIG_PCI_HOST
#ifndef CONFIG_PCI_TEST
			pNode->read(dest,src,len,pNode->slot);
#else
			// test don't have  dma
			memcpy(dest,src,len);
#endif
#else
			// slave don't have dma
			memcpy(dest,src,len);
#endif
			ret = len;
		}
		else{
			ret = -1;
		}
	
	}
	else{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"!!!!memcpy!!!!! dest 0x%8lX src 0x%8lX read 0x%d",(unsigned long)dest,(unsigned long)src,len);
		memcpy(dest,src,len);
		ret = len;
	}
	// waitqueue empty

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readpack end %ld",ret);
	
	return ret;
}

int hi3511_pci_read_priorityhead(struct Hi3511_pci_data_transfer_head *head,void * mmapinfo)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)mmapinfo ;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)mmapinfo;
#endif
	const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);

	memcpy(head, (char *)(pmmapinfo->recv.pripackageaddr - headsize),headsize);//memcpy(&head,(char *)(pmmapinfo->recv.pripackageaddr - headsize),headsize);	

	return 0;
}

// read from share memory
int hi3511_pci_readonepackfromsharebuffer_priority(void * handle,char *buffer, int flags)
{
	struct Hi3511_pci_data_transfer_handle *phandle = (struct Hi3511_pci_data_transfer_handle *)handle;
		
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[phandle->target  - 1];
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif

	struct Hi3511_pci_data_transfer_head  head;

	const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	unsigned short readable = 0;
	
	int ret = 0;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readonepackfromsharebuffer_priority%d",0);
	
	if(pmmapinfo->mapped != PCI_SLAVE_BEEN_MAPPED){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"target %d memory not mapped yet",head.target & 0xF );	
		return -1;
	}

	// syn mode by default. should modify asyn
	if(phandle->synflage == PCI_SHARE_MEMORY_RECV_SYN){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"syn mode %d",0);
		
		/* lock */
		ret = down_interruptible(&pmmapinfo->recv.prisemaphore);
		if(ret)
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt hi3511_pci_readfrombuffer %d",0);
			return -ERESTARTSYS;
		}
	}
	// asyn mode
	else
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",0);
		
		ret = down_trylock(&pmmapinfo->recv.prisemaphore);
		if(ret){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"share memory busy %d",0);
			return -1;
		}
	}
	
	readable = __raw_readw((char *)pmmapinfo->recv.pripackageflagaddr);
	
	// read the data in share memory
	// high PRI package read first 
	if(readable ==  PCI_READABLE_PRIPACKAGE_FLAG )
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pri readable %d",0);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->recv.pripackageaddr - headsize));

		//memcpy(&head, (char *)(pmmapinfo->recv.pripackageaddr - headsize),headsize);
		hi3511_pci_read_priorityhead(&head,(void *)pmmapinfo);

		if(head.reserved != 0x2D5A){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"some thing bad happened!%d" ,0);
			ret = -1;
			goto hi3511_pci_readonepackfromsharebuffer_priority_out;
		}
		
		// handle exsit
		if((head.src == (phandle->target & 0xF)) && ( head.msg == (phandle->msg & 0xF))
			&& ( head.port == (phandle->port & 0xF))){
			
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle exist target %d  src %d",head.target,head.src);

			// wait for DMA
			ret = hi3511_pci_readpack(head.src,buffer,(char *)pmmapinfo->recv.pripackageaddr,head.length,flags);
			if(ret <= 0){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);

				goto hi3511_pci_readonepackfromsharebuffer_priority_out;
			}
				
			//*(char *)pmmapinfo->recv.pripackageflagaddr = PCI_UNREADABLE_PRIPACKAGE_FLAG ;	
			__raw_writew(PCI_UNREADABLE_PRIPACKAGE_FLAG,(char *)pmmapinfo->recv.pripackageflagaddr);

			udelay(20);
			readable = __raw_readw((char *)pmmapinfo->recv.pripackageflagaddr);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"value of pripackage flag after pack read  %d" ,readable);
		}

	}
	else{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle not exist target %d  src %d",head.target,head.src);
	}
hi3511_pci_readonepackfromsharebuffer_priority_out:
	
	/* release lock */
	up(&pmmapinfo->recv.prisemaphore);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readonepackfromsharebuffer_priority end %d",0);
	
	return ret;	

}

int hi3511_pci_read_normalhead(struct Hi3511_pci_data_transfer_head *head,void *mmapinfo,int readpointer)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)mmapinfo;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)mmapinfo;
#endif 

	const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);

	memcpy(head,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),headsize);
	
	return 0;
}
int hi3511_pci_readonepackfromsharebuffer2handle_normal(int target,char *buffer, int length)
{

#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[target  - 1];
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif
	unsigned int step1 = 0,step2 = 0;
	int readpointer = 0,writepointer = 0;
	int packresidue = 0;
	int ret = 0;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readonepackfromsharebuffer2handle_normal %d",0);

	// read the data in share memory
	// get read / write pointer
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 host read the host readpointer address is 0x%lx \n",(unsigned long)pmmapinfo->recv.readpointeraddr);
	readpointer = __raw_readl((char *)pmmapinfo->recv.readpointeraddr);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 host read the host writepointer address is 0x%lx \n",(unsigned long)pmmapinfo->recv.writepointeraddr);
	writepointer = __raw_readl((char *)pmmapinfo->recv.writepointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"readpointer %d writepointer %d",readpointer,writepointer);

	if(readpointer < writepointer)
	{

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read %d < write %d",readpointer,writepointer);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle exist %d",length);
				
		ret = hi3511_pci_readpack(target,buffer,(char *)(pmmapinfo->recv.normalpackageaddr+readpointer) ,length,0);				
		if(ret < 0){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);

			goto hi3511_pci_readonepackfromsharebuffer2handle_normal_out;
		}
		
		// recv one pack readpointer change
		readpointer += length + SHARE_MEMORY_128BIT_ORDERLINES - (length&0x7);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle readpointer  %d",readpointer);
		
		// set read pointer
		__raw_writel(readpointer, (char *)pmmapinfo->recv.readpointeraddr);		
	}
	else if(readpointer > writepointer)
	{

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read >= write %d",0);

		packresidue = length & 0x7;

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle exist %d",0);

		// the whole read pointer to buttom 
		if(length <= (pmmapinfo->recv.normalpackagelength - readpointer))
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"length < packlen - readpointer %d",0);				

			ret = hi3511_pci_readpack(target,buffer,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer),length,0);				
			if(ret < 0){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);
	
				goto hi3511_pci_readonepackfromsharebuffer2handle_normal_out;
			}
			
			if(( readpointer + length + SHARE_MEMORY_128BIT_ORDERLINES - packresidue ) < pmmapinfo->recv.normalpackagelength){

				readpointer += length + SHARE_MEMORY_128BIT_ORDERLINES - packresidue;

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"readpointer keep to bottom %d\n",0);		
			}
			else
			{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read pointer return to head %d\n",0);		

				readpointer = 0;
			}
		}
		else
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"two step %d",0);				

			step1 = pmmapinfo->recv.normalpackagelength - readpointer;

			step2 = length - step1;

			ret = hi3511_pci_readpack(target,buffer,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer),step1,0);
			if(ret < 0){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);
	
				goto hi3511_pci_readonepackfromsharebuffer2handle_normal_out;
			}
			ret = hi3511_pci_readpack(target,(buffer+step1),(char *)(pmmapinfo->recv.normalpackageaddr),step2,0);
			if(ret < 0){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);
	
				goto hi3511_pci_readonepackfromsharebuffer2handle_normal_out;
			}

			ret = length;

			readpointer = step2 + SHARE_MEMORY_128BIT_ORDERLINES - (step2&0x7);				
		}
		ret = length;
		//}

		// set read pointer
		__raw_writel(readpointer, (char *)pmmapinfo->recv.readpointeraddr);			
			
	}
	else	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read = write %d",0);

		ret = 0;
	}

hi3511_pci_readonepackfromsharebuffer2handle_normal_out:
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readonepackfromsharebuffer2handle_normal end %d",ret);
	
	return ret;		
}


// read from share memory
int hi3511_pci_readonepackfromsharebuffer_normal(void * handle,char *buffer, int flags)
{

#ifdef CONFIG_PCI_HOST
	struct Hi3511_pci_data_transfer_handle *phandle = (struct Hi3511_pci_data_transfer_handle *)handle;
	struct slave2host_mmap_info *pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[phandle->target  - 1];
#else
	struct slave_mmap_info *pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;  // slave just have one mmapinfo
#endif 

	struct Hi3511_pci_data_transfer_head  head;		
	static const int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	unsigned int step1 = 0,step2 = 0;
	int readpointer = 0,writepointer = 0;
	int packagelen = 0;
	int bufferleft = 0;
	int packresidue = 0;
	int ret = 0;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readonepackfromsharebuffer_normal %d",0);

	/*
	if(!pmmapinfo->mapped)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"target %d memory not mapped yet",head.target & 0xF );
		return -1;
	}*/

	// syn mode by default. should modify asyn

// all syn or asyn not use by chanjinn 20071017
//	if(!phandle->synflage)
//	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->synflage syn %d",0);

		/* lock */
		ret = down_interruptible(&pmmapinfo->recv.normalsemaphore);
		if(ret)
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt hi3511_pci_readfrombuffer %d",0);
			return -ERESTARTSYS;
		}
//	}
	// asyn mode
	/*
	else
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->synflage asyn %d",0);
		
		ret = down_trylock(&pmmapinfo->recv.normalsemaphore);
		if(ret)
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"share memory busy %d",0);
			return -1;
		}
	}*/

	// read the data in share memory
	// get read / write pointer
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 host read the host readpointer address is 0x%lx \n",(unsigned long)pmmapinfo->recv.readpointeraddr);
	readpointer = __raw_readl((char *)pmmapinfo->recv.readpointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 host read the host writepointer address is 0x%lx \n",(unsigned long)pmmapinfo->recv.writepointeraddr);
	writepointer = __raw_readl((char *)pmmapinfo->recv.writepointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"readpointer %d writepointer %d",readpointer,writepointer);

	if(readpointer < writepointer)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read %d < write %d",readpointer,writepointer);

//		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize));		

		// head size small,should use directly
		//memcpy(&head,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),headsize);
		hi3511_pci_read_normalhead(&head,(void *)pmmapinfo,readpointer);

		if(head.reserved != 0x2D5A){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"some thing bad happened!readpointer %d writepointer %d head size %d reserved %d" ,readpointer,writepointer,head.length,head.reserved);
			ret = -1;
			goto hi3511_pci_readfrombuffer_out;
		}
	
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle exist %d",head.length);
				
		ret = hi3511_pci_readpack(head.src,buffer,(char *)(pmmapinfo->recv.normalpackageaddr+readpointer) ,head.length,flags);				
		if(ret < 0){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);
	
			goto hi3511_pci_readfrombuffer_out;
		}

		// should set head 0 after read chanjinn 20071016
		memset((char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),0,headsize);
		
		// recv one pack readpointer change
		readpointer += head.length + SHARE_MEMORY_128BIT_ORDERLINES - (head.length&0x7);

		// set read pointer
		__raw_writel(readpointer, (char *)pmmapinfo->recv.readpointeraddr);	
		
	}
	else if(readpointer > writepointer)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read >= write %d",0);

//		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize));		

		//memcpy(&head,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),headsize);
		//memcpy(&head,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),headsize);
		hi3511_pci_read_normalhead(&head,(void *)pmmapinfo,readpointer);

		if(head.reserved != 0x2D5A){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"some thing bad happened!readpointer %d writepointer %d head size %d reserved %d" ,readpointer,writepointer,head.length,head.reserved);
			ret = -1;
			goto hi3511_pci_readfrombuffer_out;
		}

		// recvie one package if handle right,else give it up
/*		if((head.target == (phandle->target & 0xF)) && ( head.msg == (phandle->msg & 0xF))
			&& ( head.port == (phandle->port & 0xF)))
		{*/
		packagelen = head.length;
		packresidue = head.length & 0x7;

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle exist %d",0);
		
		bufferleft = pmmapinfo->recv.normalpackagelength - readpointer;
		
		// the whole read pointer to buttom 
		if(packagelen <= bufferleft)
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"the whole read pointer to buttom pack %d left %d",packagelen,bufferleft);

			ret = hi3511_pci_readpack(head.src,buffer,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer ),head.length,flags);				
			if(ret < 0){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"hi3511_pci_readpack error %d" ,ret);
	
				goto hi3511_pci_readfrombuffer_out;
			}

			// should set head 0 after read
			memset((char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),0,headsize);

			if(( readpointer + packagelen + SHARE_MEMORY_128BIT_ORDERLINES - packresidue ) < pmmapinfo->recv.normalpackagelength){
				readpointer += packagelen + SHARE_MEMORY_128BIT_ORDERLINES - packresidue;
			}
			else
			{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read pointer return to head %d\n",0);		
				readpointer = 0;
			}
		}
		//
		else
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"two step %d",0);				
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"normalpackagelength %d",pmmapinfo->recv.normalpackagelength);

			step1 = pmmapinfo->recv.normalpackagelength - readpointer;
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"step1 %d",step1);	
			
			step2 = packagelen - step1;
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"step2 %d",step2);

			hi3511_pci_readpack(head.src,buffer,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer ),step1,flags);

			hi3511_pci_readpack(head.src,(buffer+step1),(char *)(pmmapinfo->recv.normalpackageaddr),step2,flags);

			// should set head 0 after read
			memset((char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),0,headsize);

			readpointer = step2 + SHARE_MEMORY_128BIT_ORDERLINES - (step2&0x7);				
		}				
		ret = head.length;			
		//}

		// set read pointer
		__raw_writel(readpointer, (char *)pmmapinfo->recv.readpointeraddr);			
			
	}
	else	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read = write %d",0);

		ret = 0;

		goto hi3511_pci_readfrombuffer_out;
	}
hi3511_pci_readfrombuffer_out:

	/* release lock */
	up(&pmmapinfo->recv.normalsemaphore);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_readonepackfromsharebuffer_normal end %d",ret);
	
	return ret;	
}


void hi3511_pci_getrecv_priority_datain(void * data)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)data;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)data;
	struct Hi3511_pci_initial_host2pci init_package;
#endif

	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	struct Hi3511_pci_data_transfer_handle *phandle;	
	struct Hi3511_pci_data_transfer_head head;
	unsigned int prireadable = 0;
	int target,msg,port = 0;
	int handle_pos = 0;
	int ret = 0;
	int lock = 0;
	int len = 0;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getrecv_priority_datain %d",0);
	
	// get PRI info 
	/* lock */
	lock = down_interruptible(&pmmapinfo->recv.prisemaphore);
	if(lock){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt hi3511_pci_readfrombuffer PRI package %d",0);
		return;
	}
	
	// PRI data check
	prireadable = __raw_readw((char *)pmmapinfo->recv.pripackageflagaddr);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pripackageflagaddr %d",prireadable);
	if(!prireadable){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pri not readable %d",0);
		/* release PRI share memory recv lock */
		up(&pmmapinfo->recv.prisemaphore);
		goto hi3511_pci_getrecv_priority_datain_out;
	}

//	memset(&head,0,sizeof(struct Hi3511_pci_data_transfer_head));
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->recv.pripackageaddr - headsize));

	//head = __raw_readl((char *)(pmmapinfo->recv.pripackageaddr - headsize));
	memcpy(&head,(char *)(pmmapinfo->recv.pripackageaddr - headsize), headsize);
	
#ifdef  CONFIG_PCI_TEST
	target = head.target;
#else
	target = head.src;
#endif

	msg = head.msg;
	port = head.port;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pri %d src %d msg %d port %d",head.pri,target,msg,port);

	if(head.reserved != 0x2D5A){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"some thing bad happened! %d" ,0);
		/* release PRI share memory recv lock */
		up(&pmmapinfo->recv.prisemaphore);		
		goto hi3511_pci_getrecv_priority_datain_out;
	}

// for slave initial buffer
#ifndef CONFIG_PCI_HOST
	// initial slave memory flage
	if(head.pri ==  0x3)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave init mem %d",0);
		
		// read from priority buffer
		memcpy( &init_package,(char *)pmmapinfo->recv.pripackageaddr,sizeof(struct Hi3511_pci_initial_host2pci));

		// host recv means slave send, host send means slave recv
		//ret = hi3511_init_sharebuf(init_package.recvbuffersize,init_package.sendbuffersize, init_package.recvbufferaddrhost, SHARE_MEMORY_SETSIZEFLAG);	

		hi3511_pci_vdd_info.id = init_package.slaveid;

		hi3511_pci_vdd_info.bus_slot = init_package.slot;
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"init package slot %ld",(unsigned long)hi3511_pci_vdd_info.bus_slot);

		hi3511_pci_vdd_info.mapped = 1;
		
		hi3511_slave_interrupt_flag = init_package.interrupt_flag_addr;

		__raw_writew(PCI_UNREADABLE_PRIPACKAGE_FLAG,(char *)pmmapinfo->recv.pripackageflagaddr);

		/* release PRI share memory recv lock */
		up(&pmmapinfo->recv.prisemaphore);

		goto hi3511_pci_getrecv_priority_datain_out;
	}
#endif

	handle_pos = Hi3511_pci_handle_pos(target , msg ,port);

	phandle = hi3511_pci_handle_table->handle[handle_pos];
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle_pos  %d",handle_pos);	
	if(phandle == NULL){
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle NULL %d",0);		
		// not handle , give up data
		//*(char *)pmmapinfo->recv.pripackageflagaddr = PCI_UNREADABLE_PRIPACKAGE_FLAG;
		__raw_writew(PCI_UNREADABLE_PRIPACKAGE_FLAG,(char *)pmmapinfo->recv.pripackageflagaddr);

		/* release PRI share memory recv lock */
		up(&pmmapinfo->recv.prisemaphore);
		
		goto hi3511_pci_getrecv_priority_datain_out;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle exist %d",0);		

	// lock handle
	lock = down_interruptible(&phandle->recvsemaphore);
	if(lock){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);

		/* release PRI share memory recv lock */
		up(&pmmapinfo->recv.prisemaphore);
		
		goto hi3511_pci_getrecv_priority_datain_out;
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->synflage %d",phandle->synflage);
	// syn mode
	if(phandle->synflage == PCI_SHARE_MEMORY_RECV_SYN)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"syn mode %d",0);
		
		// wake up sleep thread
		if(waitqueue_active(&phandle->waitqueuehead) != 0){			

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"wait queue not empyt %d",0);

			phandle->recvflage = PCI_SHARE_MEMORY_DATA_IN_PRIORITY;
			
			//wait queue not empty
			// release handle for recv
			up(&phandle->recvsemaphore);

			/* release PRI share memory recv lock */
			up(&pmmapinfo->recv.prisemaphore);
	
			wake_up(&phandle->waitqueuehead);

			//msleep_interruptible((unsigned int)10);sleep to wait write priority,other write thread should wait for this by lock func
			yield();// for short wait~~				
			/*
			lock = down_interruptible(&pmmapinfo->recv.prisemaphore);
			if(lock) {
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrput %d",0);
				return ;
			}*/
		}
		else
		{
			// for warning
			len = head.length;
			
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"share to handle %d",len);
			// waitqueue empty
			// cp data from share memory to handle buffer
			ret = hi3511_pci_recvfrom_prioritysharebuf2handlebuf((void *)pmmapinfo,(void *)phandle,len);	

			if(ret < 0){
				atomic_add(1,&lostpacknum);

				/* release PRI share memory recv lock */
				up(&phandle->recvsemaphore);
				up(&pmmapinfo->recv.prisemaphore);	
			
				hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"give up package! totel %d now",lostpacknum.counter);
				goto hi3511_pci_getrecv_priority_datain_out;
			}
			
			// PRI data check
			//*(char *)pmmapinfo->recv.pripackageflagaddr = PCI_UNREADABLE_PRIPACKAGE_FLAG;
			__raw_writew(PCI_UNREADABLE_PRIPACKAGE_FLAG,(char *)pmmapinfo->recv.pripackageflagaddr);

			/* release handle */
			up(&phandle->recvsemaphore);

			/* release PRI share memory recv lock */
			up(&pmmapinfo->recv.prisemaphore);	
		}
		
	}
	// asyn mode
	else
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",0);

		// warning!!
		// pci_vdd_notifier_recvfrom should call pci_vdd_recvfrom when used
		// or NULL function
		if(NULL != phandle->pci_vdd_notifier_recvfrom){
			// should wait if recvflage is PCI_SHARE_MEMORY_DATA_IN??
			phandle->recvflage = PCI_SHARE_MEMORY_DATA_IN_PRIORITY;

			/* release handle lock */
			up(&phandle->recvsemaphore);

			/* release PRI share memory recv lock */
			up(&pmmapinfo->recv.prisemaphore);
		
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",1);

			phandle->pci_vdd_notifier_recvfrom(phandle, &head);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",2);

			//for tranfic control 20071124
#ifdef CONFIG_PCI_HOST
			master_to_slave_irq(pmmapinfo->slot);
#else
			slave_to_master_irq(0);
#endif

			lock = down_interruptible(&pmmapinfo->recv.prisemaphore);
			if(lock) {
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrput %d",3);
				return ;
			}
		}
		else{
			// PRI data check
			//*(char *)pmmapinfo->recv.pripackageflagaddr = PCI_UNREADABLE_PRIPACKAGE_FLAG;
			__raw_writew(PCI_UNREADABLE_PRIPACKAGE_FLAG,(char *)pmmapinfo->recv.pripackageflagaddr);
		}
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",5);

		/* release PRI share memory recv lock */
		up(&pmmapinfo->recv.prisemaphore);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",6);

	}
	
hi3511_pci_getrecv_priority_datain_out:
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getrecv_priority_datain end %d",0);	

	return;

}

//53us
void hi3511_pci_getrecv_normal_datain(void *data)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)data;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)data;
#endif
	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	struct Hi3511_pci_data_transfer_handle *phandle;	
	struct Hi3511_pci_data_transfer_head head;
	unsigned int readpointer = 0, writepointer = 0;	
	//unsigned int prereadpointer = 0;
	int handle_pos = 0;
	int lock = 0;
	int ret = 0;
	int len = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getrecv_normal_datain %d",0);	

	// get PRI info 
	/* lock */
	lock = down_interruptible(&pmmapinfo->recv.normalsemaphore);
	if(lock) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
		return ;
	}
	
	// read / write data check
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 host read the host readpointer address is 0x%lx \n",(unsigned long)pmmapinfo->recv.readpointeraddr);
	readpointer = __raw_readl((char *)pmmapinfo->recv.readpointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 host read the host writepointer address is 0x%lx \n",(unsigned long)pmmapinfo->recv.writepointeraddr);		
	writepointer = __raw_readl((char *)pmmapinfo->recv.writepointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"readpointer %d writepointer %d",readpointer,writepointer);

	while( readpointer != writepointer ){		

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"normal data in while %d",0);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head addr 0x%8lXh", (unsigned long)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize));
	
		memcpy(&head,(char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),headsize);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head len %d msg %d port %d",head.length,head.msg,head.port);

		if(head.reserved != 0x2D5A){

			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"some thing bad happened! readpointer 0x%8lX writepointer 0x%8lX head size %d" ,(unsigned long)pmmapinfo->recv.readpointeraddr,(unsigned 	long)pmmapinfo->recv.writepointeraddr,head.length);

			ret = -1;

			/* release PRI lock */
			up(&pmmapinfo->recv.normalsemaphore);	

			goto hi3511_pci_getrecv_normal_datain_out;
		}
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"__raw_readsb head %d",0);

		handle_pos = Hi3511_pci_handle_pos(head.src, head.msg, head.port);

		phandle = hi3511_pci_handle_table->handle[handle_pos];

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle position %d",handle_pos);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle 0x%lx",(unsigned long)phandle);

		if(NULL == phandle){

			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"handle null %d",handle_pos);

			// handle empty give up this package
			readpointer = hi3511_pci_get_pointernextpos((void *)pmmapinfo,head.length,readpointer,writepointer);

			__raw_writel(readpointer,(char *)pmmapinfo->recv.readpointeraddr);

			atomic_add(1,&lostpacknum);
			
			continue ;
		}

		// lock handle
		lock = down_interruptible(&phandle->recvsemaphore);
		if(lock){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrput %d",0);

			/* release PRI lock */
			up(&pmmapinfo->recv.normalsemaphore);	

			goto hi3511_pci_getrecv_normal_datain_out;
		}

		// syn mode
		if(phandle->synflage == PCI_SHARE_MEMORY_RECV_SYN)
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"syn mode %d",0);

			// wake up sleep thread
			if(waitqueue_active(&phandle->waitqueuehead) != 0)
			{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"wait queue not empyt %d",0);
				// wait queue not empty

				phandle->recvflage = PCI_SHARE_MEMORY_DATA_IN_NORMAL;	
				
				// release handle for recv
				up(&phandle->recvsemaphore);

				/* release mmap lock */
				up(&pmmapinfo->recv.normalsemaphore);

				wake_up(&phandle->waitqueuehead);

				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"yield() %d",0);
				
				//msleep_interruptible((unsigned int)10);sleep to wait write priority,other write thread should wait for this by lock func
				yield();// for short wait~~

				goto hi3511_pci_getrecv_normal_datain_out;

			}
			else	{
				
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"wait queue empty %d",0);

				// for warning
				len = head.length;
			
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"share to handle len %d",len);


				// copy data from share memory to handle buffer
				ret = hi3511_pci_recvfrom_normalsharebuf2handlebuf((void *)pmmapinfo,phandle,readpointer,head.length);
				if(ret < 0){
					atomic_add(1,&lostpacknum);
			
					// release handle for recv
					up(&phandle->recvsemaphore);			

					/* release PRI share memory recv lock */
					up(&pmmapinfo->recv.normalsemaphore);	

					hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"give up package! totel %d now",lostpacknum.counter);

					goto hi3511_pci_getrecv_normal_datain_out;
				}
				
				atomic_add(1,&readpack_num);

				// should set head 0 after read
				memset((char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),5,headsize);

				readpointer = hi3511_pci_get_pointernextpos(pmmapinfo,head.length,readpointer,writepointer);

				__raw_writel(readpointer,(char *)pmmapinfo->recv.readpointeraddr);
		
				// release handle for recv
				up(&phandle->recvsemaphore);

				/* release PRI share memory recv lock */
				//up(&pmmapinfo->recv.normalsemaphore);	chanjinn 20071016  free handle sem not free buffer sem

			}
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"syn readpointer = %d writepointer %d after read",readpointer,writepointer);

		}
		// asyn mode
		else
		{	
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",0);

			// warning!!
			// pci_vdd_notifier_recvfrom should call pci_vdd_recvfrom when used
			// or NULL function
			if(NULL != phandle->pci_vdd_notifier_recvfrom){
				// should wait if recvflage is PCI_SHARE_MEMORY_DATA_IN??
				phandle->recvflage = PCI_SHARE_MEMORY_DATA_IN_NORMAL;
			
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",1);	

				// record package size for next readpointer position
				len = head.length;
			
				/* release handle lock */
				up(&phandle->recvsemaphore);

				//readpointer = hi3511_pci_get_pointernextpos(pmmapinfo,head.length,readpointer,writepointer); chanjinn 20071016
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",2);	

				/* release mmap lock */
				up(&pmmapinfo->recv.normalsemaphore);
				
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn mode %d",3);

				phandle->pci_vdd_notifier_recvfrom(phandle, &head);
	
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"readpointer after asyn recv %d",readpointer);

				atomic_add(1,&readpack_num);
			
				lock = down_interruptible(&pmmapinfo->recv.normalsemaphore);
				if(lock){
				
					hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrput %d",0);

					goto hi3511_pci_getrecv_normal_datain_out;
				}

				readpointer = hi3511_pci_get_pointernextpos(pmmapinfo,len,readpointer,writepointer);
				
			}
			else{
				// should set head 0 after read 
				memset((char *)(pmmapinfo->recv.normalpackageaddr + readpointer - headsize),0x4,headsize);

				readpointer = hi3511_pci_get_pointernextpos(pmmapinfo,len,readpointer,writepointer);				

				__raw_writel(readpointer,(char *)pmmapinfo->recv.readpointeraddr);
			}

			//readpointer = __raw_readl((char *)pmmapinfo->recv.readpointeraddr);	
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"asyn readpointer = %d writepointer %d after read",readpointer,writepointer);

		}
			
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"after read readpointer %d writepointer %d",readpointer,writepointer);

	}

	/* release PRI lock */
	up(&pmmapinfo->recv.normalsemaphore);	

hi3511_pci_getrecv_normal_datain_out:


	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"readpointer = %d writepointer %d  head.reserved %d length %d after read",readpointer,writepointer,head.reserved,head.length);

#ifndef  CONFIG_PCI_TEST
	readpointer = __raw_readl((char *)pmmapinfo->recv.readpointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"readpointer = %d writepointer %d after read",readpointer,writepointer);
#endif

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getrecv_normal_datain end %d",0);
	return;
}


// 37us
// copy data from normal share memory to handle buffer
int hi3511_pci_recvfrom_normalsharebuf2handlebuf(void*mmapinfo,
	struct Hi3511_pci_data_transfer_handle *handle,int info_readpoint, int len)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)mmapinfo;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)mmapinfo;
#endif
	struct Hi3511_pci_data_transfer_handle *phandle = handle;
	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	int handlebufferreadpointer = phandle->buffer->readpointer;
	int handlebufferwritepointer = phandle->buffer->writepointer;
	struct Hi3511_pci_data_transfer_head head;
	int info_writepoint = 0;
	int ret = -1;
	int setpost = len + SHARE_MEMORY_128BIT_ORDERLINES -(len&0x7);
	int writepos = 0;

	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_recvfrom_normalsharebuf2handlebuf %d",len);

	// get info read write pointer
	info_writepoint = __raw_readl((char *)pmmapinfo->recv.writepointeraddr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"info_writepoint %d",info_writepoint);
	
	if(len > HI3511_PCI_HANDLE_BUFFER_SIZE){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"handle buffer not enough %d",0);
		return -1;
	}

#ifdef _FOR_TIME_
	do_gettimeofday(&tv4);
#endif

	/* lock */
	if(down_interruptible(&phandle->buffer->write)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"phandle->buffer->write lock error %d",0);
		return -ERESTARTSYS;
	}
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"before recvie wp %d rp %d",handlebufferwritepointer,handlebufferreadpointer);

	head.length = len;
	head.reserved = 0;
	
	// buffer enough
	// handlebufferwritepointer >= handlebufferreadpointer
	if( handlebufferwritepointer >= handlebufferreadpointer)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handlebufferwritepointer %d >= handlebufferreadpointer %d",handlebufferwritepointer,handlebufferreadpointer);

		if((handlebufferwritepointer + len + SHARE_MEMORY_128BIT_ORDERLINES) <= (HI3511_PCI_HANDLE_BUFFER_SIZE)){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"wp %d + len %d < size %d",handlebufferwritepointer,len,(int)HI3511_PCI_HANDLE_BUFFER_SIZE);
			writepos = handlebufferwritepointer;
		}
		else{
			if(len < (handlebufferreadpointer - SHARE_MEMORY_128BIT_ORDERLINES )){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read pointer %d < len %d",handlebufferreadpointer,len);
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"set write pointer place flag %d",0);

				/* set poison value */
				head.reserved = 0x2AD5;
//				head.notused = 0xA55A;
				
				memcpy((char *)(phandle->buffer->buffer + handlebufferwritepointer - headsize), &head, headsize);
				
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"set wp to top %d",0);

				writepos = 0;
				head.reserved = 0;
//				head.notused = 0;
			}
			else{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"len %d > handlebufferreadpointer %d",len,handlebufferreadpointer);
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle mem not enough %d",0);
				ret = -1;
				goto hi3511_pci_recvfrom_normalsharebuf2handlebuf_out;
			}
		}			
	}
	//handlebufferwritepointer < handlebufferreadpointer
	else {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handlebufferwritepointer %d < handlebufferreadpointer %d",handlebufferwritepointer ,handlebufferreadpointer);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"setpost %d",setpost);
			
		if((handlebufferwritepointer + setpost) <  handlebufferreadpointer){
			writepos = handlebufferwritepointer;
		}
		else{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"len %d > handlebufferreadpointer - handlebufferwritepointer %d",len,(handlebufferreadpointer - handlebufferwritepointer));
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle mem not enough %d",0);
			ret = -1;
			goto hi3511_pci_recvfrom_normalsharebuf2handlebuf_out;
		}
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"writepos %d",writepos);

	// copy share memory data to buffer
	ret = hi3511_pci_readonepackfromsharebuffer2handle_normal(phandle->target,(char *) (phandle->buffer->buffer + writepos),len);
	if(ret >= 0){
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"head len %d",head.length);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"head 0x%8lX",(unsigned long)(phandle->buffer->buffer + writepos - headsize));
		// set the head to the pack top
		memcpy((char *)(phandle->buffer->buffer + writepos - headsize), &head, headsize);		
	
		phandle->packnum++;

		phandle->packsize += ret;

		phandle->buffer->writepointer = writepos + setpost;

		hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"read one normal package %d",phandle->packnum);		

		hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"phandle->packsize  %d",phandle->packsize);

	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"after recv handle wr %d rd %d",phandle->buffer->writepointer,phandle->buffer->readpointer);	

hi3511_pci_recvfrom_normalsharebuf2handlebuf_out:
	
	/* unlock */
	up(&phandle->buffer->write);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_recvfrom_normalsharebuf2handlebuf end %d",0);	
	return ret;
}


// copy data from normal share memory to handle buffer
int hi3511_pci_recvfrom_prioritysharebuf2handlebuf(void *data,void*handle, int len)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info *)data;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info *)data;
#endif

	struct Hi3511_pci_data_transfer_handle *phandle = (struct Hi3511_pci_data_transfer_handle *) handle;
	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	int handlebufferreadpointer = phandle->buffer->readpointer;
	int handlebufferwritepointer = phandle->buffer->writepointer;
	struct Hi3511_pci_data_transfer_head head;
	int info_writepoint = 0;
	int ret = -1;
	int writepos = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_recvfrom_prioritysharebuf2handlebuf %d",ret);

	// get info read write pointer
	info_writepoint = (int ) *(char *)pmmapinfo->recv.writepointeraddr;

	// buffer enough?
	if(phandle->packsize >= PCI_HANDLE_MEM_MAXSIZE){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"handle buffer full %d",phandle->packsize);
		atomic_add(1,&lostpacknum);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"give up package! totel %d now",lostpacknum.counter);	
		return -1;
	}

	if(len > PCI_HANDLE_MEM_MAXSIZE){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"handle buffer not enough %d",len);
		return -1;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->buffer->write lock %d",0);

	/* lock */
	if(down_interruptible(&phandle->buffer->write)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"phandle->buffer->write lock error %d",0);
		return -ERESTARTSYS;
	}

	head.length = len;
	
	// buffer enough
	// handlebufferwritepointer >= handlebufferreadpointer
	if( handlebufferwritepointer >= handlebufferreadpointer)
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handlebufferwritepointer >= handlebufferreadpointer %d",0);

		if((handlebufferwritepointer + len) <= (HI3511_PCI_HANDLE_BUFFER_SIZE - SHARE_MEMORY_128BIT_ORDERLINES)){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"wp %d + len %d < size %d",handlebufferwritepointer,len,(int)HI3511_PCI_HANDLE_BUFFER_SIZE);
			writepos = handlebufferwritepointer;
		}
		else{

			if(len <= (handlebufferreadpointer - SHARE_MEMORY_128BIT_ORDERLINES  + (handlebufferreadpointer&0x7))){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read pointer %d < len %d",handlebufferreadpointer,len);
				
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"set write pointer place flag %d",0);

				/* set poison value */
				head.reserved = 0x2AD5;
//				head.notused = 0xA55A;
				memcpy((char *)(phandle->buffer->buffer + handlebufferwritepointer - headsize), &head, headsize);
				
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"set wp to top %d",0);

				writepos = 0;
				head.reserved = 0;
//				head.notused = 0;	
			}
			else{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"len %d > handlebufferreadpointer %d",len,handlebufferreadpointer);
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"mem not enough %d",0);
				ret = -1;
				goto hi3511_pci_recvfrom_prioritysharebuf2handlebuf_out;
			}
		}
	}
	//handlebufferwritepointer < handlebufferreadpointer
	else{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handlebufferwritepointer < handlebufferreadpointer %d",0);
		if((handlebufferwritepointer + len + SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7)) <= handlebufferreadpointer){
			writepos = handlebufferwritepointer;
		}
		else{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"len %d > handlebufferreadpointer - handlebufferwritepointer %d",len,(handlebufferreadpointer - handlebufferwritepointer));
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"mem not enough %d",0);
			ret = -1;
			goto hi3511_pci_recvfrom_prioritysharebuf2handlebuf_out;
		}
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"buff + writepos - headsize 0x%8lX",(unsigned long)(phandle->buffer->buffer + writepos - headsize));

	// set the head to the top
	memcpy((char *)(phandle->buffer->buffer + writepos - headsize), &head, headsize);		

	up(&pmmapinfo->recv.prisemaphore);

	// copy share memory data to buffer
	ret = hi3511_pci_readonepackfromsharebuffer_priority((void *)phandle,(char *)(phandle->buffer->buffer + writepos), 0);

	if(down_interruptible(&pmmapinfo->recv.prisemaphore))
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt hi3511_pci_readfrombuffer %d",0);

		return -ERESTARTSYS;
	}
		
	if(ret >= 0){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read pri package %d",ret);
		
		phandle->packnum++;

		phandle->packsize += ret;
		
		phandle->buffer->writepointer = writepos + len + SHARE_MEMORY_128BIT_ORDERLINES - (len&0x7);
	}
	else	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"read pri failure %d",ret);
	}
	hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"after recv handle wr %d rd %d",phandle->buffer->writepointer,phandle->buffer->readpointer);
hi3511_pci_recvfrom_prioritysharebuf2handlebuf_out:

	/* unlock */
	up(&phandle->buffer->write);
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_recvfrom_prioritysharebuf2handlebuf end %d",ret);	

	return ret;
	
}

// wrpointer means pointer to be change 
// rpointer read only
int hi3511_pci_get_pointernextpos(void *data,int packlen,int wrpointer, int rpointer)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo = (struct slave2host_mmap_info * )data;
#else
	struct slave_mmap_info *pmmapinfo = (struct slave_mmap_info * )data;
#endif 

	int rescue = packlen & 0x7;
	int writelen = wrpointer + packlen + SHARE_MEMORY_128BIT_ORDERLINES - rescue;
	int retpos = writelen;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_get_pointernextpos %d",0);	
	
	// not handle , give up data
	if(wrpointer >= rpointer) {
		if((wrpointer + packlen) <= pmmapinfo->recv.normalpackagelength){
			if( writelen >= pmmapinfo->recv.normalpackagelength){
				retpos = 0;
			}
		}
		else	{
			retpos = packlen - (pmmapinfo->recv.normalpackagelength - wrpointer) + (int)SHARE_MEMORY_128BIT_ORDERLINES  - rescue;
		}
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_get_pointernextpos pos = %d",retpos);		

	return retpos;
}

int hi3511_pci_recvfromhandlebuffer(struct Hi3511_pci_data_transfer_handle *handle,char *buffer)
{
	struct Hi3511_pci_data_transfer_handle *phandle = handle;
	int handlereadpointer = phandle->buffer->readpointer;
	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	struct Hi3511_pci_data_transfer_head head;
	int prehandlereadpointer = 0;
	int ret = -1;
//	int i = 0;
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_recvfromhandlebuffer %d",0);

	/* lock */
	if(down_interruptible(&phandle->buffer->read))
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"phandle->buffer->write lock error %d",0);
		return -ERESTARTSYS;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"before recv handle wr %d rd %d",phandle->buffer->writepointer,phandle->buffer->readpointer);
	
	// get head
	memcpy(&head,(char *)(phandle->buffer->buffer + handlereadpointer - headsize),headsize);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"head length %d",head.length);
	
	/* if head is a flag to top */
	if(head.reserved == 0x2AD5){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"next buffer at top %d",head.length);
		prehandlereadpointer = handlereadpointer;
		handlereadpointer = 0;
		memcpy(&head,(char *)(phandle->buffer->buffer - headsize),headsize);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"head 0x%8lX",(unsigned long)(phandle->buffer->buffer - headsize));
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"head at top %d",head.length);
	}
	
	memcpy(buffer,(char *)(phandle->buffer->buffer + handlereadpointer),head.length);

	memset((char *)(phandle->buffer->buffer + prehandlereadpointer - headsize),0,headsize);

	phandle->packnum --;

	phandle->packsize -= head.length;

	ret = head.length;

	phandle->buffer->readpointer = handlereadpointer + head.length + SHARE_MEMORY_128BIT_ORDERLINES - (head.length&0x7);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"after recv handle wr %d rd %d",phandle->buffer->writepointer,phandle->buffer->readpointer);

	up(&phandle->buffer->read);	

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_recvfromhandlebuffer end %d",0);

	return  ret;	
}

// wake up 30us ,work 63 us
int hi3511_pci_kernelthread_deamon(void *data)
{
#ifdef CONFIG_PCI_HOST
	struct slave2host_mmap_info *pmmapinfo ;
	int mask;
#else
	struct slave_mmap_info *pmmapinfo;
#endif
	static DEFINE_SPINLOCK(irq_lock);
	struct hi3511_dev *pNode;
	struct list_head *node;
	int i = 0;
	int exitflag = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_kernelthread_deamon begin %d",0);

	do{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_kernelthread_deamon begin %d",irq_num.counter);

// for slave to host pci irq
#ifdef CONFIG_PCI_HOST

		for(i = 0; i < HI3511_PCI_SLAVE_NUM; i++ ){
			mask = atomic_read(&host2slave_irq[i]);
			if(mask){
				pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[i];				
				atomic_set(&host2slave_irq[i], 1);
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"master_to_slave_irq slot %d",pmmapinfo->slot);
				master_to_slave_irq(pmmapinfo->slot);
			}
		}
		i = 0;
#else
		// slave set host pci  irq 
		// we modified code here 
		// pci dma resource should locked by semlock in thread enviroment
		// by chanjinn -- c58721
		if(atomic_read(&slave2host_irq)){

			atomic_set(&slave2host_irq,0);

			slave_to_master_irq(0);		
		}
#endif
		if(!atomic_read(&irq_num)){
		 
			spin_lock_irq(&irq_lock);
			 hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"irq_num before hi3511_pci_kernelthread_deamon being scheduled %d",irq_num.counter);
	
			 // set current sleep	 
			 set_current_state(TASK_INTERRUPTIBLE);
			 spin_unlock_irq(&irq_lock);
			 schedule();
		 }else{
   
			 hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"before irq_num set 0 %d",irq_num.counter);
			 atomic_set(&irq_num,0);
			 //atomic_dec(&irq_num);			
	 	}

		list_for_each(node, &hi3511_dev_head->node){
			pNode = list_entry(node, struct hi3511_dev, node);
				
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_kernelthread_deamon while %d",pNode->slot);

#ifdef CONFIG_PCI_HOST
			pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[i];
#else
			pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;
#endif	
			if(pmmapinfo->mapped != 1){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"not mapped %d",atomic_read(&readpack_num));
				goto hi3511_pci_kernelthread_deamon_sleep;
			}

			hi3511_pci_getrecv_priority_datain((void *)pmmapinfo);

			hi3511_pci_getrecv_normal_datain((void *)pmmapinfo);

			i++ ;
		}
			
		i = 0;

hi3511_pci_kernelthread_deamon_sleep:
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"sleep... %d",0);

		exitflag = atomic_read(&threadexitflag);
		
		if(!exitflag)
			break;

#ifdef CONFIG_PCI_HOST
//		ENABLE_INT_HI_SLOT(slot_type);
#else
		slave_int_enable();
#endif

	}while(1);	

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"recv thread exit %d",0);

	return 0;
}

void hi3511_mapped_info(unsigned long data)
{
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1, "map time out %d",0);
	printk("Hi3511 slave in slot: ");
	if(hi3511_pci_vdd_info.mapped & 0x10)
		printk("slot0 ");
	if(hi3511_pci_vdd_info.mapped & 0x20)
		printk("slot1 ");
	if(hi3511_pci_vdd_info.mapped & 0x40)
		printk("slot2 ");
	if(hi3511_pci_vdd_info.mapped & 0x80)
		printk("slot3 ");
	printk("\n");
	hi3511_pci_vdd_info.mapped = 1;
}

#ifdef CONFIG_PCI_HOST

// when interrupt initial here 
//@param resetflag  means re-set buffer size
// interrupt should know which slot interrupt
// return 0 means success; -1 means failure;others means been mapped
int hi3511_init_sharebuf(int slot,unsigned long sendbufsize,unsigned long recvbufsize, int resetflag)
{
	struct Hi3511_pci_slave2host_mmap_info *pmmap_info = &Hi3511_slave2host_mmap_info;
	unsigned long sharesize = 0 ;	
	unsigned long host_recv_buffer_physics,host_recv_buffer_virt;
	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	struct Hi3511_pci_data_transfer_head;
	struct Hi3511_pci_initial_host2pci;
	struct hi3511_dev *pNode = NULL;
	struct list_head *node;	
	int ret  = -1;
	int i = 0;

	// for no warning
	//setup the receive buffer in HOST ddr 
	host_recv_buffer_physics = host_sharebuffer_start;
	host_recv_buffer_virt = (unsigned long)host_recv_buffer;
	//host_recv_buffer_end=host_recv_buffer+host_sharebuffer_size;
	
	// find the slave in slot, set it mem
	list_for_each (node, &hi3511_dev_head->node)
	{
		pNode = list_entry(node, struct hi3511_dev, node);

		slot  = pNode->slot; 
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"ShareSize 0x%8lXh memstart 0x%8lXh memend 0x%8lXh buffersize 0x%8lXh", sharesize, pNode->pf_base_addr,pNode->pf_end_addr,(pNode->pf_end_addr - pNode->pf_base_addr));					
		//if( pNode->slot == slot )
		{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slot = %d",pNode->slot);	

			sharesize = recvbufsize ;	

			/* should not over load */
			if( sharesize >  (pNode->pf_end_addr - pNode->pf_base_addr)){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"slot %d set share memory size 0x%8lx which overload",slot,sharesize);
				ret = PCI_SLAVE_MAP_OVERLOAD;
			}
			/* buffer should larger than two high priority package , wirte pointer, read pointer (recv and send)*/
			else if(sharesize <=  PCI_HIGHPRI_PACKAGE_SIZE +  SHARE_MEMORY_128BIT_ORDERLINES )
			{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"slot%d set share memory size %ld which not enough",slot,sharesize);
				ret = PCI_SLAVE_MAP_NOTENOUGH;
			}
			
			else if( ((pmmap_info->mmapinfo[i].mapped & 0xF) != PCI_SLAVE_BEEN_MAPPED)
					|| ( (resetflag & SHARE_MEMORY_SETSIZEFLAG) == SHARE_MEMORY_SETSIZEFLAG) )
			{
			
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"slot %d ShareSize 0x%8lXh memstart 0x%8lXh memend 0x%8lXh",slot, sharesize, pNode->pf_base_addr,pNode->pf_end_addr);
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"send size 0x%8lX recv size 0x%08lX",sendbufsize,recvbufsize);

				/* High PRI data head need remain 2 bytes and data memory should 64 bits orderlines */
 				/*	
 				      0____________________________ send pri
					|___________________________|
					|___________________________|
					|___________________________|
					|___________________________|recv pri
					|___________________________|
					|___________________________|
					|___________________________|
				     7|___________________________|send normal 
				     8|___________________________|
					|___________________________|
					|___________________________|
					|___________________________|
					|___________________________|recv normal
					|___________________________|
					|___________________________|
				   15|___________________________|
					|___________________________|
				*/
				
				pmmap_info->mmapinfo[i].send.baseaddr =  pNode->pf_base_addr ;
				//pmmap_info->mmapinfo[i].send.baseaddr_physics =  host_recv_buffer_physics ;

				/* set recv share buffer info */
				// recv pri addr and flag
				pmmap_info->mmapinfo[i].send.pripackageaddr = pNode->pf_base_addr + SHARE_MEMORY_128BIT_ORDERLINES - (pNode->pf_base_addr & 0x7);

				//memset((void *) pNode->pf_base_addr ,0, sendbufsize);
				
				/* pri wirte/read flag up head 2 bytes*/
				pmmap_info->mmapinfo[i].send.pripackageflagaddr  = pmmap_info->mmapinfo[i].send.pripackageaddr - headsize - 2;

				//*(char *)pmmap_info->mmapinfo[i].send.pripackageflagaddr  = PCI_WRITEABLE_PRIPACKAGE_FLAG;
			//	__raw_writew(PCI_WRITEABLE_PRIPACKAGE_FLAG,(char *)pmmap_info->mmapinfo[i].send.pripackageflagaddr);
			//	udelay(20);

#ifdef  CONFIG_PCI_TEST
				// here for cycle test in one board 
				// send and receive memory in the same place 
				pmmap_info->mmapinfo[i].recv.pripackageaddr = pmmap_info->mmapinfo[i].send.pripackageaddr;
#else
				// recv pri addr and flag
				// release
				pmmap_info->mmapinfo[i].recv.pripackageaddr =  (unsigned long)(host_recv_buffer_virt+SHARE_MEMORY_128BIT_ORDERLINES - ((unsigned long)host_recv_buffer_virt & 0x7));
#endif
				/* pri wirte/read flag up head 1 byte*/
				pmmap_info->mmapinfo[i].recv.pripackageflagaddr  = pmmap_info->mmapinfo[i].recv.pripackageaddr - headsize - 2;
				__raw_writew(PCI_WRITEABLE_PRIPACKAGE_FLAG,(char *)pmmap_info->mmapinfo[i].recv.pripackageflagaddr);//c61104 20071122

				// send normal			
				pmmap_info->mmapinfo[i].send.buflength = sendbufsize;

				/* read pointer just under PRI package end 1 byte */
				pmmap_info->mmapinfo[i].send.readpointeraddr = pmmap_info->mmapinfo[i].send.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + PCI_READ_POINTER_EXCURSION;
				
				/* read pointer just under PRI package end 2 bytes */
				pmmap_info->mmapinfo[i].send.writepointeraddr = pmmap_info->mmapinfo[i].send.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + PCI_WRITE_POINTER_EXCURSION;

				/* change the pointer it can */
				//*(char *)pmmap_info->mmapinfo[i].send.writepointeraddr = 0;
			//	__raw_writel(0,(char *)pmmap_info->mmapinfo[i].send.writepointeraddr);

				/* normal package begin at PRI package addr + 16 byte */
				pmmap_info->mmapinfo[i].send.normalpackageaddr = pmmap_info->mmapinfo[i].send.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + SHARE_MEMORY_128BIT_ORDERLINES; 

				pmmap_info->mmapinfo[i].send.normalpackagelength = sendbufsize - PCI_HIGHPRI_PACKAGE_SIZE - 2*SHARE_MEMORY_128BIT_ORDERLINES;
	
				/* send share memory semaphore */
				init_MUTEX(&pmmap_info->mmapinfo[i].send.prisemaphore);
				
				init_MUTEX(&pmmap_info->mmapinfo[i].send.normalsemaphore);			

				pmmap_info->mmapinfo[i].recv.buflength = recvbufsize;
				

				/* read pointer just under PRI package end 1 byte */
				pmmap_info->mmapinfo[i].recv.readpointeraddr = pmmap_info->mmapinfo[i].recv.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE+ PCI_READ_POINTER_EXCURSION;

				/* write pointer just under PRI package end 2 bytes */
				pmmap_info->mmapinfo[i].recv.writepointeraddr = pmmap_info->mmapinfo[i].recv.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + PCI_WRITE_POINTER_EXCURSION;

				/* change the pointer it can */
				//*(char *)pmmap_info->mmapinfo[i].recv.readpointeraddr = 0;
				__raw_writel(0, (char *)pmmap_info->mmapinfo[i].recv.readpointeraddr);


				/* normal package begin at PRI package addr + 8 byte */
				pmmap_info->mmapinfo[i].recv.normalpackageaddr = pmmap_info->mmapinfo[i].recv.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + SHARE_MEMORY_128BIT_ORDERLINES;  

				pmmap_info->mmapinfo[i].recv.normalpackagelength = recvbufsize - PCI_HIGHPRI_PACKAGE_SIZE - 2*SHARE_MEMORY_128BIT_ORDERLINES;

				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"recv normal lenth %d",pmmap_info->mmapinfo[i].recv.normalpackagelength);

				/* recv share memory semaphore */
				init_MUTEX(&pmmap_info->mmapinfo[i].recv.prisemaphore);
				
				init_MUTEX(&pmmap_info->mmapinfo[i].recv.normalsemaphore);
				
				// set mapped flag
				pmmap_info->mmapinfo[i].mapped = PCI_SLAVE_BEEN_MAPPED;

				//pmmap_info->mmapinfo[i].slot = slot;
				pmmap_info->mmapinfo[i].slot = pNode->slot;

				pmmap_info->mmapinfo[i].dev = pNode;

				ret = 0;

				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"send initial slot %d info",pNode->slot);
#if 0
				/* set head */
				// send a high priority package to slave know the slot num, the mem size , the send addr  and the recv addr 
				// i use memcpy here
#ifdef  CONFIG_PCI_TEST
				//for test pri initial
				pri_head.target = slot;
#else
				pri_head.target = i + 1;
#endif
				/* host to slave initial package */
				pri_head.src = 0;

				pri_head.pri = 0x3;

				pri_head.length = sizeof(struct Hi3511_pci_initial_host2pci);

				pri_head.reserved = 0x2D5A;
				
				/* set initial data from host to slave*/
				/* host send means slave recv */
				init_package.recvbuffersize = sendbufsize;

				init_package.sendbuffersize = recvbufsize;

				init_package.slaveid = i + 1;

				init_package.recvbufferaddrhost=host_recv_buffer_physics;

				//init_package.slot = slot;
				init_package.slot = pNode->slot;

				/* copy head to share memory*/
				//memcpy((char *)(pmmap_info->mmapinfo[i].send.pripackageaddr  - headsize),&pri_head,headsize);
				memcpy((char *)(pmmap_info->mmapinfo[i].send.pripackageaddr  - headsize),&pri_head,headsize);
				
				memcpy((char *)pmmap_info->mmapinfo[i].send.pripackageaddr, &init_package, pri_head.length);

				// set pri memory been writen flag
				//*(char *)pmmap_info->mmapinfo[i].send.pripackageflagaddr = PCI_UNWRITEABLE_PRIPACKAGE_FLAG;
				__raw_writew(PCI_UNWRITEABLE_PRIPACKAGE_FLAG, (char *)pmmap_info->mmapinfo[i].send.pripackageflagaddr);
				udelay(20);
#endif
				__raw_writel( host_recv_buffer_physics, pNode->np_base_addr + 0x54);

				printk("addr %8lx value %8lx\n",pNode->np_base_addr + 0x54,host_recv_buffer_physics);
				
				host_recv_buffer_virt += (recvbufsize + fifo_stuff_size);
				host_recv_buffer_physics += (recvbufsize + fifo_stuff_size);

				// host send ok!enable slave recv interrupt
				//ENABLE_INT_HI_SLOT(slot);
//				master_to_slave_irq(pmmap_info->mmapinfo[i].slot);

				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"send initial info end %d",pmmap_info->mmapinfo[i].slot);

				hi3511_pci_vdd_info.slave_map =  hi3511_pci_vdd_info.slave_map | ( 1 << pNode->slot );

				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"vdd slave map %ld",hi3511_pci_vdd_info.slave_map);

				ret = 0;
				
			}
			else	{
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"been mapped %d",0);
				ret = PCI_SLAVE_BEEN_MAPPED;
			}
			break;
		}
		i++;
	}
	
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_init_sharebuf end ret = %d",ret);	
	return ret;
	
}


void hi3511_pci_host2slave_irq(unsigned long target)
{
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_host2slave_irq %d",Hi3511_slave2host_mmap_info.mmapinfo[target - 1].slot);

	atomic_set(&host2slave_irq[target], 1);

	wake_up_process(precvthread);

	return;
}

#else

// slave
// when interrupt initial here 
//@param resetflag  means re-set buffer size
// interrupt should know which slot interrupt
// return 0 means success; -1 means failure;others means been mapped
int hi3511_init_sharebuf(unsigned long sendbufsize,unsigned long recvbufsize, unsigned long sendbufaddr, int resetflag)
{
	struct hi3511_dev *pNode = list_entry(hi3511_dev_head->node.next, struct hi3511_dev, node);

	struct slave_mmap_info *pmmap_info = &Hi3511_slave_mmap_info.mmapinfo;

	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	// find the slave in slot, set it mem
	unsigned long sharesize = recvbufsize ;			

	unsigned long int_flags;
	
	int ret  = -1;
	host_fifo_stuff_physics = sendbufaddr + PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_init_sharebuf %d",0);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"ShareSize 0x%8lXh memstart 0x%8lXh memend 0x%8lXh", sharesize, pNode->pf_base_addr,pNode->pf_end_addr);			

	/* should not over load */
	if( pNode->pf_base_addr +  sharesize >  pNode->pf_end_addr) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"set share memory size %ld which overload",sharesize);
		ret = PCI_SLAVE_MAP_OVERLOAD;
	}

	/* buffer should larger than two high priority package , wirte pointer, read pointer (recv and send)*/
	if(sharesize <= PCI_HIGHPRI_PACKAGE_SIZE +  SHARE_MEMORY_128BIT_ORDERLINES )
	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"set share memory size %ld which not enough",sharesize);
		ret = PCI_SLAVE_MAP_NOTENOUGH;
	}

	if( (pmmap_info->mapped != PCI_SLAVE_BEEN_MAPPED)
				|| ( (resetflag & SHARE_MEMORY_SETSIZEFLAG) == SHARE_MEMORY_SETSIZEFLAG) )
	{

		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"send size 0x%8lX recv size 0x%8lX",sendbufsize,recvbufsize);
	//	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pripackageaddr 0x%8lX",(unsigned long)pmmap_info->recv.pripackageaddr);
		/* High PRI data head need remain 2 bytes and data memory should 64 bits orderlines */
		/*	
		      0____________________________ base addr
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________| ---\
		     7|___________________________| ---/  head data needs 2 words  
		     8|___________________________| -->  comm data
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________|
			|___________________________|
		   15|___________________________|
			|___________________________|
		*/
		
//		pmmap_info->recv.baseaddr =  pNode->pf_base_addr +  SHARE_MEMORY_128BIT_ORDERLINES - (pNode->pf_base_addr & 0x7);

		// set recv pri
		pmmap_info->recv.pripackageaddr = pNode->pf_base_addr +  SHARE_MEMORY_128BIT_ORDERLINES - (pNode->pf_base_addr & 0x7);
		pmmap_info->recv.pripackageflagaddr  = pmmap_info->recv.pripackageaddr - headsize - 2;
		pmmap_info->recv.buflength = recvbufsize;

#ifdef  CONFIG_PCI_TEST
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"here is test!recv and send in one buffer recv 0x%8lX",(unsigned long)pmmap_info->recv.pripackageaddr);

		// here for cycle test in one board 
		// send and receive memory in the same place 
		pmmap_info->send.pripackageaddr = pmmap_info->recv.pripackageaddr;

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"here is test!recv and send in one buffer send 0x%8lX",(unsigned long)pmmap_info->send.pripackageaddr);
#else
		// set send pri
		//pmmap_info->send.baseaddr= sendbufaddr;
		pmmap_info->send.baseaddr_physics = sendbufaddr;
		pmmap_info->send.pripackageaddr = pmmap_info->send.baseaddr_physics + SHARE_MEMORY_128BIT_ORDERLINES;
#endif
/* pri wirte/read flag up head 1 byte*/
		pmmap_info->send.pripackageflagaddr  = pmmap_info->send.pripackageaddr - headsize - 2;

		//memset((void *) (pmmap_info->send.pripackageaddr - SHARE_MEMORY_128BIT_ORDERLINES), 0, sendbufsize);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave sendbufsize is 0x%lx \n",(unsigned long)sendbufsize);

		//local_irq_save(int_flags);

		/* lock dma */
		int_flags = down_interruptible(&pci_dma_sem);
		if(int_flags) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
			return -1;
		}

		//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);

		memset(datarelay+send_buffer_clear, 0, sendbufsize -send_buffer_clear);

		hi3511_pci_dma_write((pmmap_info->send.pripackageaddr - SHARE_MEMORY_128BIT_ORDERLINES), 
					(settings_addr_physics+send_buffer_clear),
					(settings_buffer_size-send_buffer_clear));
#if 0			
		control = ((settings_buffer_size-send_buffer_clear)<<8)| 0x71;
		
		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmap_info->send.pripackageaddr - SHARE_MEMORY_128BIT_ORDERLINES);

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+send_buffer_clear));

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif

		//__raw_writew(PCI_WRITEABLE_PRIPACKAGE_FLAG,(char *)pmmap_info->send.pripackageflagaddr);
		__raw_writew(PCI_WRITEABLE_PRIPACKAGE_FLAG,(volatile char *)(datarelay+pci_writeable_pripackage_flag));

		hi3511_pci_dma_write(pmmap_info->send.pripackageflagaddr, (settings_addr_physics+pci_writeable_pripackage_flag), 0x2);
#if 0
		control = (2<<8)| 0x71;
		
		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmap_info->send.pripackageflagaddr );

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+pci_writeable_pripackage_flag));

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif
		//local_irq_restore(int_flags);
		//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
		up(&pci_dma_sem);

		// set recv normal 
		/* read pointer just under PRI package end 1 byte */
		pmmap_info->recv.readpointeraddr = pmmap_info->recv.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE  + PCI_READ_POINTER_EXCURSION;

		/* read pointer just under PRI package end 2 bytes */
		pmmap_info->recv.writepointeraddr = pmmap_info->recv.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE  + PCI_WRITE_POINTER_EXCURSION;

		/* change the pointer it can */
		//*(char *)pmmap_info->mmapinfo[i].send.writepointeraddr = 0;
		__raw_writel(0,(char *)pmmap_info->recv.readpointeraddr);
	
		/* normal package begin at PRI package addr + 16 byte */
		pmmap_info->recv.normalpackageaddr = pmmap_info->recv.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE  + SHARE_MEMORY_128BIT_ORDERLINES; 

		pmmap_info->recv.normalpackagelength = recvbufsize - PCI_HIGHPRI_PACKAGE_SIZE - 2*SHARE_MEMORY_128BIT_ORDERLINES;

		/* send share memory semaphore */
		init_MUTEX(&pmmap_info->recv.prisemaphore);

		init_MUTEX(&pmmap_info->recv.normalsemaphore);

		pmmap_info->send.buflength = sendbufsize;

		/* read pointer just under PRI package end 1 byte */
		pmmap_info->send.readpointeraddr = pmmap_info->send.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + PCI_READ_POINTER_EXCURSION;

		/* write pointer just under PRI package end 2 bytes */
		pmmap_info->send.writepointeraddr = pmmap_info->send.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE + PCI_WRITE_POINTER_EXCURSION;

		/* change the pointer it can */
		//*(char *)pmmap_info->recv.readpointeraddr = 0;
		
		//printk("9 \n"); 
		//__raw_writel(0, (char *)pmmap_info->send.writepointeraddr);
		__raw_writel(0,(datarelay+write_pointer_physics));

		//local_irq_save(int_flags);

		/* lock dma */
		int_flags = down_interruptible(&pci_dma_sem);
		if(int_flags) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
			return -1;
		}

		//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
		hi3511_pci_dma_write(pmmap_info->send.writepointeraddr, 
								(settings_addr_physics+write_pointer_physics), 0x4);
#if 0		
		control = (4<<8)| 0x71;
		
		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pmmap_info->send.writepointeraddr);

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+write_pointer_physics));

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif
		//local_irq_restore(int_flags);
		//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);
		up(&pci_dma_sem);

		//printk("10 \n"); 
				
		/* normal package begin at PRI package addr + 8 byte */
		pmmap_info->send.normalpackageaddr = pmmap_info->send.pripackageaddr + PCI_HIGHPRI_PACKAGE_SIZE  + SHARE_MEMORY_128BIT_ORDERLINES;

		pmmap_info->send.normalpackagelength = sendbufsize -  PCI_HIGHPRI_PACKAGE_SIZE - 2*SHARE_MEMORY_128BIT_ORDERLINES;

		/* send share memory semaphore */
		init_MUTEX(&pmmap_info->send.prisemaphore);
				
		init_MUTEX(&pmmap_info->send.normalsemaphore);

		// set mapped flag
		pmmap_info->mapped = PCI_SLAVE_BEEN_MAPPED;

		pmmap_info->dev = pNode;
		
		ret = 0;

	}
	else	{
		ret = PCI_SLAVE_BEEN_MAPPED;
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_init_sharebuf end ret = %d",ret);
	return ret;
	
}
void hi3511_pci_slave2host_irq(unsigned long data)
{
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_slave2host_irq %d",0);

	atomic_set(&slave2host_irq,1);
	
	wake_up_process(precvthread);
}

#endif
EXPORT_SYMBOL(hi3511_init_sharebuf);

