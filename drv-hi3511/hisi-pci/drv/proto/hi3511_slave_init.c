/* 
*hi3511_slave.c: A hi3511 low level 
*Copyright 2006 hisilicon Corporation 
*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <linux/mii.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/crc32.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <asm/processor.h>      /* Processor type for cache alignment. */
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>	/* User space memory access functions */
#include "datacomm.h"
#include "proto.h"
#include "../include/hi3511_pci.h"
#include "../../include/pci_vdd.h"

#ifndef CONFIG_PCI_HOST

#define  BASEADDRFORPCI 0x00
#define  SIZEADDRFORPCI 0x00
#define  SHARMEMSTARTOFFSET 0x00
#define  SHARMEMENDOFFSET 0x00

//#define  HI3511_SLAVE_INT (1<<5)
//#define  HI3511_SLAVE_INIT_INT (1<<4)


struct hi3511_dev hi3511_device_head;
EXPORT_SYMBOL(hi3511_device_head);
extern struct pci_vdd_info hi3511_pci_vdd_info;


extern void *datarelay;
extern unsigned long settings_addr_physics;
extern void *slave_recv_buffer;
extern unsigned long host_fifo_stuff_physics;
extern unsigned long hi3511_slave_interrupt_flag;
extern struct semaphore pci_dma_sem;

int hi3511_slave_interrupt_check(void)
{
	unsigned int status;
	status=PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3;
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"int status is %x \n",status);
	if(interrupt_check(status)){
	if(hi3511_bridge_ahb_readl(CPU_ISTATUS)&(PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3)) 
		return HI_HOST;
	}
	return -1;
}

//slave to master irq
void slave_to_master_irq(int slot)
{
	unsigned int flag = 0;

	/* lock dma */
	flag = down_interruptible(&pci_dma_sem);
	if(flag) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
		return ;
	}
	hi3511_pci_dma_read(hi3511_slave_interrupt_flag,(settings_addr_physics+irq_prepost_r),4);
#if 0
	control = (4<<8)| 0x61;
		
	/*use hi3511_interrupt_flag buffer on host to specify which slot interrupting, c61104*/
	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, hi3511_slave_interrupt_flag);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"at the same time hi3511 slave ddr address is 0x%lx \n",(unsigned long)settings_addr_physics);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics+irq_prepost_r));

	hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);
#endif

	hi3511_pci_dma_read(hi3511_slave_interrupt_flag,slave_fifo_stuff_physics,0x100);
#if 0
	control = (0x100 <<8)| 0x61;
		
	/*use hi3511_interrupt_flag buffer on host to specify which slot interrupting, c61104*/
	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, hi3511_slave_interrupt_flag);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"at the same time hi3511 slave ddr address is 0x%lx \n",(unsigned long)settings_addr_physics);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, slave_fifo_stuff_physics);

	hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);
#endif

	flag=*(volatile unsigned long *)(datarelay+irq_prepost_r);
	/* free dma*/
	up(&pci_dma_sem);
	
	if (flag == 0xdadadada){
	// some problem may happened here
	// set flag when flag = 0xdadadada
	// but all slaves use this same flag address
		writel((hi3511_pci_vdd_info.bus_slot|(((~hi3511_pci_vdd_info.mapped)&0x1)<<4)),datarelay+irq_prepost);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_vdd_info.bus_slot is %lx",hi3511_pci_vdd_info.bus_slot);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_vdd_info.mapped is %lx",hi3511_pci_vdd_info.mapped);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_vdd_info.bus_slot is %lx",(hi3511_pci_vdd_info.bus_slot|(((~hi3511_pci_vdd_info.mapped)&0x1)<<4)));

		/* lock dma */
		flag = down_interruptible(&pci_dma_sem);
		if(flag) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_dma_sem interrupt %d",0);
			return ;
		}

		//pci_dma_lock_test(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);	

		hi3511_pci_dma_write(hi3511_slave_interrupt_flag,(settings_addr_physics+irq_prepost),4);

#if 0
	 control = (4<<8)| 0x71;

	 /*use hi3511_interrupt_flag buffer on host to specify which slot interrupting, c61104*/
	 hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, hi3511_slave_interrupt_flag);

	 hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"at the same time hi3511 slave ddr address is 0x%lx \n",(unsigned long)settings_addr_physics);

	 hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+irq_prepost));

	 hi3511_bridge_ahb_writel(WDMA_CONTROL, control);
	 
         while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);
#endif

		hi3511_pci_dma_write(host_fifo_stuff_physics, (settings_addr_physics+irq_prepost) ,0x40);

		//pci_dma_lock_free(HI3511_PCI_VIRT_BASE + CPU_CIS_PTR);

		/* free dma*/
		up(&pci_dma_sem);
#if 0
#if pci_dma_push_fifo
	 control = (0x40<<8)| 0x71;

	 hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, host_fifo_stuff_physics);

	 hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, (settings_addr_physics+irq_prepost));

	 hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	 while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);			 
#endif
#endif
		//slave set it's cpu door bell
		hi3511_bridge_ahb_writel(CPU_ICMD,0x00f00000);
	}
}

void slave_int_disable(void)
{
	//hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)&0xfff0ffff));
//c61104
	hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)&0xfff0f0f0));

}

void slave_int_clear(void)
{
	//hi3511_bridge_ahb_writel(CPU_ISTATUS,0x000f0000);
	//61104
	hi3511_bridge_ahb_writel(CPU_ISTATUS,0x000f0f0f);

}

void slave_int_enable(void)
{
	hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)|0x000f0000));
}

//init the device descript structure for up level application.
void init_hi3511_slave_device(void)
{
	//init slave address register,PCIAHB_ADDR PCIAHB_SIZ(on AHB side)

	//hi3511_bridge_ahb_writel(PCIAHB_ADDR_PF,BASEADDRFORPCI);
	//hi3511_bridge_ahb_writel(PCIAHB_SIZ_PF,SIZEADDRFORPCI);
	struct hi3511_dev *hi_dev_slave = NULL;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"init_hi3511_slave_device begin %d",0);

	hi_dev_slave = (struct hi3511_dev *)kmalloc(sizeof(struct 	hi3511_dev),GFP_KERNEL);
	if(hi_dev_slave == NULL){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"vmalloc error %d",0);	
		return ;
	}
	hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"hi_dev_slave 0x%8lX",(unsigned long)hi_dev_slave);
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"slave-func %d",0);
	
	memset(hi_dev_slave,0,sizeof(struct hi3511_dev));

	INIT_LIST_HEAD(&hi3511_device_head.node);
	
	INIT_LIST_HEAD(&hi_dev_slave->node);	
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"hi_dev_slave 0x%8lX",(unsigned long)hi_dev_slave);
	
	//init share area
	hi_dev_slave->trigger=slave_to_master_irq;

	//hi_dev_slave->pf_base_addr = (unsigned long)ioremap(0xe5000000, 0x800000);//delete for dynamic alloc
	hi_dev_slave->pf_base_addr = (unsigned long)slave_recv_buffer;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"pf_base_addr is 0x%8lX",(unsigned long)hi_dev_slave->pf_base_addr);
	
	//hi_dev_slave->pf_end_addr = hi_dev_slave->pf_base_addr+0x800000;
	hi_dev_slave->pf_end_addr = hi_dev_slave->pf_base_addr+PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"pf_end_addr is 0x%8lX",(unsigned long)hi_dev_slave->pf_end_addr);

	list_add(&hi_dev_slave->node,&hi3511_device_head.node);

	//interrupt_enable(CPU_IMASK,0x0F000000);
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"init_hi3511_slave_device end %d",0);

	return;
}

void hi3511_slave_device_cleanup(void)
{
	struct hi3511_dev *pNode = NULL;
	struct list_head *node;
//	interrupt_disable(CPU_IMASK,0x0F000000);
	list_for_each(node,&hi3511_device_head.node)
	{
		pNode = list_entry(node,struct hi3511_dev,node);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pNode 0x%8lX",(unsigned long)pNode);
		iounmap((char *)pNode->pf_base_addr);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"iounmap %d",0);
		list_del(&pNode->node);
		break;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"vfree 0x%8lX",(unsigned long)pNode);
	kfree(pNode);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_slave_device_cleanup 0x%8lX",(unsigned long)pNode);
	return;
}

int hi3511_dma_check(void)
{
	return NOTBUSY;
}

#endif
