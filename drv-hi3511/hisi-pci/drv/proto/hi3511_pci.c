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
#include <linux/init.h>/*
 * I2C_2_GPIO.
 * Copyright (c)  DaoGuang Wu <wdgvip@gmail.com>
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
 
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>

#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_reg.h>
#include <linux/circ_buf.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
//#include <linux/platform_device.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kthread.h>

#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <asm/uaccess.h>

#include <linux/moduleparam.h>

#include <asm/uaccess.h>

#include <asm/arch/gpio.h>
#include <asm/arch/platform.h>
#include <asm/arch/yx_gpiodrv.h>
#include<asm/arch/yx_iomultplex.h>




#include <linux/i2c.h>
#include <linux/i2c-id.h>

#include "xr20m1172.h"
#include <linux/debug.h>
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
//#include <asm-arm/arch-hi3510_v100_p01/media-mem.h>
#include "../include/hi3511_pci.h"
#include "../../include/pci_vdd.h"
#include "datacomm.h"
#include "proto.h"

#ifdef  CONFIG_PCI_TEST

struct hi3511_dev hi3511_device_head;


int hi3511_interrupt_check()
{
#ifdef CONFIG_PCI_HOST
	return HI_SLOT1;
#else
	return HI_HOST;
#endif
}

//#ifdef CONFIG_PCI_HOST
int hi3511_dma_check()
{
	static int i = 0;
	i++;
	if(i%3 == 0){
		return NOTBUSY;
	}
	else if(i%3 == 1){
		return READBUSY;
	}
	else{	
		return WRITEBUSY;
	}
}

void slave_to_master_irq(struct pci_dev * pci_dev)
{
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave_to_master_irq %d",0);
#ifdef CONFIG_PCI_HOST
	pci_host_irq_handle(0,NULL,NULL);
#else
	pci_slave_irq_handle(0,NULL,NULL);
#endif
	return ;
}
void master_to_slave_irq(int slot)
{
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"irq slot %d",slot);
#ifdef CONFIG_PCI_HOST
	pci_host_irq_handle(slot,NULL,NULL);
#else
	pci_slave_irq_handle(slot,NULL,NULL);
#endif
	return ;
}
//#endif
void slave_int_disable(void)
{
	return;
}

int hi3511_slave_interrupt_check(void)
{
	return HI_HOST;
}

void slave_int_enable(void)
{
	return;
}

void slave_int_clear(void)
{
	return;
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

	hi_dev_slave->pf_base_addr = (unsigned long)ioremap(0x62400000, 0x400000);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL4,"pf_base_addr is 0x%8lX",(unsigned long)hi_dev_slave->pf_base_addr);
	
	hi_dev_slave->pf_end_addr = hi_dev_slave->pf_base_addr+0x400000;
	
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


void DISABLE_INT_HI_SLOT(int slot)
{
	return;
}

void DISABLE_INT_HI_DMA(void)
{
	return;
}

void CLEAR_INT_HI_SLOT(int slot)
{
	return;
}

void CLEAR_INT_HI_DMA(void)
{
	return;
}

void ENABLE_INT_HI_SLOT(int slot)
{
	return;
}
void ENABLE_INT_HI_DMA(void)
{
	return;
}
#endif
