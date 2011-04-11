/*
 * s3c2410-fpga.c
 *
 *  S3C2410 FPGA
 *  derive  from at91rm9200-jtag by threewater
 *
 * Author: wb <wbinbuaa@163.com>
 * Date  : $Date: 2005/06/13 $ 
 *
 * $Revision: 1.0.0.1 $
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mm.h>

#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/sizes.h>

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif


#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif


#define FPGA_PHY_START		0x20000000	// NCS4: Slot FPGA physical base address
#define FPGA_PHY_SIZE			SZ_4K		// NCS4: Slot FPGA physical base size 4k

#define DEVICE_NAME	"fpga"
#define FPGARAW_MINOR	1

static int fpgaMajor = 0;
unsigned long s3c2410_fpga_base;

static int fpga_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT; 
	unsigned long physical = FPGA_PHY_START + off; 
	unsigned long vsize = vma->vm_end - vma->vm_start; 
	unsigned long psize = FPGA_PHY_SIZE- off; 

	DPRINTK("mmap offset=0x%x, protect=0x%x. \n", off, vma->vm_page_prot);

	if (vsize > psize) 
	   return -EINVAL; //  spans too high 
	
	vma->vm_flags |= VM_IO|VM_RESERVED;
	vma->vm_page_prot=pgprot_noncached(vma->vm_page_prot);		

 	remap_page_range(vma->vm_start, physical, vsize, vma->vm_page_prot); 
 	//remap_page_range(vma->vm_start, physical, vsize, PAGE_SHARED); 

	return 0;
}


static int fpga_open(struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	DPRINTK( "fpga device open!\n");
	return 0;
}

static int fpga_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	DPRINTK( "fpga device release!\n");
	return 0;
}

//for fpga iocontrol

static int fpga_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
	default:
		DPRINTK("Do not have ioctl methods\n");
	}

	return 0;	
}

static struct file_operations s3c2410_fops = {
	owner:	THIS_MODULE,
	open:	fpga_open,
	mmap:	fpga_mmap,
//	read:	fpga_read,	
//	write:	fpga_write,
	ioctl:	fpga_ioctl,
	release:	fpga_release,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_fpga_dir, devfs_fpgaraw;
#endif


int __init fpga_init(void)
{
	int ret;

/*	ret = request_irq(IRQ_ADC_DONE, adcdone_int_handler, SA_INTERRUPT, DEVICE_NAME, NULL);
	if (ret) {
		return ret;
	}*/
	s3c2410_fpga_base= (unsigned long) ioremap(FPGA_PHY_START, SZ_4K); 
	if(!s3c2410_fpga_base) {
		printk("ioremap S3C2410 fpga failed\n");
		return -EINVAL;
	}

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	fpgaMajor=ret;

#ifdef CONFIG_DEVFS_FS
	devfs_fpga_dir = devfs_mk_dir(NULL, "fpga", NULL);
	devfs_fpgaraw = devfs_register(devfs_fpga_dir, "0raw", DEVFS_FL_DEFAULT,
				fpgaMajor, FPGARAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_fops, NULL);
#endif
	printk (DEVICE_NAME"\tdevice initialized\n");

	return 0;
}

module_init(fpga_init);

#ifdef MODULE
void __exit fpga_exit(void)
{

#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_fpgaraw);
	devfs_unregister(devfs_fpga_dir);
#endif
	unregister_chrdev(fpgaMajor, DEVICE_NAME);
}

module_exit(fpga_exit);
#endif
MODULE_LICENSE("GPL");
