#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
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
#include <linux/device.h>
#include <asm/processor.h>      /* Processor type for cache alignment. */
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>	/* User space memory access functions */

#include "hisi_pci_hw.h"
#include "hi3511_pci.h"

extern struct hi3511_dev hi3511_device_head;

pci_vdd_info_t hisi_pci_vdd_info;
EXPORT_SYMBOL(hisi_pci_vdd_info);

struct hisi_pci_dev_list hisi_pci_dev;
EXPORT_SYMBOL(hisi_pci_dev);

static unsigned int offset_size = 0;
module_param(offset_size, uint, S_IRUGO);

static int __init hw_init(void)
{
	struct hisi_pcicom_dev *pci_dev = NULL;
	struct list_head *entry, *tmp;
	struct  hi3511_dev *hal_dev;

	INIT_LIST_HEAD(&hisi_pci_dev.list);
	hisi_pci_dev.list_num = 0;

	list_for_each_safe(entry, tmp, &hi3511_device_head.node){
		/* get the HW-HAL dev from hi3511_device_head */
		hal_dev = list_entry(entry, struct hi3511_dev, node);

		pci_dev = hisi_alloc_pci_dev();
	        if(!pci_dev){
        	        printk(KERN_INFO "Failed to allocate memory\n");
			goto init_failture;
	        }
		init_timer(&pci_dev->timer);
		
		/* paramaters and functions from hw-hal layer */
		pci_dev->pdev = hal_dev->pdev;
		pci_dev->shm_phys_addr = hal_dev->shm_phys_addr;

		if(hal_dev->is_host(hal_dev->slot)){
			/* host from hw-hal layer */
			pci_dev->pf_base_addr = hal_dev->pf_base_addr + offset_size;
			pci_dev->pf_end_addr = hal_dev->pf_end_addr;
		}
		else{
			/* slave from ioremap */
			pci_dev->pf_base_addr = (unsigned long)ioremap(hal_dev->shm_phys_addr + offset_size, hal_dev->shm_size);
			barrier();
			pci_dev->pf_end_addr = pci_dev->pf_base_addr + hal_dev->shm_size;
			printk("base 0x%8lx end 0x%8lx \n",pci_dev->pf_base_addr,pci_dev->pf_end_addr);
			pci_dev->shm_size = hal_dev->shm_size;
		}
		pci_dev->irq = hal_dev->irq;
		pci_dev->slot_id = hal_dev->slot + 1; 
		pci_dev->cfg_base_addr = hal_dev->cfg_base_addr;
		
		pci_dev->trigger = hal_dev->trigger;
		pci_dev->check_irq = hal_dev->check_doorbell_irq;
		pci_dev->enable_irq = hal_dev->enable_doorbell_irq;
		pci_dev->disable_irq = hal_dev->disable_doorbell_irq;
		pci_dev->clear_irq = hal_dev->clear_doorbell_irq; 
		
		list_add_tail(&pci_dev->node, &hisi_pci_dev.list);

		hisi_pci_dev.list_num ++;

	        hisi_pci_vdd_info.version = 1;

		/* host and slave are different */
		if(hal_dev->is_host(hal_dev->slot)){
	        	hisi_pci_vdd_info.id = 0;
		}
		else{
	        	hisi_pci_vdd_info.id = 1;
		}
		
		printk(KERN_INFO "PCI: slot %d \n",pci_dev->slot_id);
	}

	/* initial slave mapped flage */
	hisi_pci_vdd_info.mapped = 0; 
	printk(KERN_INFO "PCI: host communicate hw layer setup! \n");

	return  0;

init_failture:
	list_for_each_safe(entry, tmp, &hisi_pci_dev.list){
		pci_dev = list_entry(entry, struct hisi_pcicom_dev, node);
		if(pci_dev){
			list_del(&pci_dev->node);
			/* slave need to iounmap*/
			if(!hal_dev->is_host(hal_dev->slot)){
				iounmap((void *)pci_dev->pf_base_addr);
			}
			kfree(pci_dev);
		}
	}

	return -1;
}


static void __exit hw_cleanup(void)
{
	struct hisi_pcicom_dev *pci_dev = NULL;
	struct list_head *entry, *tmp;

	list_for_each_safe(entry, tmp, &hisi_pci_dev.list){
		pci_dev = list_entry(entry, struct hisi_pcicom_dev, node);
		if(pci_dev){
			list_del(&pci_dev->node);
			kfree(pci_dev);
		}
	}

	return;
}

module_init(hw_init);
module_exit(hw_cleanup);
MODULE_AUTHOR("chanjinn");
MODULE_DESCRIPTION("PCI communicate interface for Hisilicon Solution");

