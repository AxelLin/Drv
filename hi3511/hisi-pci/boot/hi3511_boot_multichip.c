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

#include "hi3511_pci.h"
#include <linux/kcom.h>
#include <kcom/mmz.h>

#define HI3511_BOOT_MODULE_NAME "hi3511_boot_multichip"
#define HI3511_BOOT_VERSION     "v2.0"

#define UBOOT_FILE_NAME  "u-boot.bin"
#define KERNEL_FILE_NAME "kernel.img"
#define FSIMG_FILE_NAME  "cramfs.initrd.img"

#define UBOOT_FILE_MAXLEN  0x100000  /* U-boot no larger than 1MB            */
#define KERNEL_FILE_MAXLEN 0x200000  /* Linux Kernel no larger than 2MB      */
#define FSIMG_FILE_MAXLEN  0x400000  /* File System image no larger than 4MB */


struct kcom_object       *kcom;
struct kcom_media_memory *kcom_media_memory;

unsigned long        boot_pci_dma_zone_base;
void                *boot_pci_dma_zone_virt_base;

static unsigned long npmemaddr;
void                *npmemaddr_virt;

static unsigned long pfmemaddr;
void                *pfmemaddr_virt;

static char version[] __devinitdata =
    KERN_INFO "" HI3511_BOOT_MODULE_NAME "" HI3511_BOOT_VERSION "\n";

unsigned long slot4jinlei = 0;

static char *path   = "/hisi-pci/";
static int  slotuse = 1;
static int  ddrsize = 128;

static struct pci_device_id hi3511_pci_tbl [] = {
        {PCI_VENDOR_HI3511, PCI_DEVICE_HI3511,
         PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
        {0,}
};

MODULE_DEVICE_TABLE (pci, hi3511_pci_tbl);

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("hi3511 boot");
MODULE_LICENSE("GPL");


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

	intend_addr=((unsigned long)register_addr&0xffff0000UL)|0x00000001UL;

	*(volatile unsigned int *)boot_pci_dma_zone_virt_base=intend_addr;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);

	for(i=0; i<100; i++) {
		udelay(100);
	};

	//*(volatile unsigned int *)(HI3511_PCI_PF_VIRT_BASE+((unsigned long)register_addr&0x0000ffff))=val;
	*(volatile unsigned int *)(pfmemaddr_virt+((unsigned long)register_addr&0x0000ffff))=val;

	for(i=0; i<100; i++) {
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

	//printk("windows read 0x%8x\n",register_addr);

	intend_addr=((unsigned long)register_addr&0xffff0000UL)|0x00000001UL;

	*(unsigned int *)boot_pci_dma_zone_virt_base = intend_addr;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr + 0x74);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);

	for(i=0; i<100 ; i++) {
		udelay(100);
	};
	//*val = __raw_readl(HI3511_PCI_PF_VIRT_BASE+((unsigned long)register_addr&0x0000ffff));
	//printk("pfmemaddr_virt is 0x%lx \n",pfmemaddr_virt+((unsigned long)register_addr&0x0000ffff));

	*val = __raw_readl(pfmemaddr_virt + ((unsigned long)register_addr&0x0000ffff));
	//*val=*(unsigned int *)boot_pci_dma_zone_virt_base;

	for(i=0; i<100; i++) {
		udelay(100);
	};


	return 0;
}

//the interface function for slave boot.
// Attention: The image size must be little than 2.3M
unsigned int  pci_slave_load(const char  *filedir, unsigned int  slot)
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

	char fn[100];

	struct file *fp_boot, *fp_kernel, *fp_initrd;

	unsigned int  count;

	unsigned long *pb4addr, *pb3addr;

	unsigned long b4addr,b3addr;
	// sysctl
	unsigned long base = 0x101e0000;

	char  *pFileBuff, *pFileBuff1, *pFileBuff2; 

	pb4addr=&b4addr;

	pb3addr=&b3addr;

	PCI_HAL_DEBUG(0, "ok, loading  %d", 1);

	devfn=PCI_DEVFN(slot,0);

	addr =(char *)(0xb0000000 + 0x098);

	regval=0xff000000;
	printk("before set system controller!!\n");
	move_np_window_write(addr, slot, regval); //sizeof pf

	//move_np_window_write((void *)(base + 0x018), slot, 0x00000022); 
	move_np_window_write((void *)(base + 0x018), slot, 0x00000c09); 

	udelay(100);

	move_np_window_write((void *)(base + 0x014), slot, 0xff96880); 

	move_np_window_write((void *)(base ), slot, 0x00000214); 

	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);

	regval = (1<<12);

	move_np_window_write((void *)(base + 0x1c), slot, regval);

	/*	move_np_window_read((void *)base, slot, &regval);
		printk("after window read\n");
		regval |= 0x07;
		move_np_window_write((void *)base, slot, regval);
		do{
		int i;
		move_np_window_read((void *)base, slot, &regval);
		udelay(100);
		i++;
		if (i >= 1000)
		printk("something wrong ,can not set arm to normal mode!! \n");
		}
		while((regval & 0x78) == 0x20);
	 */	
	base = 0x10150000;
	/* exit self-refresh and bring CKE high */
	move_np_window_write((void *)(base + 0x0004), slot, 0x00000000); 
	udelay(100);

	printk("before set DDR controller!!\n");
	/* config the DDR mode register mrs: 0532   emrs: 446 (maybe it need modify the Rtt)*/
	move_np_window_write((void *)(base + 0x0008), slot, 0x04460542); 
	/* config the DDR extend mode register emrs2: 0000 emrs3: 0000*/
	//	move_np_window_write((void *)(base + 0x000c), slot, 0x00000000); 

	/* config DDR config register iDDR2 32bit,row 13,col 10, bank 3*/
	if(ddrsize == 256)
	{
	    /* 256MB = 1Gbit */
    	move_np_window_write((void *)(base + 0x0010), slot, 0x800470a2);
	}
	else if(ddrsize == 128)
	{
	    /* 128MB = 512Mbit*/
    	move_np_window_write((void *)(base + 0x0010), slot, 0x80047022);
	}
	else
	{
	    /* you should modify this code if ddr size is other value */
	    printk("Don't known how to config the DDR\n");
	    return -1;
	}
	
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
	move_np_window_write((void *)(base + 0x00a8), slot, 0x00000006);
	/* ARMI */
	move_np_window_write((void *)(base + 0x00ac), slot, 0x00000007);
	/* DSU */
	move_np_window_write((void *)(base + 0x00b0), slot, 0x00000005);
	/* VEDU */
	move_np_window_write((void *)(base + 0x00b4), slot, 0x00000004);
	/* start initialization */
	move_np_window_write((void *)(base + 0x0004), slot, 0x00000002);
	//INITDATA_END(DDRC)

    /*---------------- Load U-boot Image---------------------------*/
	pFileBuff = (char *)vmalloc(UBOOT_FILE_MAXLEN);
	if(pFileBuff==NULL){

		return -ENOMEM;
	}

	memset(pFileBuff, 0, UBOOT_FILE_MAXLEN);

	old_fs = get_fs();	

	set_fs(KERNEL_DS);
	
	sprintf(fn,"%s%s", filedir, UBOOT_FILE_NAME);
	printk("%s will be load\n",fn);

	fp_boot = filp_open(fn, O_RDONLY, 0);
	if(IS_ERR(fp_boot) ){

		set_fs(old_fs);

		vfree(pFileBuff);

		printk("<ERROR>filp_open error,please check the image file is correct!!!!\n");

		return IS_ERR(fp_boot);
	}
	count = fp_boot->f_op->read(fp_boot,pFileBuff, UBOOT_FILE_MAXLEN, &pos);
	pos = 0;	
	set_fs(old_fs);

	// write to slave chip from pci
	*(volatile unsigned int *)boot_pci_dma_zone_virt_base=0x00000001;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);

	for(i=0; i<10 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++)
	{
		udelay(1);
	};

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)
	{
		printk(KERN_ERR "pci_config_dma_read timeout!\n");
	}

	for(i=0; i<100; i++)
	{
		udelay(100);
	};
	
	memcpy(boot_pci_dma_zone_virt_base,pFileBuff, UBOOT_FILE_MAXLEN);

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pfmemaddr);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	do_gettimeofday(&tv1);

	printk("Load image %s ", UBOOT_FILE_NAME);
	hi3511_bridge_ahb_writel(WDMA_CONTROL, ((UBOOT_FILE_MAXLEN << 8) | 0x71));

	for(i=0; i<10000 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++)
	{
		udelay(2000);
	};

	do_gettimeofday(&tv2);

	transfer_time=(1000000 * tv2.tv_sec + tv2.tv_usec)-(1000000 * tv1.tv_sec + tv1.tv_usec);

	printk( "use time %d uS\n", (int)transfer_time);

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)
		printk(KERN_ERR "pci_config_dma_read timeout!\n");

	for(i=0; i<100 ; i++)
	{
		udelay(100);
	};

	filp_close(fp_boot,0);

	vfree(pFileBuff);


    /*---------------- Load Linux Kernel Image---------------------------*/
	pFileBuff1 = (char *)vmalloc(KERNEL_FILE_MAXLEN);
	if(pFileBuff1 == NULL){

		return -ENOMEM;
	}
	memset(pFileBuff1,0,KERNEL_FILE_MAXLEN);

	old_fs = get_fs();	

	set_fs(KERNEL_DS);
	
	sprintf(fn,"%s%s",filedir, KERNEL_FILE_NAME);
	printk("%s will be load\n",fn);

	fp_kernel = filp_open(fn, O_RDONLY, 0);

	if(IS_ERR(fp_kernel) ){

		set_fs(old_fs);

		vfree(pFileBuff1);

		printk("<ERROR>filp_open error,please check the image file is correct!!!!\n");

		return IS_ERR(fp_kernel);
	}

	count = fp_kernel->f_op->read(fp_kernel,pFileBuff1,KERNEL_FILE_MAXLEN,&pos);
	pos = 0;
	set_fs(old_fs);

	// write to slave chip from pci
	*(volatile unsigned int *)boot_pci_dma_zone_virt_base=0x01000001;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);

	for(i=0; i<10 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {

		udelay(1);

	};

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001){

		printk(KERN_ERR "pci_config_dma_read timeout!\n");

	}

	for(i=0; i<100; i++) {

		udelay(100);

	};
	
	memcpy(boot_pci_dma_zone_virt_base,pFileBuff1,KERNEL_FILE_MAXLEN);

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pfmemaddr);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	do_gettimeofday(&tv1);

	printk("Load image %s ", KERNEL_FILE_NAME);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, ((KERNEL_FILE_MAXLEN << 8) | 0x71));

	for(i=0; i<10000 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {

		udelay(2000);

	};

	do_gettimeofday(&tv2);

	transfer_time=(1000000 * tv2.tv_sec + tv2.tv_usec)-(1000000 * tv1.tv_sec + tv1.tv_usec);

	printk( "use time %d uS\n", (int)transfer_time);

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)

		printk(KERN_ERR "pci_config_dma_read timeout!\n");

	for(i=0; i<100 ; i++) {

		udelay(100);

	};

	filp_close(fp_kernel,0);

	vfree(pFileBuff1);

	
    /*---------------- Load File System  Image---------------------------*/
	pFileBuff2 = (char *)vmalloc(FSIMG_FILE_MAXLEN);

	if(pFileBuff2==NULL){

		return -ENOMEM;
	}
	memset(pFileBuff2,0,FSIMG_FILE_MAXLEN);

	old_fs = get_fs();	

	set_fs(KERNEL_DS);

	sprintf(fn,"%s%s",filedir,FSIMG_FILE_NAME);
	printk("%s will be load\n",fn);

	fp_initrd = filp_open(fn, O_RDONLY, 0);

	if(IS_ERR(fp_initrd) ){

		set_fs(old_fs);

		vfree(pFileBuff2);

		printk("<ERROR>filp_open error,please check the image file is correct!!!!\n");

		return IS_ERR(fp_initrd);
	}

	count = fp_initrd->f_op->read(fp_initrd,pFileBuff2,FSIMG_FILE_MAXLEN,&pos);
	pos = 0;
	set_fs(old_fs);

	//load image
	*(volatile unsigned int *)boot_pci_dma_zone_virt_base=0x01500001;

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, npmemaddr+0x74);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	hi3511_bridge_ahb_writel(WDMA_CONTROL, 0x00000471);

	for(i=0; i<10 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {

		udelay(10);

	};

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001){

		printk(KERN_ERR "pci_config_dma_read timeout!\n");

	}

	for(i=0; i<100; i++) {

		udelay(100);

	};

	memcpy(boot_pci_dma_zone_virt_base,pFileBuff2,FSIMG_FILE_MAXLEN);

	hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, pfmemaddr);

	hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, boot_pci_dma_zone_base);

	do_gettimeofday(&tv1);

	printk("Load image %s ", FSIMG_FILE_NAME);
	hi3511_bridge_ahb_writel(WDMA_CONTROL, ((FSIMG_FILE_MAXLEN << 8) | 0x71));

	for(i=0; i<10000 && (hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001); i++) {

		udelay(2000);

	};

	do_gettimeofday(&tv2);

	transfer_time=(1000000 * tv2.tv_sec + tv2.tv_usec)-(1000000 * tv1.tv_sec + tv1.tv_usec);

	printk( "use time %d uS\n", (int)transfer_time);

	if(hi3511_bridge_ahb_readl(WDMA_CONTROL)&0x00000001)

		printk(KERN_ERR "pci_config_dma_read timeout!\n");

	for(i=0; i<100 ; i++) {

		udelay(100);

	};

	filp_close(fp_initrd,0);

	vfree(pFileBuff2);

#if 1
	addr = (char *)0xb0000000;

	regval = 0x35110100;

	move_np_window_write(addr, slot, regval);
/////////////////////////////////////////////////////////4jinlei
	addr = (char *)0xb0000054;                       

	move_np_window_write(addr, slot, slot4jinlei);

	slot4jinlei ++;
/////////////////////////////////////////////////////////4jinlei
	//start the slave  arm     
	arm_reset |= 0x00101000;

	addr = (char *)ARM_RESET;

	regval = arm_reset;

	move_np_window_write(addr, slot, regval);
#endif

	printk(KERN_ERR "data transfered!\n");

	return 0;

}
EXPORT_SYMBOL(pci_slave_load);

static int __devinit hi3511_boot_probe(struct pci_dev *pci_dev,
		const struct pci_device_id *pci_id)
{

	static unsigned int slotIndex = 0;

	npmemaddr = pci_resource_start(pci_dev, 3);

	pfmemaddr = pci_resource_start(pci_dev, 4);

	printk("\nslot %d npmemaddr is %lx, ",slotIndex, npmemaddr);

	printk("pfmemaddr is %lx \n",pfmemaddr);

	pfmemaddr_virt = ioremap(pfmemaddr, 0xa00000);

	npmemaddr_virt = ioremap(npmemaddr, 0x1000);

	pci_write_config_word(pci_dev, 0x04, 0x1ff);

	if (pfmemaddr_virt == NULL) return -ENOMEM;

	if (pfmemaddr_virt == NULL) return -ENOMEM;

	if(slotIndex < slotuse)
	{
		pci_slave_load(path, slotIndex);
	}
	
	slotIndex++;
	return 0;
}

static void __devexit hi3511_boot_remove(struct pci_dev *pci_dev)
{
	return ;
}

#ifdef CONFIG_PM

static int hi3511_boot_suspend(struct pci_dev *pci_dev, pm_message_t state)
{

        return 0;
}

static int hi3511_boot_resume(struct pci_dev *pci_dev)
{

        return 0;
}
#endif /* CONFIG_PM */


static struct pci_driver hi3511_pci_boot = {
        .name           = HI3511_BOOT_MODULE_NAME,
        .id_table       = hi3511_pci_tbl,
        .probe          = hi3511_boot_probe,
        .remove         = hi3511_boot_remove,

#ifdef CONFIG_PM
        .suspend        = hi3511_boot_suspend,
        .resume         = hi3511_boot_resume,
#endif /* CONFIG_PM */
};




//ko
static int __init hi3511_boot_init_module(void)
{
	int retn;

	PCI_HAL_DEBUG(0, "ok, boot start %d", 1);

	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	/* when a module, this is printed whether or not devices are found in probe */
#ifdef MODULE

	printk(version);

#endif
	kcom_getby_uuid(UUID_MEDIA_MEMORY, &kcom);

	if(kcom == NULL) {

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

	memset(boot_pci_dma_zone_virt_base, 0x0, SZ_16M);

	retn = pci_module_init(&hi3511_pci_boot);

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

	pci_unregister_driver(&hi3511_pci_boot);
}

module_init(hi3511_boot_init_module);

module_exit(hi3511_boot_cleanup_module);

module_param(path,   charp, S_IRUGO);
MODULE_PARM_DESC(path, "The path of the u-boot, kernel and fs image file.");

module_param(slotuse, int,   S_IRUGO);
MODULE_PARM_DESC(slotuse, "The pci slot number would be used.");

module_param(ddrsize, int,   S_IRUGO);
MODULE_PARM_DESC(ddrsize, "The DDR size(MB) of slave board.");


MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);


