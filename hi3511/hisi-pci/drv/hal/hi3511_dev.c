/* 
*hi3511_pci_driver.c: A hi3511 low level driver for Linux.
*Copyright 2006 hisilicon Corporation 
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
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

#include <asm/processor.h>      /* Processor type for cache alignment. */
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>	/* User space memory access functions */

#include "../include/hi3511_pci.h"
#include <linux/kcom.h>
#include <kcom/mmz.h>

struct kcom_object *kcom;
struct kcom_media_memory *kcom_media_memory;
unsigned long hi3511_slave_interrupt_flag;
EXPORT_SYMBOL(hi3511_slave_interrupt_flag);

#define HI3511_MODULE_NAME "hi3511"
#define HI3511_DRV_VERSION "v0.0"
#define HI_PCI_DEVICE_NUMBER 20 //max device number

#define TEST_DBUG_LEVEL1	0
#define TEST_DBUG_LEVEL2	1
#define TEST_DBUG_LEVEL3	2
#define TEST_DBUG_LEVEL4	3
#define TEST_DBUG_LEVEL	  1	
#define TEST_DEBUG(level, s, params...)   do{ \
	if(level >= TEST_DBUG_LEVEL3)  \
		printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, ##params); \
	}while(0)

#define  BEHIND_PCI_BRIDGE  10
static char version[] __devinitdata =
KERN_INFO "hi3511_pci_driver.c: " HI3511_DRV_VERSION "\n";

static struct pci_device_id hi3511_pci_tbl [] = {
	{PCI_VENDOR_HI3511, PCI_DEVICE_HI3511,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};

struct  hi3511_dev hi3511_device_head;
unsigned long hi3511dev_pci_dma_zone_base;
unsigned char *hi3511dev_pci_dma_zone_virt_base;

void *hi3511_slave_interrupt_flag_virt;
EXPORT_SYMBOL(hi3511_slave_interrupt_flag_virt);

#ifdef CONFIG_PCI_HOST
DECLARE_MUTEX(pci_dma_sem);
EXPORT_SYMBOL(pci_dma_sem);
#endif

struct _slot_map
{
	struct pci_dev *pdev;
	void *base_addr_virt;
	void *cfg_base_addr_virt;
};

static struct _slot_map slot_map[HI_PCI_DEVICE_NUMBER];

EXPORT_SYMBOL(hi3511_device_head);
EXPORT_SYMBOL(slot_map);

MODULE_DEVICE_TABLE (pci, hi3511_pci_tbl);

//static void sis900_read_mode(struct net_device *net_dev, int *speed, int *duplex);

/* share memory size, only on device. */
static unsigned int shm_size = 0;
module_param(shm_size, uint, S_IRUGO);

static unsigned long shm_phys_addr = 0;
module_param(shm_phys_addr, ulong, S_IRUGO);

static int is_host = 0;


MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("Hi3511 PCI driver");
MODULE_LICENSE("GPL");

int hi3511_move_pf_window(void *addr, void *npaddr_p)
{
	unsigned long intend_addr;
	unsigned long _addr;
	int i;
	//unsigned int reg;
	_addr = (unsigned long)npaddr_p+0x74;
	intend_addr=((unsigned long)addr&0xffff0000UL)|0x00000001UL;
	*(unsigned int *)hi3511dev_pci_dma_zone_virt_base=intend_addr;
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, _addr);
	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, hi3511dev_pci_dma_zone_base);
	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);
	//reg = readl(HI3511_PCI_VIRT_BASE +WDMA_CONTROL);
	for(i=0; i<10 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {
		udelay(100);
	};
	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)
		printk(KERN_ERR "pci_config_dma_read timeout!\n");
	return 0;

}

int hi3511_dma_write( void *pci_addr_virt, void *ahb_addr_phy, int len,unsigned int slot)
{
	void *base_addr_phy;
	unsigned int control;
	int i;
	//int err=0;
	//judge the len argument 
	PCI_HAL_DEBUG(0, "ok dma write %d", 1);

	if(len>0xffffff){
			printk("transfer length is too long\n");
			goto out;
	}
	
	base_addr_phy = (char *)pci_resource_start(slot_map[slot].pdev, 4);
	PCI_HAL_DEBUG(0, "ahb_addr_phy is 0x%lx", (unsigned long)ahb_addr_phy);
	PCI_HAL_DEBUG(0, "pci_addr_virt is 0x%lx", (unsigned long)pci_addr_virt);

	PCI_HAL_DEBUG(0, "base_addr_phy is 0x%lx", (unsigned long)base_addr_phy);
	//write ahb_addr, pci_addr, and length to ahb registers
	hi3511_bridge_ahb_writel( WDMA_AHB_ADDR,(unsigned long)ahb_addr_phy);
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, (unsigned long)(base_addr_phy+((unsigned long)pci_addr_virt-(unsigned long)slot_map[slot].base_addr_virt)));

	control= (len<<8 |DMAW_MEM|DMAC_INT_EN);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	control |= DMAC_START;

	PCI_HAL_DEBUG(0, "dma control is 0x%x", control);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

	for(i=0; i<5000 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {
		udelay(1000);
	};
	
	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)
		printk(KERN_ERR "pci_config_dma_read timeout!\n");
      return 0;
out:
      return -1;
}


int hi3511_dma_read(void *ahb_addr_phy, void *pci_addr_virt, int len,unsigned int slot)
{
	void *base_addr_phy;
	unsigned int control;
	int i;
	//int err=0;
	//judge the len argument 
	PCI_HAL_DEBUG(0, "ok dma read %d", 1);
	if(len>0xffffff){
		printk("transfer length is too long\n");
		goto out;
	}
	base_addr_phy = (char *)pci_resource_start(slot_map[slot].pdev, 4);
	//write ahb_addr, pci_addr, and length to ahb registers
	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (unsigned long)ahb_addr_phy);
	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, (unsigned long)((unsigned long)base_addr_phy+((unsigned long)pci_addr_virt-(unsigned long)slot_map[slot].base_addr_virt)));
	control= (len<<8 |DMAR_MEM|DMAC_INT_EN);
	hi3511_bridge_ahb_writel(RDMA_CONTROL, control);
	control |= DMAC_START;
	hi3511_bridge_ahb_writel(RDMA_CONTROL, control);
	for(i=0; i<5000 && (hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001); i++) {
		udelay(1000);
	};
	if(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001)
		printk(KERN_ERR "pci_config_dma_read timeout!\n");
      return 0;
out:
      return -1;
}


/**
 *	alloc_hi3511dev - allocate hi3511 device
 *	@sizeof_priv:	size of private data to allocate space for
 *	@name:		device name format string
 *	@setup:		callback to initialize device
 *
 *	Allocates a struct hi3511_dev for driver use
 *	and performs basic initialization.
 */
struct hi3511_dev *alloc_hi3511dev(void)
{
	struct hi3511_dev *dev;
	int alloc_size;
	PCI_HAL_DEBUG(0, "alloc hi3511 %d", 1);
	alloc_size = sizeof(*dev) ;
	dev= kmalloc(alloc_size, GFP_KERNEL);
	if (!dev) {
		printk(KERN_ERR "alloc_hi3511dev: Unable to allocate device.\n");
		return NULL;
	}
	memset(dev, 0, alloc_size);
	return dev;
}
EXPORT_SYMBOL(alloc_hi3511dev);

void free_hi3511dev(struct hi3511_dev *dev)
{
	kfree(dev);
}

//master to slave irq
void master_to_slave_irq(unsigned long slot)
{
//	int flag;

	//access target slave's configuration register,set pci doorbell  bit
	//according to the slot number, to difference slave, different pci doorbell bit is triggered.
	TEST_DEBUG(0, "master_to_slave_irq %d", slot);

	/* lock dma */
/*	flag = down_interruptible(&pci_dma_sem);
	if(flag) {
		return ;
	}
	if(slot_map[slot].pdev != NULL)
		pci_bus_write_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_ICMD, 0x000f0000);

	up(&pci_dma_sem);
*/
	writel(0x000f0000, slot_map[slot].cfg_base_addr_virt + PCI_ICMD);

	TEST_DEBUG(0, "master_to_slave_irq, ok %d", 1);

}

EXPORT_SYMBOL(master_to_slave_irq);

void slave_to_master_irq(unsigned long slot)
{
	hi3511_bridge_ahb_writel(CPU_ICMD,0x00f00000);
}
EXPORT_SYMBOL(slave_to_master_irq);

void disable_int_hi_slot(int slot)
{
    unsigned long mask;
//	pci_bus_write_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_IMASK, 0x00000000);
    mask = readl(slot_map[slot].cfg_base_addr_virt + PCI_IMASK);
    mask &= 0xff0fffff;
    writel(mask, slot_map[slot].cfg_base_addr_virt + PCI_IMASK);
	interrupt_disable(PCIINT0);
}
EXPORT_SYMBOL(disable_int_hi_slot);

void disable_int_hi_dma(void)  
{
	interrupt_disable((DMAREADEND|DMAWRITEEND));
}
EXPORT_SYMBOL(disable_int_hi_dma);

void clear_int_hi_slot(int slot) 
{
    unsigned long status;
	writel(0x00f00000, slot_map[slot].cfg_base_addr_virt + PCI_ISTATUS);
    status = readl(slot_map[slot].cfg_base_addr_virt + PCI_ISTATUS);
	udelay(2); // for bus transfer delay
	interrupt_clear(PCIINT0);
}
EXPORT_SYMBOL(clear_int_hi_slot);

unsigned long read_int_hi_slot(int slot) 
{
	unsigned long int_status;
//	pci_bus_read_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_ISTATUS,(unsigned int *)&int_status);
	int_status = readl(slot_map[slot].cfg_base_addr_virt + PCI_ISTATUS);
	return int_status;

}
EXPORT_SYMBOL(read_int_hi_slot);

unsigned long check_int_hi_slot(int slot)
{
	unsigned long doorbell_int_status;
	doorbell_int_status = read_int_hi_slot(slot) & 0x00f00000;
	return doorbell_int_status;
}

void clear_int_hi_dma(void)
{	
	interrupt_clear((DMAREADEND|DMAWRITEEND));
}
EXPORT_SYMBOL(clear_int_hi_dma);

/* enable doorbell interrupt on a slot */
void enable_int_hi_slot(int slot)
{
    unsigned long int_mask;
    int_mask = readl(slot_map[slot].cfg_base_addr_virt + PCI_IMASK);
    int_mask |= 0x00f00000;

//	pci_bus_write_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_IMASK, 0x00f00000);
	writel(int_mask, slot_map[slot].cfg_base_addr_virt + PCI_IMASK);

	interrupt_enable(PCIINT0);

}
EXPORT_SYMBOL(enable_int_hi_slot);
/* enable DMA interrupt on a slot */
void enable_int_dma_hi_slot(int slot)
{
    unsigned long int_mask;
	void *addr = 0;
	addr = (slot_map[slot].cfg_base_addr_virt + PCI_IMASK);
    int_mask = readl(addr);
    int_mask |= 0x00000101;
//	pci_bus_write_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_IMASK, 0x00f00000);
	writel(int_mask, addr);
	interrupt_enable(PCIINT0);

}
EXPORT_SYMBOL(enable_int_dma_hi_slot);
/* added by c42025 */
void disable_int_dma_hi_slot(int slot)
{
    unsigned long int_mask;
    int_mask = readl(slot_map[slot].cfg_base_addr_virt + PCI_IMASK);
    int_mask &= ~0x00000101;
//	pci_bus_write_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_IMASK, 0x00f00000);
	writel(int_mask, slot_map[slot].cfg_base_addr_virt + PCI_IMASK);

	interrupt_disable(PCIINT0);

}
EXPORT_SYMBOL(disable_int_dma_hi_slot);
void clear_int_dma_hi_slot(int slot)
{
    //  pci_bus_write_config_dword(slot_map[slot].pdev->bus, slot_map[slot].pdev->devfn, PCI_IMASK, 0x00f00000);
        writel(0x00000101, slot_map[slot].cfg_base_addr_virt + PCI_ISTATUS);
        readl(slot_map[slot].cfg_base_addr_virt + PCI_ISTATUS);
		udelay(2);
        interrupt_clear(PCIINT0);    
}
EXPORT_SYMBOL(clear_int_dma_hi_slot);


void enable_int_hi_dma(void) 
{
	interrupt_enable((DMAREADEND|DMAWRITEEND));
}
EXPORT_SYMBOL(enable_int_hi_dma);


void slave_int_door_disable(int slot)
{
    hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)&0xfff0ffff));
}

void slave_int_door_clear(int slot)
{
   // hi3511_bridge_ahb_writel(CPU_ISTATUS,0x000f0000);
    hi3511_bridge_ahb_writeb(CPU_ISTATUS + 0x2,0x0f);
}

void slave_int_door_enable(int slot)
{
    hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)|0x000f0000));
}

/*void slave_int_dma_disable(void)
{
    hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)&0xfffffefe));
}

void slave_int_dma_clear(void)
{
   // hi3511_bridge_ahb_writel(CPU_ISTATUS,0x00000101);
    hi3511_bridge_ahb_writew(CPU_ISTATUS,0x101);
}

void slave_int_dma_enable(void)
{
    hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)|0x00000101));
}
*/


unsigned long pci_slave_interrupt_check(int slot)
{
    unsigned int status;
    status=PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3;
    if(interrupt_check(status)){
        return hi3511_bridge_ahb_readl(CPU_ISTATUS); //&(PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3))
    }
    
    return 0;
}

int pci_is_host(int slot)
{
	return HI_PCIT_IS_HOST;
}

/**
 *	hi3511_probe - Probe for hi3511 device
 *	@pci_dev: the hi3511 pci device
 *	@pci_id: the pci device ID
 *
 *	Check and probe hi3511 device for @pci_dev.
 *	
 */

static int __devinit hi3511_probe(struct pci_dev *pci_dev,
				const struct pci_device_id *pci_id)
{
	struct hi3511_dev *hi_dev;
	//struct pci_dev *dev;
	unsigned long npmemaddr;
	unsigned long pfmemaddr;
	unsigned long cfgmemaddr;
	unsigned long npmemaddr_virt;
	unsigned long pfmemaddr_virt;
	unsigned long cfgmemaddr_virt;
	int i, ret;
	unsigned long cmd,io_size=0x400000;
	PCI_HAL_DEBUG(0, "ok, hi3511_dev probed %d", 1);
	//char *card_name = card_names[pci_id->driver_data];
	//const char *dev_name = pci_name(pci_dev);

/* when built into the kernel, we only print version if device is found */
#ifndef MODULE
	static int printed_version;

	if (!printed_version++)

		printk(version);
#endif
	PCI_HAL_DEBUG(0, "probing now!! %d", 1);
	/* setup various bits in PCI command register */
	ret = pci_enable_device(pci_dev);

	if(ret) return ret;
	
	i = pci_set_dma_mask(pci_dev, DMA_32BIT_MASK);

	if(i){
		printk(KERN_ERR "hi3511.c: architecture does not support"
			"32bit PCI busmaster DMA\n");
		return i;
	}
	
	//pci_set_master(pci_dev);
	
	hi_dev = alloc_hi3511dev();

	if (!hi_dev) return -ENOMEM;
	/* We do a request_region() to register /proc/ioports info. */
	npmemaddr = pci_resource_start(pci_dev, 3);
	
	pfmemaddr = pci_resource_start(pci_dev, 4);
	cfgmemaddr = pci_resource_start(pci_dev, 5);

	ret = pci_request_regions(pci_dev, "hi3511");

	if (ret)
		goto err_out;

	PCI_HAL_DEBUG(0, "The bus number is %d", pci_dev->bus->number);

	hi_dev->slot =  PCI_SLOT(pci_dev->devfn) + (pci_dev->bus->number)*BEHIND_PCI_BRIDGE;

	PCI_HAL_DEBUG(0, "The dev slot number is %d", hi_dev->slot);

	slot_map[hi_dev->slot].pdev=pci_dev;

	pci_set_drvdata(pci_dev,hi_dev);

	pci_write_config_dword(pci_dev,PCI_IMASK,0x00f00000);
	
	hi_dev->irq = pci_dev->irq;
	//spin_lock_init(&sis_priv->lock);
	//pci_set_drvdata(pci_dev, hi_dev);
	/* The hi3511-specific entries in the device structure. */
	hi_dev->write = hi3511_dma_write;

	hi_dev->read = hi3511_dma_read;

	hi_dev->trigger = &master_to_slave_irq;
	hi_dev->check_doorbell_irq = &check_int_hi_slot;
	hi_dev->enable_doorbell_irq = &enable_int_hi_slot;
	hi_dev->disable_doorbell_irq = &disable_int_hi_slot;
	hi_dev->clear_doorbell_irq = &clear_int_hi_slot;
	hi_dev->is_host = &pci_is_host;

	hi_dev->pdev = pci_dev;
	
	list_add_tail(&hi_dev->node, &hi3511_device_head.node);

	npmemaddr_virt= (unsigned long)ioremap(npmemaddr,io_size);

	pfmemaddr_virt= (unsigned long)ioremap(pfmemaddr, io_size);

	cfgmemaddr_virt= (unsigned long)ioremap(cfgmemaddr, 256);
	slot_map[hi_dev->slot].base_addr_virt = (unsigned long *)pfmemaddr_virt;
	slot_map[hi_dev->slot].cfg_base_addr_virt = (unsigned long *)cfgmemaddr_virt;
//set pf window size 16M Byte
	//hi3511_move_pf_window(0xb0000000, npmemaddr);
	//*(unsigned long *)(npmemaddr_virt+0x98)=0xff000000;
	writel(0xff000000, npmemaddr_virt+0x98);

	hi_dev->pf_base_addr = pfmemaddr_virt;

	hi_dev->np_base_addr = npmemaddr_virt; 

	hi_dev->pf_end_addr = pfmemaddr_virt+0x800000;
	hi_dev->cfg_base_addr = cfgmemaddr_virt; 	

	hi_dev->pf_base_addr_phy = pfmemaddr;

	hi_dev->np_base_addr_phy = npmemaddr; 

	hi_dev->cfg_base_addr_phy = cfgmemaddr; 	
//macro start addr
	//hi3511_move_pf_window(0xb0000000, npmemaddr);
	//*(unsigned long *)(npmemaddr_virt+0x74)=0xe4000001;
//	writel(PCI_SLAVE_SHAREMEM_BASE+1, npmemaddr_virt+0x74);//delete for dynamic alloc  

	//printk("npmemaddr is %lx, pfmemaddr is %lx \n,npmemaddr_virt is %lx, pfmemaddr_virt is %lx,pfmemaddr_virt_end is %lx\n",npmemaddr,pfmemaddr,npmemaddr_virt,pfmemaddr_virt,hi_dev->pf_end_addr);

	pci_set_master(pci_dev);

	cmd=0x403f;//latency time and cache line

	pci_write_config_dword(pci_dev, PCI_CACHE_LINE_SIZE, cmd);

	pci_write_config_word(pci_dev, 0x04, 0x1ff);

	
	return 0;

  /*err_out_cleardev:
 	pci_set_drvdata(pci_dev, NULL);
	pci_release_regions(pci_dev);*/
 err_out:

	free_hi3511dev(hi_dev);

	return ret;
}


int hi3511_interrupt_check(void)
{
	unsigned long status,flag=0;
	PCI_HAL_DEBUG(0, "interrupt checked %d", 1);
	
	status=PMEINT|CLKRUNINT|PMSTATINT|PCISERR|PCIINT3|\
		PCIINT2|PCIINT1|PCIINT0|APWINDISCARD|APWINFTCHERR|\
		APWINPSTERR |DMAREADAHBERR|DMAREADPERR|DMAREADABORT|\
		DMAREADEND|PAWINDISCARD|PAWINFTCHERR |PAWINPSTERR |\
		DMAWRITEAHBERR |DMAWRITEPERR|DMAWRITEABORT|DMAWRITEEND;
	if(interrupt_check(status)){

		if(hi3511_bridge_ahb_readl(CPU_ISTATUS)&PCIINT0){
			struct list_head *node;

			unsigned long i = 0;		

			list_for_each(node, &(hi3511_device_head.node)){
		
				flag = readl(hi3511_slave_interrupt_flag_virt + 4 * i);

				PCI_HAL_DEBUG(0,"the interrupt flag is %lu", flag);

				if(flag == 0xdadadada){
					i++;
				}
				else{

					PCI_HAL_DEBUG(0, "the interrupt flag is %lu", flag);
					break;
				}
			}
			if(flag != 0xdadadada){
				flag = flag | i<<(24);	
				i = 0;

				PCI_HAL_DEBUG(0, "the interrupt flag is %lu", flag);
				return flag;
			}else{
				flag = 0x1111;
			}
		}
		PCI_HAL_DEBUG(0, "interrupt checked %d", 4);

		if(hi3511_bridge_ahb_readl(CPU_ISTATUS)&PCIINT1) return HI_NONE;

//		PCI_HAL_DEBUG(0, "interrupt checked %d", 5);

//		if(hi3511_bridge_ahb_readl(CPU_ISTATUS)&PCIINT2) return HI_SLOT3;

		PCI_HAL_DEBUG(0, "interrupt checked %d", 6);

	}
	//PCI_HAL_DEBUG(0, "interrupt checked %d", 7);
	return -1;
}
EXPORT_SYMBOL(hi3511_interrupt_check);

int hi3511_dma_check(void)
{
	PCI_HAL_DEBUG(0, "dma checked %d", 1);

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&DMASTARTTRANS) return WRITEBUSY;

	if(hi3511_bridge_ahb_readl(RDMA_CONTROL)&DMASTARTTRANS) return READBUSY;

	return NOTBUSY;
}
EXPORT_SYMBOL(hi3511_dma_check);

/*functions for external DMA, move window of the device which is marked with the slot argument. 
addr is the physics address on slave*/
/*unsigned int hi3511_window_move_from(int slot, unsigned long addr, int size)
{
	int devfn;
	unsigned long *pb4addr, *pb3addr;
	unsigned long b4addr,b3addr,cur_addr;
	pb4addr=&b4addr;
	pb3addr=&b3addr;
	PCI_HAL_DEBUG(0, "ok, move or not %d", 1);
	devfn=PCI_DEVFN(slot,0);
//write the intended register address to PCIAHB_ADDR_NP with BAR4(correspond to the PCIAHB_ADDR_PF). 
	pci_bus_read_config_dword ((struct pci_bus *)0, devfn, BAR4, pb4addr);
	pci_bus_read_config_dword ((struct pci_bus *)0, devfn, BAR3, pb3addr);
//set size of the windows
	__raw_writel(PCIAHB_SIZ_NP+HI3511_PCI_VIRT_BASE,b4addr+PCIAHB_ADDR_NP);   //write PCI_SIZ_NP address into np window
	__raw_writel(size,b3addr);                //access bar3 the base of np window
//save current window position
	cur_addr=__raw_readl(b4addr+PCIAHB_ADDR_NP);
//write value to the intended register with BAR3(correspond to the PCIAHB_ADDR_NP)
	__raw_writel(addr,b4addr+PCIAHB_ADDR_NP); 

	return cur_addr;

}

int hi3511_window_restore(int slot, unsigned long saved_addr)
{
//restore window position
	int devfn;
	unsigned long *pb4addr, *pb3addr;
	unsigned long b4addr,b3addr,cur_addr;
	pb4addr=&b4addr;
	pb3addr=&b3addr;
	PCI_HAL_DEBUG(0, "ok, move or not %d", 1);
	devfn=PCI_DEVFN(slot,0);
//write the intended register address to PCIAHB_ADDR_NP with BAR4(correspond to the PCIAHB_ADDR_PF). 
	pci_bus_read_config_dword ((struct pci_bus *)0, devfn, BAR4, pb4addr);
	pci_bus_read_config_dword ((struct pci_bus *)0, devfn, BAR3, pb3addr);
	__raw_writel(saved_addr,b4addr+PCIAHB_ADDR_NP);

	return 0;
}
*/
void dev_resource_unmap(void)
{
	struct list_head *node;

	struct hi3511_dev *pNode;

	list_for_each (node, &(hi3511_device_head.node))
	{
		pNode = list_entry(node, struct hi3511_dev, node);

		iounmap((void *)pNode->pf_base_addr);

		iounmap((void *)pNode->np_base_addr);
	}
}

	
/**
 *	hi3511_remove - Remove hi3511 device 
 *	@pci_dev: the pci device to be removed
 *
 *	remove and release hi3511  device
 */

static void __devexit hi3511_remove(struct pci_dev *pci_dev)
{
	struct hi3511_dev *dev = pci_get_drvdata(pci_dev);
	
	dev_resource_unmap();

	pci_release_regions(pci_dev);

	free_hi3511dev(dev);

	pci_disable_device(pci_dev);

	pci_set_drvdata(pci_dev, NULL);
}

#ifdef CONFIG_PM

static int hi3511_suspend(struct pci_dev *pci_dev, pm_message_t state)
{
	
	return 0;
}

static int hi3511_resume(struct pci_dev *pci_dev)
{
	
//stop the slave arm
	/*arm_reset |=0x00;
	hi3511_bridge_writew(WDMA_AHB_ADDR, &arm_reset);
	hi3511_bridge_writew(WDMA_PCI_ADDR, pci_addr+ARM_RESET);
	control= (1<<8 |DMAW_MEM|DMAC_INT_EN);
	hi3511_bridge_writew(WDMA_CONTROL, control);
	control |= DMAC_START;
	hi3511_bridge_writew(WDMA_CONTROL, control);*/

//count the slot number

//call the interface function for slave boot.


	return 0;
}
#endif /* CONFIG_PM */

static struct pci_driver hi3511_pci_driver = {
	.name		= HI3511_MODULE_NAME,
	.id_table	= hi3511_pci_tbl,
	.probe		= hi3511_probe,
	.remove		= hi3511_remove,
#ifdef CONFIG_PM
	.suspend	= hi3511_suspend,
	.resume		= hi3511_resume,
#endif /* CONFIG_PM */
};

#if 0
DECLARE_MUTEX(pci_dma_sem);
EXPORT_SYMBOL(pci_dma_sem);

void hi3511_pci_dma_read (unsigned long pci_phy,unsigned long ahb_phy,int len)
{
		unsigned long control;

		control = (len<<8)| 0x61;

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pci_phy);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, ahb_phy);

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);

		return ;
}

void hi3511_pci_dma_write (unsigned long pci_phy,unsigned long ahb_phy,int len)
{
		unsigned long control;

		control = (len<<8)| 0x71;

		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pci_phy);

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, ahb_phy);

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);	

		return;
}

EXPORT_SYMBOL(hi3511_pci_dma_read);
EXPORT_SYMBOL(hi3511_pci_dma_write);
#endif

static int hi3511_pci_slave_init(void)
{            /* alloc share memory. */
	struct hi3511_dev *hi_dev;

	hi_dev = alloc_hi3511dev();

	if(hi_dev == NULL) return 0;

	hi_dev->slot = 0;

	hi_dev->irq = 30;
	/* The hi3511-specific entries in the device structure. */
	hi_dev->trigger= &slave_to_master_irq;
	hi_dev->check_doorbell_irq = &pci_slave_interrupt_check;
	hi_dev->enable_doorbell_irq = &slave_int_door_enable;
	hi_dev->disable_doorbell_irq = &slave_int_door_disable;
	hi_dev->clear_doorbell_irq = &slave_int_door_clear;
	hi_dev->is_host = &pci_is_host;
	
	hi_dev->shm_size = shm_size;

	hi_dev->shm_phys_addr = shm_phys_addr;
	printk("share meory: phys addr (%lx), size(%x)\n", hi_dev->shm_phys_addr, hi_dev->shm_size);

	hi3511_bridge_ahb_writel(PCIAHB_ADDR_PF, hi_dev->shm_phys_addr + 1);

	list_add_tail(&hi_dev->node, &hi3511_device_head.node);

	return 0;
}

static int hi3511_pci_slave_remove(void)
{
	struct list_head *node;

	struct hi3511_dev *pNode;

	list_for_each (node, &(hi3511_device_head.node))
	{
		pNode = list_entry(node, struct hi3511_dev, node);

		free_hi3511dev(pNode);

	}

	
}

static int __init hi3511_init_module(void)
{

	int retn;
	
	PCI_HAL_DEBUG(0, "driver begin %d", 1);
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
/* when a module, this is printed whether or not devices are found in probe */
#ifdef MODULE
	printk(version);
#endif
#if 0
	if(!(hi3511_bridge_ahb_readl(CPU_VERSION)&0x00002000)) 
	{
//		printk("ERROR! Hardware adaptive module shouldn't be inserted on Non-PCI-host system!\n");

		return 0;
	}
#endif
	kcom_getby_uuid(UUID_MEDIA_MEMORY, &kcom);

        if(kcom ==NULL) {

                printk(KERN_ERR "MMZ: can't access mmz, start mmz service first.\n");

                return -1;
        }

        kcom_media_memory = kcom_to_instance(kcom, struct kcom_media_memory, kcom);
	//hi3511dev_pci_dma_zone_base = hil_mmb_alloc("pcidev-dma-buffer", SZ_16M, 0, 0, NULL);
	hi3511dev_pci_dma_zone_base = kcom_media_memory->new_mmb("pcidev-dma-buffer", SZ_1K, 0,NULL);

	if(hi3511dev_pci_dma_zone_base == 0){

		PCI_HAL_DEBUG(0, "hil_mmb_alloc error %d", 1);
	}
	//hi3511dev_pci_dma_zone_virt_base = hil_mmb_map2kern(hi3511dev_pci_dma_zone_base);
	hi3511dev_pci_dma_zone_virt_base =  kcom_media_memory->remap_mmb(hi3511dev_pci_dma_zone_base);

	memset(hi3511dev_pci_dma_zone_virt_base ,0xa5, SZ_1K);


	hi3511_slave_interrupt_flag = kcom_media_memory->new_mmb("PCI_Int_Flags_For_Certain_Slaves", hi3511_slave_interrupt_flag_size, 0, NULL);

	if(hi3511_slave_interrupt_flag == 0){

		PCI_HAL_DEBUG(0, "hil_mmb_alloc error %d", 1);
	}

	hi3511_slave_interrupt_flag_virt =  kcom_media_memory->remap_mmb(hi3511_slave_interrupt_flag);


//	hi3511_slave_interrupt_flag_virt = ioremap(hi3511_slave_interrupt_flag, hi3511_slave_interrupt_flag_size);
		
//		if (hi3511_slave_interrupt_flag_virt== NULL) return -ENOMEM;
	
	memset(hi3511_slave_interrupt_flag_virt, 0xda , hi3511_slave_interrupt_flag_size);

	PCI_HAL_DEBUG(0, "driver begin %d", 2);

	INIT_LIST_HEAD(&hi3511_device_head.node);
	
	PCI_HAL_DEBUG(0, "driver begin %d", 3);

	is_host = HI_PCIT_IS_HOST;	
	printk("is_host %d\n", is_host);
	if(is_host){
		retn = pci_module_init(&hi3511_pci_driver);
	}
	else{
		retn = hi3511_pci_slave_init();
	}
	PCI_HAL_DEBUG(0, "driver begin %d", retn);

	return retn; 
}




static void __exit hi3511_cleanup_module(void)
{
	if(!(hi3511_bridge_ahb_readl(CPU_VERSION)&0x00002000)) 
	{
//		printk("ERROR! Hardware adaptive module shouldn't be inserted on Non-PCI-host system!\n");

		return;
	}	
	//hil_mmb_unmap(hi3511dev_pci_dma_zone_base);
	//hil_mmb_free(hi3511dev_pci_dma_zone_base);
	kcom_media_memory->unmap_mmb(hi3511dev_pci_dma_zone_virt_base);

	kcom_media_memory->delete_mmb(hi3511dev_pci_dma_zone_base);

	kcom_media_memory->unmap_mmb(hi3511_slave_interrupt_flag_virt);

	kcom_media_memory->delete_mmb(hi3511_slave_interrupt_flag);

	kcom_put(kcom);

//	iounmap(hi3511_slave_interrupt_flag_virt);

	PCI_HAL_DEBUG(0, "driver modul exit %d", 1);

	is_host = HI_PCIT_IS_HOST;	
	if(is_host){
		pci_unregister_driver(&hi3511_pci_driver);
	}
	else{

        	hi3511_pci_slave_remove();
        }

}

module_init(hi3511_init_module);

module_exit(hi3511_cleanup_module);

MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
