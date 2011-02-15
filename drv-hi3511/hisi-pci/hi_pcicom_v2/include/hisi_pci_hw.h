#ifndef _HISI_PCI_HW_H_ 
#define _HISI_PCI_HW_H_ 1

#include <linux/types.h>
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
#include <asm/arch/platform.h>
#include <asm/arch/hardware.h>

#define PCI_VENDOR_HISI 0x19e5	//0x1556
#define PCI_DEVICE_HISI  0x3511	//0xbb00

/* default slave irq */
#define HISI_PCI_SLAVE_IRQ 30

/* 1M bytes shared by default */
#define HISI_PCI_SHARE_MEM_SIZE (1024 * 1024)

#define PCI_VIRT_BASE IO_ADDRESS(HISILICON_PCI_BR_BASE)
#if 0
enum hisi_cpu_interrupt_status_register_bits {
	PMEINT = 0x80000000, CLKRUNINT = 0x40000000, PMSTATINT = 0x20000000, 
	PCISERR = 0x10000000, PCIINT3 = 0x08000000, PCIINT2 = 0x04000000,
	PCIINT1 = 0x02000000, PCIINT0 = 0x01000000,  PCIDOORBELL3 = 0x00080000,
	PCIDOORBELL2 = 0x00040000, PCIDOORBELL1 = 0x00020000, PCIDOORBELL0 = 0x00010000, 
	APWINDISCARD = 0x00004000, APWINFTCHERR = 0x00002000, APWINPSTERR = 0x00001000,
	DMAREADAHBERR = 0x00000800, DMAREADPERR  = 0x00000400, DMAREADABORT = 0x00000200,
	DMAREADEND = 0x00000100, PAWINDISCARD  = 0x00000040, PAWINFTCHERR = 0x00000020, 
	PAWINPSTERR = 0x00000010, DMAWRITEAHBERR = 0x00000008, DMAWRITEPERR  = 0x00000004, 
	DMAWRITEABORT = 0x00000002, DMAWRITEEND  = 0x00000001
};

enum hisi_pci_interrupt_status_register_bits {
	CPUDOORBELL3 = 0x00800000, CPUDOORBELL2 = 0x00400000, CPUDOORBELL1 = 0x00200000,
	CPUDOORBELL0 = 0x00100000, PDMAREADEND = 0x00000100, PDMAWRITEEND = 0x00000001
};

enum hisi_pci_dma_control_register_bits {
	DMAPCIINTERRPUT = 0x00000008, DMASTOPTRANS = 0x00000002, DMASTARTTRANS = 0x00000001
};

enum hisi_ahb_registers{
	WDMA_PCI_ADDR=0x00,   /*Write DMA start address on the PCI bus*/
	WDMA_AHB_ADDR=0x04,   /*Write DMA transfer start address on the AHB bus*/
	WDMA_CONTROL=0x08,    /*Write DMA size & control*/
        RDMA_PCI_ADDR=0x20,   /*Read DMA start address on the PCI bus*/
        RDMA_AHB_ADDR=0x24,   /*Read DMA transfer start address on the AHB bus*/
        RDMA_CONTROL=0x28,    /*Read DMA size & control*/
        CPU_IMASK =0x40,                         /*Interrupt mask*/
        CPU_ISTATUS=0x44,                       /*Interrupt status*/
        CPU_ICMD=0x48,                           /*Interrupt command*/
        CPU_VERSION=0x4c,                      /*Bridge version and miscellaneous information*/
        CPU_CLKRUN=0x50,                        /*CLKRUN control and status register*/
        CPU_CIS_PTR=0x54,                       /*Cardbus CIS Pointer of PCI configuration space*/
        CPU_PM=0x58,                               /*PMC register and PM state of PCI configuration space*/
        PCIAHB_ADDR_NP=0x70,                 /*PCI-AHB window non-prefetchable range control*/
        PCIAHB_ADDR_PF=0x74,                 /*PCI-AHB window prefetchable range control*/
        PCIAHB_TIMER =0x78,                    /*PCI-AHB window discard timer*/
        AHBPCI_TIMER=0x7c,                     /*AHB-PCI window discard timer*/
        PCI_CONTROL=0x80,                       /*PCI control bits*/
        PCI_DV=0x84,                                /*PCI device and vendor ID*/
        PCI_SUB=0x88,                              /*PCI subsystem device and vendor ID*/
        PCI_CREV=0x8c,                            /*PCI class code and revision ID*/
        PCI_BROKEN=0x90,                        /*PCI arbiter broken master register*/
        PCIAHB_SIZ_NP=0x94,                         /*PCI-AHB window non-prefetchable range size*/
        PCIAHB_SIZ_PF=0x98,                         /*PCI-AHB window prefetchable range size*/
};

enum hisi_pci_registers{
        VENDOR_ID=0x00,                         /*Vendor ID, 16bit*/
        DEVICE_ID=0x02,                          /*Device ID, 16bit*/
        COMMAND=0x04,                           /*Device command, 16bit*/
        STATUS=0x06,                               /*Device status, 16bit*/
        REVISION_ID=0x08,                           /*Revision ID, 8bit*/
        CLASS_CODE=0x09,                               /*Class code, 24bit*/
        CACHELINE_SIZE=0x0c,                         /*Cacheline size, 8bit*/
        MASTER_LATENCY_TIMER=0x0d,                          /*Master latency timer, 8bit*/
        HEADER_TYPE=0x0e,                           /*Header type, 8bit*/
        BIST=0x0f,                               /*Build in self test , 8bit*/
        BAR0=0x10,                           /*Base address 0*/
        BAR1=0x14,                           /*Base address 1*/
        BAR2=0x18,                           /*Base address 2*/
        BAR3=0x1c,                           /*Base address 3, PCI-AHB window non-
prefetchable range size*/
        BAR4=0x20,                           /*Base address 4, PCI-AHB window
prefetchable range size*/
        BAR5=0x24,                           /*Base address 5*/
        CARDBUS_CIS_POINTER=0x28,                               /*Cardbus CIS pointer
,32 bit*/
        SUBSYS_VED_ID=0x2c,                           /*Subsystem vendor ID, 16bit*/
        SUBSYS_ID=0x2e,                           /*Subsystem ID, 16bit*/
        EXP_ROM_BAR=0x30,                           /*Expansion ROM base address*/
        CAP_PTR=0x34,                                  /*Capabilities pointer, 8bit*/
        INTERRUPT_LINE=0x3c,                           /*Interrupt line, 8bit*/
        INTERRUPT_PIN=0x3d,                             /*Interrupt pin, 8bit*/
        MIN_GNT=0x3e,                           /*Device's burst period assuming a clock rate of 33Mhz, 8bit*/
        MAX_LAT=0x3f,                           /*Device's desired settings for latency timer values, 8bit*/
        CBUS_FE=0x68,                           /*Function event*/
        CBUS_FEM=0x6c,                           /*Function event mask*/
        CBUS_FPS=0x70,                           /*Function present state*/
        PCI_FFE=0x74,                           /*Function present state*/
        PMC=0x78,                           /*Power management capabilities*/
        PMCSR=0x7c,                           /*Power management control/status*/
        PCI_IMASK=0x80,                           /*Interrupt mask*/
        PCI_ISTATUS=0x84,                           /*Interrupt status*/
        PCI_ICMD=0x88,                           /*Interrupt command*/
        PCI_VERSION=0x8C,                           /*Bridge version and miscellaneous information host: 2380h; simple: 1380h*/
};
#endif

// hi3511 bridge ahb register access routines
#define hisi_bridge_ahb_writeb(o,v) \
	(writeb(v, PCI_VIRT_BASE + (unsigned long)(o)))
	
#define hisi_bridge_ahb_readb(o) \
	(readb(PCI_VIRT_BASE + (unsigned long)(o)))

#define hisi_bridge_ahb_writew(o,v) \
	(writew(v, PCI_VIRT_BASE + (unsigned long)(o)))
#define hisi_bridge_ahb_readw(o) \
	(readw(PCI_VIRT_BASE + (unsigned long)(o)))

#define hisi_bridge_ahb_writel(o,v) \
	(writel(v, PCI_VIRT_BASE + (unsigned long)(o)))
#define hisi_bridge_ahb_readl(o) \
	({unsigned long __v = readl((PCI_VIRT_BASE + (unsigned int)(o))); __v;})

//interrupt checking
#define hisi_pci_interrupt_check(o) \
	(hisi_bridge_ahb_readl((unsigned long)CPU_ISTATUS)&(unsigned long)(o))
//enable interrupt
#define hisi_pci_interrupt_enable(o) \
	hisi_bridge_ahb_writel(CPU_IMASK,(hisi_bridge_ahb_readl(CPU_IMASK)|(unsigned long)(o)))
//disable interrupt
#define hisi_pci_interrupt_disable(o) \
	(hisi_bridge_ahb_writel(CPU_IMASK,hisi_bridge_ahb_readl(CPU_IMASK)&(unsigned long)(~o)))
//clear interrupt
#define hisi_pci_interrupt_clear(o) \
	(hisi_bridge_ahb_writel(CPU_ISTATUS,(unsigned long)(o)))


struct hisi_pcicom_dev 
{
	struct list_head node;	/* node in list of hi3511 device */
	unsigned long	pf_base_addr;	/* prefetchable space base address of device */
	unsigned long	pf_end_addr;	/* prefetchable space end address of device */
	unsigned long	np_base_addr;	/* non_prefetchable space base address of device */
	unsigned long	np_end_addr;	/* non_prefetchable space end address of device */
	unsigned long	cfg_base_addr;	/* configuration space base address of device */
        unsigned long   shm_phys_addr;
        unsigned int    shm_size;
	unsigned int	irq;		/* irq number of device	*/
	unsigned int	slot_id; 	/* device slot number,marks a device uniquely */	
	struct pci_dev	*pdev;		/* usr pci dev structure */
	struct timer_list timer;	/*  timer list for send data during time */	
	void			(*trigger)(unsigned long slot);	/* enable another side irq signal func */
        unsigned long		(*check_irq)(int slot);
        void			(*enable_irq)(int slot);
        void			(*disable_irq)(int slot);
        void			(*clear_irq)(int slot);
	int			(*ishost)(int slot);	
};

struct pci_vdd_info
{
        unsigned long version;		/* PCI_VDD version */
        unsigned long id;		/* ID of this system, host = 0 */
        unsigned long bus_slot;		/* PCI slot number, host not have this value */
	unsigned long mapped;		/* PCI slave memory mapped flage */
};

typedef struct pci_vdd_info pci_vdd_info_t;
extern pci_vdd_info_t hisi_pci_vdd_info;

struct hisi_pci_dev_list
{
        struct list_head list;
        unsigned int list_num;
};

/**
 *      alloc_pci_dev - allocate pci device
 *      @sizeof_priv:   size of private data to allocate space for
 *      @name:          device name format string
 *      @setup:         callback to initialize device
 *
 *      Allocates a struct hisi_pcicom_dev for driver use
 *      and performs basic initialization.
 */
static inline struct hisi_pcicom_dev *hisi_alloc_pci_dev(void)
{
        struct hisi_pcicom_dev *dev;
        dev= kmalloc(sizeof(struct hisi_pcicom_dev), GFP_KERNEL);
        if (!dev) {
                printk(KERN_ERR "alloc_pci_dev: Unable to allocate device.\n");
                return NULL;
        }
        memset(dev, 0, sizeof(struct hisi_pcicom_dev));
        return dev;
}

#define DEBUG_PCI  0
#if DEBUG_PCI
#define MCC_DBG_LEVEL 0x3
#define mcc_trace(level, s, params...) do{ if(level & MCC_DBG_LEVEL)\
                printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
                }while(0)

#define mcc_trace_func() mmz_trace(0x02,"%s", __FILE__)
#else
#define mcc_trace(level, s, params...)
#endif

#endif

