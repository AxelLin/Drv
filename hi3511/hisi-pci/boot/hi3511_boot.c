/* 
*hi3511_boot.c: A module for hi3511 slave device booting
*Copyright 2006 hisilicon Corporation 
*/

#include <linux/module.h>

#include <linux/device.h>
#include <linux/mempolicy.h>

#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/vmalloc.h>


#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/crc32.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>

#include <asm/processor.h>      /* Processor type for cache alignment. */
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>	/* User space memory access functions */

#include "../drv/include/hi3511_pci.h"

#include <linux/kcom.h>
#include <kcom/mmz.h>
struct kcom_object *kcom;
struct kcom_media_memory *kcom_media_memory;

#define HI3511_BOOT_MODULE_NAME "hi3511"
#define HI3511_BOOT_VERSION "v0.0"
#define IMAGEFILE_BUF_LEN 0xa00000

//hil_mmb_t *boot_pci_dma_zone_base;
//unsigned char *boot_pci_dma_zone_virt_base;

unsigned long boot_pci_dma_zone_base;
void *boot_pci_dma_zone_virt_base;
void *pfmemaddr_virt;

static char version[] __devinitdata =
KERN_INFO "hi3511_boot.c: " HI3511_BOOT_VERSION "\n";
unsigned int *boot_image;

static struct pci_device_id hi3511_pci_tbl[] =
{
	{0x1556, 0xbb00, PCI_ANY_ID, PCI_ANY_ID, 0, 0, },
	{}
};

MODULE_DEVICE_TABLE (pci, hi3511_pci_tbl);

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("hi3511 boot");
MODULE_LICENSE("GPL");

static unsigned long npmemaddr;
static unsigned long pfmemaddr;

//register_addr: intended registers' address
//slot: target device
//val:value to write
unsigned int move_np_window_write(void *register_addr, unsigned int slot, unsigned long val)
{
	unsigned long intend_addr;
	int devfn,i;
	unsigned long *pb4addr, *pb3addr;
	unsigned long b4addr,b3addr;
	pb4addr=&b4addr;
	pb3addr=&b3addr;
	PCI_HAL_DEBUG(0, "ok, move or not %d", 1);
	devfn=PCI_DEVFN(slot,0);
//	printk("windows write 0x%8x\n",register_addr);

	intend_addr=((unsigned long)register_addr&0xffff0000UL)|0x00000001UL;
	*(volatile unsigned int *)boot_pci_dma_zone_virt_base=intend_addr;
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);
	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);
	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);
	for(i=0; i<100 ; i++) {
		udelay(100);
		};

	//*(volatile unsigned int *)(HI3511_PCI_PF_VIRT_BASE+((unsigned long)register_addr&0x0000ffff))=val;
	*(volatile unsigned int *)(pfmemaddr_virt+((unsigned long)register_addr&0x0000ffff))=val;
	for(i=0; i<100 ; i++) {
		udelay(100);
		};

	return 0;
}

unsigned int  move_np_window_read(void *register_addr, unsigned int slot, unsigned int *val)
{
	unsigned long intend_addr;
	int i;
	int devfn;
	unsigned long *pb4addr, *pb3addr;
	unsigned long b4addr,b3addr;
	pb4addr=&b4addr;
	pb3addr=&b3addr;
	devfn=PCI_DEVFN(slot,0);
	PCI_HAL_DEBUG(0, "ok, move or not %d", 2);
	
	intend_addr=((unsigned long)register_addr&0xffff0000UL)|0x00000001UL;

	*(unsigned int *)boot_pci_dma_zone_virt_base=intend_addr;
	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);
	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);
	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);
	for(i=0; i<100 ; i++) {
			udelay(100);
		};

	*val = __raw_readl(pfmemaddr_virt+((unsigned long)register_addr&0x0000ffff));
	//*val=*(unsigned int *)boot_pci_dma_zone_virt_base;

	for(i=0; i<100 ; i++) {
			udelay(100);
		};


	return 0;
}

//the interface function for slave boot.
unsigned int  pci_slave_load(const char  *fn, unsigned int  slot)
{
	int i;
	char      *addr;
	unsigned int regval=0;     
	struct timeval tv1,tv2;
	unsigned long transfer_time;
	//unsigned int len=0;
 	unsigned int devfn;
 	unsigned int arm_reset=0;
 	loff_t      pos = 0;
 	mm_segment_t old_fs;
	struct file* fp;
 	unsigned int  count;
 	unsigned long *pb4addr, *pb3addr;
	unsigned long b4addr,b3addr;
	// sysctl
	unsigned long base = 0x101e0000;
	
	char  *pFileBuff = (char *)vmalloc(IMAGEFILE_BUF_LEN);
	
	pb4addr=&b4addr;
	
	pb3addr=&b3addr;
		
	PCI_HAL_DEBUG(0, "ok, loading  %d", 1);
	
	devfn=PCI_DEVFN(slot,0);

	addr =(char *)(0xb0000000 + 0x098);

	regval=0xff000000;

	move_np_window_write(addr, slot, regval); //sizeof pf

	move_np_window_write((void *)(base + 0x018), slot, 0x00000b09); 

	udelay(100);

	move_np_window_write((void *)(base + 0x014), slot, 0xff96880); 
	move_np_window_write((void *)(base ), slot, 0x00000214); 
	udelay(100);
	regval = (1<<12);
	
	move_np_window_write((void *)(base + 0x1c), slot, regval);
	
	base = 0x10150000;
	/* exit self-refresh and bring CKE high */
	move_np_window_write((void *)(base + 0x0004), slot, 0x00000000); 
	
	udelay(100);
	/* config the DDR mode register mrs: 0532   emrs: 446 (maybe it need modify the Rtt)*/
	move_np_window_write((void *)(base + 0x0008), slot, 0x04460542); 
	/* config the DDR extend mode register emrs2: 0000 emrs3: 0000*/
//	move_np_window_write((void *)(base + 0x000c), slot, 0x00000000); 
	/* config DDR config register iDDR2 32bit,row 13,col 10, bank 3*/
	move_np_window_write((void *)(base + 0x0010), slot, 0x800470a2);
        /* config DDR  timing parameter */
	move_np_window_write((void *)(base + 0x0020), slot, 0x22330806);
	
	move_np_window_write((void *)(base + 0x0024), slot, 0xc8023412);

	move_np_window_write((void *)(base + 0x0028), slot, 0x3230703a);

	move_np_window_write((void *)(base + 0x002c), slot, 0x00000802);
        /* enable odt (use the default value) */
//	move_np_window_write((void *)(base + 0x0040), slot, 0x00000003);
        /* config trian/track */
	move_np_window_write((void *)(base + 0x0044), slot, 0xf200fff6);
        /* config port pri port0-port7 (98-b4) VO */
	move_np_window_write((void *)(base + 0x0098), slot, 0x00000000);
        /* VI */
	move_np_window_write((void *)(base + 0x009c), slot, 0x00000001);
        /* DMA */
	move_np_window_write((void *)(base + 0x00a0), slot, 0x00000002);
        /* EXP */
	move_np_window_write((void *)(base + 0x00a4), slot, 0x00000003);
        /* ARMD */
	move_np_window_write((void *)(base + 0x00a8), slot, 0x00000005);
        /* ARMI */
	move_np_window_write((void *)(base + 0x00ac), slot, 0x00000006);
    	/* DSU */
	move_np_window_write((void *)(base + 0x00b0), slot, 0x00000007);
    	/* VEDU */
	move_np_window_write((void *)(base + 0x00b4), slot, 0x00000004);
    	/* start initialization */
	move_np_window_write((void *)(base + 0x0004), slot, 0x00000002);
	//INITDATA_END(DDRC)

	//open image file
	if(pFileBuff==NULL){

        return -ENOMEM;
	}

	memset(pFileBuff,0,IMAGEFILE_BUF_LEN);

	old_fs = get_fs();	

	set_fs(KERNEL_DS);

	fp = filp_open(fn, O_RDONLY, 0);

	if(IS_ERR(fp) ){

		set_fs(old_fs);

		vfree(pFileBuff);

		printk("<ERROR>filp_open error,please check the image file is correct!!!!\n");

		return IS_ERR(fp);
	}

	count = fp->f_op->read(fp,pFileBuff,IMAGEFILE_BUF_LEN,&pos);

	set_fs(old_fs);

	//load image......

	*(volatile unsigned int *)boot_pci_dma_zone_virt_base=0x00000001;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);

	for(i=0; i<10 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {

		udelay(1);

	};

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001){

	printk(KERN_ERR "pci_config_dma_read timeout!\n");

	}

	for(i=0; i<100 ; i++) {

		udelay(100);

	};

	memcpy(boot_pci_dma_zone_virt_base,pFileBuff,IMAGEFILE_BUF_LEN);

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pfmemaddr);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	do_gettimeofday(&tv1);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x80000071);

	for(i=0; i<10000 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {

		udelay(2000);

	};

	do_gettimeofday(&tv2);

	transfer_time=(1000000 * tv2.tv_sec + tv2.tv_usec)-(1000000 * tv1.tv_sec + tv1.tv_usec);

	printk( "Transfer time is %d uS\n", (int)transfer_time);

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)

		printk(KERN_ERR "pci_config_dma_read timeout!\n");

	for(i=0; i<100 ; i++) {

		udelay(100);

	};

 	vfree(pFileBuff);

#if 1
	addr =(char *)0xb0000000;

	regval=0x35110100;

	move_np_window_write(addr, slot, regval);

//start the slave  arm     
	arm_reset |=0x00101000;
	
	addr =(char *)ARM_RESET;

	regval=arm_reset;

	move_np_window_write(addr, slot, regval);
#endif

	printk(KERN_ERR "data transfered!\n");

	return 0;
	
}
EXPORT_SYMBOL(pci_slave_load);

//ko
static int __init hi3511_boot_init_module(void)
{
	
	unsigned int slotnum=0;

	PCI_HAL_DEBUG(0, "ok, boot start %d", 1);
printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
/* when a module, this is printed whether or not devices are found in probe */
#ifdef MODULE

	printk(version);

#endif
	kcom_getby_uuid(UUID_MEDIA_MEMORY, &kcom);

	if(kcom ==NULL) {
		printk(KERN_ERR "MMZ: can't access mmz, start mmz service first.\n");
		return -1;
	}

	kcom_media_memory = kcom_to_instance(kcom, struct kcom_media_memory, kcom);
	
	boot_pci_dma_zone_base = kcom_media_memory->new_mmb("pciboot-dma-buffer", SZ_16M, 0, NULL);

	if(boot_pci_dma_zone_base == 0){

		PCI_HAL_DEBUG(0, "hil_mmb_alloc error %d", 1);
		
		return -1;
	}

	boot_pci_dma_zone_virt_base = kcom_media_memory->remap_mmb(boot_pci_dma_zone_base);

	memset(boot_pci_dma_zone_virt_base ,0x0, SZ_16M);

	*(volatile unsigned long *)boot_pci_dma_zone_virt_base=0x000001ff;

	printk(KERN_ERR "boot_pci_dma_zone_virt_base=%x \n",*(unsigned int *)boot_pci_dma_zone_virt_base);
	
	//dma_write(0x00040004, boot_pci_dma_zone_base, 4);

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, 0x00010004);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x000004b1);

	while(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001);

	//dma_write(0x0004001c, boot_pci_dma_zone_base, );

	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, 0x0001001c);

	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR,  boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(RDMA_CONTROL, 0x000004a1);

	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);    

	npmemaddr =__raw_readl((volatile unsigned long  *)(boot_pci_dma_zone_virt_base));

	hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, 0x00010020);
	   
	hi3511_bridge_ahb_writel(RDMA_AHB_ADDR,	boot_pci_dma_zone_base);
	  
	hi3511_bridge_ahb_writel(RDMA_CONTROL, 0x000004a1);
	   
	while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);    
	   
	pfmemaddr =(__raw_readl((volatile unsigned long  *)(boot_pci_dma_zone_virt_base)))&0xfffffff0;

	printk("pfmemaddr is %lx \n",pfmemaddr);
	
	pfmemaddr_virt=ioremap(pfmemaddr, 0xa00000);

	if (pfmemaddr_virt == NULL) return -ENOMEM;
		
       for (;slotnum <1;slotnum++){

//	pci_slave_load("/mnt/pci_problem/u-boot-hi3511_fpga_v5_50mhz.bin", slotnum);
	pci_slave_load("/mnt/asic/u-boot-hi3511v100Dmeb_132MHZ.bin", slotnum);

	}

	return 0;

}

static void __exit hi3511_boot_cleanup_module(void)
{

//	hil_mmb_free(boot_pci_dma_zone_base);
	iounmap(pfmemaddr_virt);

	kcom_media_memory->unmap_mmb(boot_pci_dma_zone_virt_base);

	kcom_media_memory->delete_mmb(boot_pci_dma_zone_base);

	kcom_put(kcom);

	PCI_HAL_DEBUG(0, "ok, exit boot %d", 1);	
}

module_init(hi3511_boot_init_module);

module_exit(hi3511_boot_cleanup_module);

MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);

