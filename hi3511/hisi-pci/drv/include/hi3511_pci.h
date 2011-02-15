/*  
 * Hi3511_PCI_DRV/drv/include/hi3511.h
 * Copyright (c) 2006 Hisilicon Co., Ltd. 
 *
 * hi3511.h describes hi3511 device for down level and up level drivers
 * 
 *This program is free software; you can redistribute it and/or modify
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
 *
 */
#ifndef _HI3511_PCI_H_
#define _HI3511_PCI_H_

#define PCI_HAL_DBUG_LEVEL	5
#define PCI_HAL_DEBUG(level, s, params...) do{ if(level >= PCI_HAL_DBUG_LEVEL)\
	printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
	}while(0)

#define OSDRV_MODULE_VERSION_STRING "HISI_PCI-MDC030007 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:52:49"

#include <linux/pci.h>

#define ARM_RESET 0x101e001c
#define SYSTEM_CONTROL_BASE 0x101e0000
#define DDRC_BASE 0x10150000
#define DDRC_CONTROL 0x00//check the hi3511 manual
#define DDRC_DYNAMIC_READCONFIG 0x00//check the hi3511 manual
#define DDRC_DYNAMIC_CONTROL 0x00//check the hi3511 manual
#define DDRC_DYNAMIC_REFRESH 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_RASCAS0 0x00 //check the hi3511 manual
#define DDRC_CONFIG_VALUE 	0x00000884 	/* set DDRC config register ,hi3510*/
#define DDRC_DYNAMIC_CONFIG0 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TRP 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TRAS 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TSREX 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TWR 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TRC 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TRFC 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TXSR 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TRRD 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TMRD 0x00 //check the hi3511 manual
#define DDRC_DYNAMIC_TCDLR 0x00 //check the hi3511 manual




#define PCI_VENDOR_HI3511 0x19e5//0x1556
#define PCI_DEVICE_HI3511  0x3511//0xbb00

#define HI3511_PCI_BASE 0xb0000000
#define HI3511_PCI_PF_BASE 0xb8000000
#define HI3511_PCI_CFG_BASE 0x00000000

#define HI3511_PCI_BASE_SIZE 0x00100000
#define HI3511_PCI_PF_BASE_SIZE 0x00100000
#define HI3511_PCI_CFG_BASE_SIZE 0x001000

#define HI3511_PCI_PF_VIRT_BASE 0xe9000000 //not decided,used to access config space of device
#define HI3511_PCI_CFG_VIRT_BASE 0xe9000000
#define HI3511_PCI_VIRT_BASE IO_ADDRESS(HISILICON_PCI_BR_BASE)  /*not decided*/
#define HI3511_PCI_VIRT_END  0xe8000400  /*not decided*/

#define HI3511_PCI_MEM_BASE0 0xb0010000 /*not decided*/
#define HI3511_PCI_MEM_BASE0_SIZE 0x00010000 /*not decided*/
#define HI3511_PCI_MEM_BASE1 0xb0020000 /*not decided*/
#define HI3511_PCI_MEM_BASE1_SIZE 0x07fe0000 /*not decided*/
#define HI3511_PCI_MEM_BASE2 0xb8000000 /*not decided*/
#define HI3511_PCI_MEM_BASE2_SIZE 0x08000000 /*not decided*/
#define PCI_WINDOW_SIZE 0xff000000/*not decided*/
#define PCI_SLAVE_SHAREMEM_BASE 0xe5000000/*not decided*/


/*DMA control COMMAND*/
#define DMAR_INT_ACK (0x00&0xf0)
#define DMAR_IO (0x20&0xf0)
#define DMAR_MEM (0x60&0xf0)
#define DMAR_CONF (0xa0&0xf0)
#define DMAR_MEM_MULTI (0xc0&0xf0)
#define DMAW_IO (0x30&0xf0)
#define DMAW_MEM (0x70&0xf0)
#define DMAW_CONF (0xb0&0xf0)
#define DMAW_SPC_CYC (0x10&0xf0)
#define DMAC_INT_EN (0x01<<3)
#define DMAC_STOP (0x01<<1)
#define DMAC_START (0x01)


// hi3511 bridge ahb register access routines
#define hi3511_bridge_ahb_writeb(o,v) \
	(writeb(v, HI3511_PCI_VIRT_BASE + (unsigned long)(o)))
	
#define hi3511_bridge_ahb_readb(o) \
	(readb(HI3511_PCI_VIRT_BASE + (unsigned long)(o)))

#define hi3511_bridge_ahb_writew(o,v) \
	(writew(v, HI3511_PCI_VIRT_BASE+ (unsigned long)(o)))
#define hi3511_bridge_ahb_readw(o) \
	(readw(HI3511_PCI_VIRT_BASE + (unsigned long)(o)))

#define hi3511_bridge_ahb_writel(o,v) \
	(writel(v, HI3511_PCI_VIRT_BASE + (unsigned long)(o)))
#define hi3511_bridge_ahb_readl(o) \
	({unsigned long __v = readl((HI3511_PCI_VIRT_BASE + (unsigned int)(o))); __v;})

//interrupt checking
#define interrupt_check(o) \
	(hi3511_bridge_ahb_readl((unsigned long)CPU_ISTATUS)&(unsigned long)(o))

//enable interrupt
#define interrupt_enable(o) \
	hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)|(unsigned long)(o)))

//disable interrupt
#define interrupt_disable(o) \
	(hi3511_bridge_ahb_writel(CPU_IMASK,hi3511_bridge_ahb_readl(CPU_IMASK)&(unsigned long)(~o)))

//clear interrupt
#define interrupt_clear(o) \
do{\
    int i;\
    for(i=0; i<4; i++)\
    {\
        if(((o)>>(i*8)) & 0xFF)\
        {\
	        hi3511_bridge_ahb_writeb(CPU_ISTATUS+i,(unsigned long)((o)>>(i*8)));\
        }\
    }\
}while(0)


/* give to recv in opt */
#define Hi3511_pci_head_target(head)  (head->target & 0xF)
#define Hi3511_pci_head_src(head)  (head->src & 0xF)
#define Hi3511_pci_head_msg(head)  (head->msg & 0xF)
#define Hi3511_pci_head_port(head)  (head->port & 0xF)
#define Hi3511_pci_head_length(head)  (head->length & 0xFFFFF)
#define Hi3511_pci_head_oob(head) (head->oob & 0xFFF)

#define HI_PCIT_IS_HOST hi3511_bridge_ahb_readl((CPU_VERSION))&0x00002000

//void DISABLE_INT_HI_SLOT(int slot);
void disable_int_hi_slot(int slot);
//void DISABLE_INT_HI_DMA(void);
void disable_int_hi_dma(void);
//void CLEAR_INT_HI_SLOT(int slot);
void clear_int_hi_slot(int slot);
//void CLEAR_INT_HI_DMA(void); 
void clear_int_hi_dma(void); 
//void ENABLE_INT_HI_SLOT(int slot);
void enable_int_hi_slot(int slot);
//void ENABLE_INT_HI_DMA(void);
void enable_int_hi_dma(void);
void enable_int_dma_hi_slot(int slot);
void disable_int_dma_hi_slot(int slot);
void clear_int_dma_hi_slot(int slot);
unsigned long read_int_hi_slot(int slot);

//#define hi3511_slave_interrupt_flag 0xe5a00000 //delete for dynamic alloc
#define hi3511_slave_interrupt_flag_size  (0x400)

#define MDI_DMA_FLAG 0xe5a00004
#define MDI_DMA_FLAG_SIZE  (0x4)



int hi3511_interrupt_check(void);
int hi3511_dma_check(void);
int hi3511_window_move(int slot, unsigned long addr, int size);
int hi3511_window_restore(int slot, unsigned long saved_addr);

void master_to_slave_irq(unsigned long slot);
void slave_to_master_irq(unsigned long slot);

unsigned int move_np_window_write(void *register_addr, unsigned int slot, 
unsigned long val);
unsigned int move_np_window_read(void *register_addr, unsigned int slot, 
unsigned int *val);

#define vether_rcv_get_len(v)  (((struct Hi3511_pci_data_transfer_head *) (v))->length) 
#define vether_rcv_get_oob(v)  (((struct Hi3511_pci_data_transfer_head *) (v))->oob)
#define pci_dma_push_fifo  1

struct hi3511_dev *alloc_hi3511dev(void);

enum hi3511_ahb_registers{
	WDMA_PCI_ADDR=0x00,                 /*Write DMA start address on the PCI bus*/
	WDMA_AHB_ADDR=0x04,                /*Write DMA transfer start address on the 
AHB bus*/
	WDMA_CONTROL=0x08,                  /*Write DMA size & control*/
	RDMA_PCI_ADDR=0x20,                  /*Read DMA start address on the PCI bus*/
	RDMA_AHB_ADDR=0x24,                 /*Read DMA transfer start address on the 
AHB bus*/
	RDMA_CONTROL=0x28,                   /*Read DMA size & control*/	
	CPU_IMASK =0x40,                         /*Interrupt mask*/
	CPU_ISTATUS=0x44,                       /*Interrupt status*/
	CPU_ICMD=0x48,                           /*Interrupt command*/
	CPU_VERSION=0x4c,                      /*Bridge version and miscellaneous 
information*/	
	CPU_CLKRUN=0x50,                        /*CLKRUN control and status register*/
	CPU_CIS_PTR=0x54,                       /*Cardbus CIS Pointer of PCI 
configuration space*/
	CPU_PM=0x58,                               /*PMC register and PM state of 
PCI configuration space*/
	PCIAHB_ADDR_NP=0x70,                 /*PCI-AHB window non-prefetchable range 
control*/	
	PCIAHB_ADDR_PF=0x74,                 /*PCI-AHB window prefetchable range 
control*/	
	PCIAHB_TIMER =0x78,                    /*PCI-AHB window discard timer*/
	AHBPCI_TIMER=0x7c,                     /*AHB-PCI window discard timer*/
	PCI_CONTROL=0x80,                       /*PCI control bits*/
	PCI_DV=0x84,                                /*PCI device and vendor ID*/	
	PCI_SUB=0x88,                              /*PCI subsystem device and vendor 
ID*/
	PCI_CREV=0x8c,                            /*PCI class code and revision ID*/
	PCI_BROKEN=0x90,                        /*PCI arbiter broken master register*/
	PCIAHB_SIZ_NP=0x94,                         /*PCI-AHB window non-
prefetchable range size*/
	PCIAHB_SIZ_PF=0x98,                         /*PCI-AHB window prefetchable 
range size*/

	};

enum hi3511_pci_registers{
	VENDOR_ID=0x00,                         /*Vendor ID, 16bit*/
	DEVICE_ID=0x02,                          /*Device ID, 16bit*/
	COMMAND=0x04,                           /*Device command, 16bit*/
	STATUS=0x06,                               /*Device status, 16bit*/
	REVISION_ID=0x08,                           /*Revision ID, 8bit*/
	CLASS_CODE=0x09,                               /*Class code, 24bit*/
	CACHELINE_SIZE=0x0c,                         /*Cacheline size, 8bit*/
	MASTER_LATENCY_TIMER=0x0d,                          /*Master latency timer, 
8bit*/
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
	MIN_GNT=0x3e,                           /*Device's burst period assuming a 
clock rate of 33Mhz, 8bit*/
	MAX_LAT=0x3f,                           /*Device's desired settings for 
latency timer values, 8bit*/
	CBUS_FE=0x68,                           /*Function event*/
	CBUS_FEM=0x6c,                           /*Function event mask*/
	CBUS_FPS=0x70,                           /*Function present state*/
	PCI_FFE=0x74,                           /*Function present state*/
	PMC=0x78,                           /*Power management capabilities*/
	PMCSR=0x7c,                           /*Power management control/status*/
	PCI_IMASK=0x80,                           /*Interrupt mask*/
	PCI_ISTATUS=0x84,                           /*Interrupt status*/
	PCI_ICMD=0x88,                           /*Interrupt command*/
	PCI_VERSION=0x8C,                           /*Bridge version and 
miscellaneous information
	                                                                 host: 2380h
; simple: 1380h*/
};

enum hi3511_cpu_interrupt_status_register_bits {
	PMEINT  = 0x80000000,CLKRUNINT  = 0x40000000, PMSTATINT  = 0x20000000,
	PCISERR  = 0x10000000, PCIINT3 = 0x08000000, PCIINT2 = 0x04000000,
	PCIINT1 = 0x02000000, PCIINT0 = 0x01000000,  PCIDOORBELL3 = 0x00080000,
	PCIDOORBELL2 = 0x00040000,PCIDOORBELL1 = 0x00020000, PCIDOORBELL0 = 
0x00010000, 
	APWINDISCARD = 0x00004000, APWINFTCHERR = 0x00002000, APWINPSTERR = 
0x00001000,
	DMAREADAHBERR = 0x00000800, DMAREADPERR  = 0x00000400, DMAREADABORT  = 
0x00000200,
	DMAREADEND  = 0x00000100,    PAWINDISCARD  = 0x00000040,
	PAWINFTCHERR = 0x00000020, PAWINPSTERR = 0x00000010, DMAWRITEAHBERR = 
0x00000008,
	DMAWRITEPERR  = 0x00000004, DMAWRITEABORT = 0x00000002, DMAWRITEEND  = 
0x00000001
};

enum hi3511_pci_interrupt_status_register_bits {
	CPUDOORBELL3  = 0x00800000, CPUDOORBELL2 = 0x00400000, CPUDOORBELL1  = 
0x00200000,
	CPUDOORBELL0  = 0x00100000,  PDMAREADEND  = 0x00000100, PDMAWRITEEND  = 
0x00000001
};

enum hi3511_pci_dma_control_register_bits {
	DMAPCIINTERRPUT  = 0x00000008,  DMASTOPTRANS  = 0x00000002, DMASTARTTRANS  = 
0x00000001
};

enum hi3511_interrupts{
	HI_SLOT1=0,
	HI_SLOT2,
	HI_SLOT3,
	HI_SLOT4,
	HI_DMA_END,
	HI_HOST,
	HI_SLOT11=10,
	HI_SLOT12,
	HI_SLOT13,
	HI_SLOT14,
	HI_SLAVE_UNMAP=0x10,
	HI_NONE
	//dma error
};

enum hi3511_DMA_BUSY{
	WRITEBUSY=0,
	READBUSY,
	NOTBUSY
};

struct   hi3511_dev 
{
	struct list_head node;		/* node in list of hi3511 device */
	int		slot;           /*device slot number,marks a device uniquely*/	
	unsigned long	shared_mem_end;	/* end of shared memory */
	unsigned long	shared_mem_start;/* start of shared memory */
	unsigned long	pf_base_addr;	/* prefetchable space base address of device */
	unsigned long	pf_end_addr;	/* prefetchable space end address of device */
	unsigned long	np_base_addr;	/* non_prefetchable space base address of device */
	unsigned long		np_end_addr;	/* non_prefetchable space end address of device */
	unsigned long		cfg_base_addr;	/* configuration space base address of device */
	unsigned long		np_base_addr_phy;	/*  np space base physics address of device */
	unsigned long		pf_base_addr_phy;	/*  pf space base physics address of device */
	unsigned long		cfg_base_addr_phy;	/* configuration space base physics address of device */

        unsigned long           shm_phys_addr;
        unsigned int            shm_size;
	unsigned int		irq;		/* irq number of device	*/
	int			(*write ) (void *pci_addr_virt,void *ahb_addr_phy, int len, unsigned int slot);/*hook for hi3511 device write*/
	int			(*read  ) (void *ahb_addr_phy, void *pci_addr_virt, int len,unsigned  int slot);/*hook for hi3511 device read*/
	void                            (*trigger)(unsigned long slot);
	unsigned long			(*check_doorbell_irq)(int slot);
	void 				(*enable_doorbell_irq)(int slot);
	void 				(*disable_doorbell_irq)(int slot);
	void 				(*clear_doorbell_irq)(int slot);
	int                             (*is_host)(int slot);
	struct pci_dev      *pdev;
}; 

struct mem_addr_couple
{
unsigned long base_addr_phy;
unsigned long base_addr_virt;
struct pci_dev      *pdev;

};

static inline void hi3511_pci_dma_read (unsigned long pci_phy,unsigned long ahb_phy,int len)
{
		unsigned long control;

		control = (len<<8)| 0x61;

		hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pci_phy);

		hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, ahb_phy);

		hi3511_bridge_ahb_writel(RDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);

}

static inline void hi3511_pci_dma_write (unsigned long pci_phy,unsigned long ahb_phy,int len)
{
		unsigned long control;

		control = (len<<8)| 0x71;

		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pci_phy);

		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, ahb_phy);

		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

		while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);	
		
		if(hi3511_bridge_ahb_readl(CPU_ISTATUS) & 0x02){
			hi3511_bridge_ahb_writel(CPU_ISTATUS, 0x002);
			hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pci_phy);

			hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, ahb_phy);

			hi3511_bridge_ahb_writel(WDMA_CONTROL, control);

			while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);	
		
			if(hi3511_bridge_ahb_readl(CPU_ISTATUS) & 0x02){
				printk("PCI aboart occuring[%s %d]:\n",__FUNCTION__, __LINE__);
				printk("pci_phy is 0x%lx, ahb_phy is 0x%lx \n", pci_phy, ahb_phy);
				hi3511_bridge_ahb_writel(CPU_ISTATUS, 0x002);
				dump_stack();
			}
		}
}

#ifdef CONFIG_PCI_HOST
static inline void pci_dma_lock_test(unsigned long addr)
{
#ifdef  CONFIG_DMA_CPUS_LOCK
	int i = 0,j = 0;
	do{
		do{
			writeb(0x1,(char *)addr + 3);

			if (readb((char *)addr + 2) == 1)	{
				writeb(0x0,(char *)addr + 3);
			}
			else 
				break;

			udelay(2);
	
			i++;
	
			if(i > 300000){
				i = 0;
				printk("1 DMA waiting for too long!\n");
			}	

		}while(1);

//		writeb(0x1,(char *)addr + 3);

		if (readb((char *)addr) == 1){
			writeb(0x0,(char *)addr + 3);
		}
		else 
			break;

		udelay(1);
	
		j++;

		if(j > 300000){
			j = 0;
			printk("2 DMA waiting for too long!\n");
		}
	}while(1);

	writeb(0x1, (char *)addr);
#endif	
}

static inline void pci_dma_lock_free(unsigned long addr)
{
#ifdef  CONFIG_DMA_CPUS_LOCK
	writeb(0x0, (char *)addr);
	writeb(0x0, (char *)addr + 3);
#endif	
}
#else
static inline void pci_dma_lock_test(unsigned long addr)
{
#ifdef  CONFIG_DMA_CPUS_LOCK
	int i = 0,j = 0;
	do{
		do{
			writeb(0x1,(char *)addr + 2);

			if (readb((char *)addr + 3) == 1)	{
				writeb(0x0,(char *)addr + 2);
			}
			else 
				break;

			udelay(2);
	
			i++;
	
			if(i > 300000){
				i = 0;
				printk("1 DMA waiting for too long!\n");
			}	

		}while(1);

//		writeb(0x1,(char *)addr + 3);

		if (readb((char *)addr) == 1){
			writeb(0x0,(char *)addr + 2);
		}
		else 
			break;

		udelay(1);
	
		j++;

		if(j > 300000){
			j = 0;
			printk("2 DMA waiting for too long!\n");
		}
	}while(1);

	writeb(0x1, (char *)addr);
#endif	
}

static inline void pci_dma_lock_free(unsigned long addr)
{
#ifdef  CONFIG_DMA_CPUS_LOCK
	writeb(0x0, (char *)addr);
	writeb(0x0, (char *)addr + 2);
#endif	
}
#endif
#endif

