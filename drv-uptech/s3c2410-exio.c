/*
 * s3c2410-da.c
 *
 * da driver for UP-NETARM2410-S DA
 *
 * Author: Wang bin <wbinbuaa@163.com>
 * Date  : $Date: 2005/7/22 15:50:00 $ 
 * Description : UP-NETARM2410-S, with 74hc573
 * $Revision: 2.1.0 $
 *
 * Based on s3c2410-da-max504.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * History:
 *
 * 
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
//#include <linux/kdev_t.h> MAJOR MKDEV
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mm.h>
//#include <linux/poll.h>
//#include <linux/spinlock.h>
//#include <linux/irq.h>
#include <asm/uaccess.h>				/* copy_from_user */
//#include <asm/sizes.h>

#include <asm/arch/spi.h>
#include <asm/io.h>
#include <asm/hardware.h>


/* debug macros */
#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

/* debug macros */
//#undef STEP_DEBUG
#define STEP_DEBUG
#ifdef STEP_DEBUG
#define DPRINTK_STEP(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK_STEP(x...) (void)(0)
#endif

#define DEVICE_NAME	"s3c2410-exio"
#define EXIORAW_MINOR	1

#define SZ_1K                           0x00000400
#define EXIODEV_PHY_START		0x08000100	// NCS1: DA physical base address
#define EXIODEV_PHY_SIZE		SZ_1K		// NCS1: DA physical base size 4k

#define DA0_IOCTRL_WRITE 			0x10
#define DA1_IOCTRL_WRITE			0x11
#define DA_IOCTRL_CLR 				0x12
#define STEPMOTOR_IOCTRL_PHASE 		0x13

static unsigned int s3c2410_exio_base;
static int exioMajor = 0;


/*************************************************************************/
/*From bitops.h*/
#if 0
static void bitops_set_bit(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] |= (1U << (nr & 7));
}


static void bitops_clear_bit(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] &= ~(1U << (nr & 7));
}
#endif

static void bitops_set_bit(int nr, volatile void *addr)
{
	*((unsigned int *) addr) |= (1U << (nr));
}

static void bitops_clear_bit(int nr, volatile void *addr)
{
	*((unsigned int *) addr) &= ~(1U << (nr));
}

static void bitops_mask_bit(int nr, int mask, volatile void *addr)
{
	*((unsigned int *) addr) &= ~mask;
	*((unsigned int *) addr) |= nr;
}

static void setSPImode (void)
{
	/*  prescaler =33   0-255*/
	Set_SIO_mode(0, SPCON_SMOD_POLL | SPCON_ENSCK | SPCON_MSTR |SPCON_CPOL_HIGH | 
		SPCON_CPHA_FMTA,   40, 2, NULL, NULL, NULL);
}

/************************************************************************/

static int s3c2410_exio_open(struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	DPRINTK( "S3c2410 EXIO device open!\n");
	return 0;
}

static int s3c2410_exio_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	DPRINTK( "S3c2410 EXIO device release!\n");
	return 0;
}


/*******Enable the select port of DA ********/
static ssize_t da_enable(size_t ndev )
{
	unsigned int bak;
	
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);

	if(0 == ndev){	
		bitops_clear_bit(1, &bak);		
	}else if(1 == ndev){
		bitops_clear_bit(3, &bak);		
	}	
	
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);		
	writew(bak, s3c2410_exio_base);	//enable jtag output
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);

	return 0;	
}


/*******Disable the select port of DA ********/
static ssize_t da_disable(size_t ndev )
{
	unsigned int bak;
	
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);

	if(0 == ndev){	
		bitops_set_bit(1, &bak);		
	}else if(1 == ndev){
		bitops_set_bit(3, &bak);		
	}	
	
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);		
	writew(bak, s3c2410_exio_base);	//enable jtag output
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);

	return 0;	
}


/*******Write the select port of DA ********/
static ssize_t da_write(size_t ndev, const char *buffer)
{
	unsigned int value;
	char buf[4];

	copy_from_user(buf, buffer, 4);
	value = *((int *)buf);	
	value <<= 2;

	da_enable(ndev);
  	SPISend ((value>>8) &0xff, 0);
  	SPISend ((value&0xff), 0);
//	udelay(100);
	da_disable(ndev);

	DPRINTK("write to max504-1 => %u\n",value);
	return 0;
}


/*******Clear the select port of DA ********/
static ssize_t da_clear(int ndev)
{
	unsigned int bak;

	da_enable(ndev);
	
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);

	if(0 == ndev){
		bitops_clear_bit(0, &bak);		
		bitops_set_bit(0, &bak);		
		DPRINTK("bitops_clear_bit(0, &bak)\n");	
		
	}else if(1 == ndev){	
	
		bitops_clear_bit(2, &bak);		
		bitops_set_bit(2, &bak);		
		DPRINTK("bitops_clear_bit(2, &bak)\n");		
	}	
	
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);		
	writew(bak, s3c2410_exio_base);	//enable jtag output
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);

	da_disable(ndev);
	return 0;	
}


void tiny_delay(int t)
{
	int i;
	for(;t>0;t--)
		for(i=0;i<400;i++);
}
static int do_stepmotor_run(char phase)
{
	unsigned int bak;
	
	bak = readw(s3c2410_exio_base);
	DPRINTK_STEP("s3c2410_exio_base content is %x\n", bak);
//	tiny_delay(5);

	bitops_mask_bit(phase, 0xf0, &bak);
	DPRINTK_STEP("s3c2410_exio_base content is %x\n", bak);
//	tiny_delay(5);

	writew(bak, s3c2410_exio_base);	//enable jtag output
	bak = readw(s3c2410_exio_base);
	DPRINTK_STEP("s3c2410_exio_base content is %x\n", bak);
//	tiny_delay(5);
	DPRINTK_STEP("\n");
	return 0;
}

static int s3c2410_exio_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
		
	/*********write da 0 with (*arg) ************/		
	case DA0_IOCTRL_WRITE:
		return da_write(0, (const char *)arg);

	/*********write da 1 with (*arg) ************/		
	case DA1_IOCTRL_WRITE:
		return da_write(1, (const char *)arg);

	/*********clear da (*arg) *****************/		
	case DA_IOCTRL_CLR:
		return da_clear((int)arg);
		
	/*********step motor run ((char)arg) *****************/		
	case STEPMOTOR_IOCTRL_PHASE:
		return do_stepmotor_run((char)arg);
	}

	return 0;	
}

static struct file_operations s3c2410_exio_fops = {
	owner:	THIS_MODULE,
	open:	s3c2410_exio_open,
//	mmap:	Jtag_mmap,
//	read:	Jtag_read,	
//	write:	Jtag_write,
	ioctl:	s3c2410_exio_ioctl,
	release:	s3c2410_exio_release,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_exio_dir, devfs_exio0;
#endif
int __init s3c2410_exio_init(void)
{
	int ret;
s3c2410_exio_base = IDE_BASE + 0x100;

/*	s3c2410_exio_base = (unsigned int) ioremap(EXIODEV_PHY_START, SZ_1K); 
	if(!s3c2410_exio_base) {
		DPRINTK("ioremap S3C2410 EXIO failed\n");
		return -EINVAL;
	}
*/
	
	/*
	unsigned int bak;
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);
	
	bitops_set_bit(1, &bak);
	bitops_set_bit(3, &bak);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);
	writew(bak, s3c2410_exio_base);	//enable jtag output
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);
	
	bitops_clear_bit(1, &bak);
	bitops_clear_bit(3, &bak);	
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);
	
	writew(bak, s3c2410_exio_base);	//enable jtag output
	bak = readw(s3c2410_exio_base);
	DPRINTK("s3c2410_exio_base content is %2.2x\n", bak);
	*/

	SPI_initIO(0);
	setSPImode();
	
	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_exio_fops);
	if (ret < 0) {
		DPRINTK(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	exioMajor=ret;

#ifdef CONFIG_DEVFS_FS
	devfs_exio_dir = devfs_mk_dir(NULL, "exio", NULL);
	devfs_exio0 = devfs_register(devfs_exio_dir, "0raw", DEVFS_FL_DEFAULT,
				exioMajor, EXIORAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_exio_fops, NULL);

#endif

	DPRINTK (DEVICE_NAME"\tdevice initialized\n");
	return 0;
	
}

module_init(s3c2410_exio_init);

#ifdef MODULE
void __exit s3c2410_exio_exit(void)
{
#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_exio0);
	devfs_unregister(devfs_exio_dir);
#endif
	unregister_chrdev(exioMajor, DEVICE_NAME);
}

module_exit(s3c2410_exio_exit);
#endif


MODULE_LICENSE("GPL"); 

