#include <linux/config.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware/amba.h>
#include <asm/hardware/clock.h>
#include <pci_vdd.h>
#include <vdd-utils.h>

#if 0
extern int thread_recv_in;
static struct timeval s_old_time;
static int PCI_PrintTimeDiff(const char *file, const char *fun, const int line)
{
       struct timeval cur;
       long diff = 0;  /* unit: millisecond */  
       do_gettimeofday(&cur);
       diff = (cur.tv_sec - s_old_time.tv_sec) * 1000 + (cur.tv_usec - s_old_time.tv_usec)/1000;
       printk("\nfile:%s, function:%s, line:%d, millisecond:%ld\n", file, fun, line, diff); 
       s_old_time = cur;
}

#define PCI_DBG_TM_INIT()  do_gettimeofday(&s_old_time)
#define PCI_DBG_TM_DIFF()  PCI_PrintTimeDiff(__FILE__, __FUNCTION__, __LINE__)

#endif


#define VCOM_DBUG_LEVEL	3
#define VCOM_DEBUG(level, s, params...) do{ if(level >= VCOM_DBUG_LEVEL)\
	                	printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
	                }while(0)

#define VCOM_TTY_MAJOR	IBM_TTY3270_MAJOR
#define VCOM_TTY_MINOR  0	
#define VCOM_TTY_NR	128

#define VCOM_PEAR_PORTS 2

#define VCOM_TARGETID(line) (vcom_is_host()?((line + VCOM_PEAR_PORTS)/VCOM_PEAR_PORTS):0)
#define VCOM_PORT(line)	( ((line)%VCOM_PEAR_PORTS) + 1)
#define VCOM_MSG_ID 1

//#define __VCOM_TEST__
struct pci_virtual_serial {
	void *vdd;
	struct tty_struct * tty;
//	struct file * filp;
	unsigned int open_cout;
	struct semaphore	rsem;		/* locks this structure */
	struct semaphore	wsem;		/* locks this structure */
	unsigned char buf[];
};
#define VCOM_MAX_WRITE_ROOM	(PAGE_SIZE-sizeof(struct pci_virtual_serial))

static struct pci_virtual_serial *serial_table[VCOM_TTY_NR];	/* initially all NULL */

static int vcom_receive_from(void *handle, void *data)
{
	int rvlen, i;
	pci_vdd_opt_t opt;
	struct pci_virtual_serial *pvs;
	struct tty_struct * tty;

	opt.flags = 0;
	VCOM_DEBUG(1, "vcom_receive in%d",0);

	pci_vdd_getopt(handle, &opt);
	
	pvs = (struct pci_virtual_serial *)opt.data;

	if(pvs == NULL){
		VCOM_DEBUG(3, "vcom_receive return error %d",0);
		return -ENOMEM;
	}
	
	// for(i=0;i<10;i++)  udelay(1000);
	down(&pvs->rsem);
	

	VCOM_DEBUG(1, "vcom_receive %d",0);

	if(!pvs->open_cout){
		goto vcom_receive_from_out;
	}
	VCOM_DEBUG(1, "opt.flags :%lx",opt.flags);
	//rvlen = pci_vdd_recvfrom(handle, pvs->buf, VCOM_MAX_WRITE_ROOM, 0);
//PCI_DBG_TM_INIT();

	rvlen = pci_vdd_recvfrom(handle, pvs->buf, VCOM_MAX_WRITE_ROOM, 0);//61104
	
	if(rvlen <= 0){
		VCOM_DEBUG(1, "rvlen <= 0:%d",rvlen);
		goto vcom_receive_from_out;
	}
	else{
		VCOM_DEBUG(1, "rvlen > 0:%d",rvlen);
	}
	
	tty = pvs->tty;

	if(((opt.flags)&0x2)==0x2)//61104
	{
		for(i=0; i<rvlen; i++) {
			
			VCOM_DEBUG(1, "flags ==2 %c %d",pvs->buf[i],pvs->buf[i]);

			if(tty->flip.count >= TTY_FLIPBUF_SIZE)
			{     
				//if (tty->low_latency){
				tty->low_latency = 1;
				tty_flip_buffer_push(tty);
				VCOM_DEBUG(2, "pushing flip buffer %d",tty->low_latency);	
				//}
			}
			//udelay(1000);
			else {
				tty->low_latency=0;
			}
			tty_insert_flip_char(tty, pvs->buf[i], TTY_NORMAL);
		}
	}
	else	{
		
		for(i=0; i<rvlen; i++) {
			VCOM_DEBUG(1, "flags ==3 %c %d",pvs->buf[i],pvs->buf[i]);
			if(tty->flip.count >= TTY_FLIPBUF_SIZE)
			{	  
				tty_flip_buffer_push(tty);
				VCOM_DEBUG(2, "pushing flip buffer %d",tty->low_latency);	
						
			}
			tty->low_latency=0;
			tty_insert_flip_char(tty, pvs->buf[i], TTY_NORMAL);
		}

	}	

//udelay(1000);
	VCOM_DEBUG(2, "before end push %d",0);
//	tty->low_latency = 0;

	tty_flip_buffer_push(tty);

vcom_receive_from_out:
	VCOM_DEBUG(1, "%d",rvlen);
	up(&pvs->rsem);
	
	VCOM_DEBUG(1, "vcom recv out %d",rvlen);
	return 0;
}

#if 0
static void vcom_put_char(struct tty_struct *tty, unsigned char ch)
{
	int retval;
		
	/* get my pci_virtual_serial struct first */
	struct pci_virtual_serial *pvs = (struct pci_virtual_serial *)tty->driver_data;
	if(pvs == NULL){
		return -ENODEV;
	}
		
	VCOM_DEBUG(1, "vcom_put_char %d", 1);
	down(&pvs->wsem);
	if(!pvs->open_cout){
		/*tty have not been opened */
		goto vcom_write_exit;
	}
	//printk("this is vcom write!!!!!!!!!!!!!!!!\n");
	retval = pci_vdd_sendto(pvs->vdd, (unsigned char *)&ch, 1, 0x4);
	
	vcom_write_exit:
	
	up(&pvs->wsem);
	return retval;

	return;
}

static void vcom_flush_buffer(struct tty_struct *tty)
{
	struct pci_virtual_serial *pvs = (struct pci_virtual_serial *)tty->driver_data;
	if(pvs == NULL){
		return -ENODEV;
	}
	VCOM_DEBUG(5, "vcom_flush_buffer %d", 1);
	return;

}
#endif

static void vcom_hangup(struct tty_struct *tty)
{
	VCOM_DEBUG(1, "vcom_hungup. %d",0);
	return;
}
static int vcom_is_host(void)
{
	return vdd_is_host();
}

static int vcom_get_target_num(void)
{
	return vdd_get_target_num();
}

static int  vcom_open(struct tty_struct * tty, struct file * filp)
{
	struct pci_virtual_serial *pvs = NULL;
	int index;
	/* initialize the pointer in case something fails */
	tty->driver_data = NULL;

	/* get the serial object associated with this tty pointer */
	index = tty->index;
	pvs = serial_table[index];
	
	if(pvs == NULL) {
		pci_vdd_opt_t opt;

		VCOM_DEBUG(2, "tty->index: %d, open new handle.", tty->index);
		pvs = vmalloc(PAGE_ALIGN(sizeof(struct pci_virtual_serial)));
		
		if(pvs == NULL)
			return -ENOMEM;

		init_MUTEX(&pvs->wsem);
		init_MUTEX(&pvs->rsem);
		
		pvs->open_cout = 0;		
		pvs->vdd = pci_vdd_open(VCOM_TARGETID(tty->index), VCOM_MSG_ID, VCOM_PORT(tty->index));
		if(pvs->vdd == NULL) {
			vfree(pvs);
			return -EBUSY;
		}
		opt.flags=0;
		pci_vdd_getopt(pvs->vdd, &opt);
		opt.flags|=0x1;//c58721 for asyn recv
		opt.data = (unsigned long)pvs;
		opt.recvfrom_notify = vcom_receive_from;
		pci_vdd_setopt(pvs->vdd, &opt);

		serial_table[index] = pvs;
		
	} else {
		VCOM_DEBUG(1, "tty->index: %d, use old handle.", tty->index);
	}
	
	down(&pvs->rsem);
	down(&pvs->wsem);

	pvs->open_cout++;

	pvs->tty = tty;

	tty->driver_data = (void *)pvs;

	tty->low_latency = 1;//c61104 20071031
	
	up(&pvs->wsem);
	up(&pvs->rsem);
	
	return 0;
}

static void vcom_close(struct tty_struct * tty, struct file * filp)
{
#ifndef __VCOM_TEST__
	int index;
	struct pci_virtual_serial *pvs = (struct pci_virtual_serial *)tty->driver_data;

	if(pvs == NULL){
		goto vcom_close_exit;
	}

	down(&pvs->wsem);
	down(&pvs->rsem);

	VCOM_DEBUG(1, "vcom_close %d", 0);
	if(!pvs->open_cout){
		up(&pvs->rsem);
		up(&pvs->wsem);
		/* tty have not been opened*/
		goto vcom_close_exit;
	}
	
	pvs->open_cout --;

	if(pvs->open_cout <= 0){
		VCOM_DEBUG(1, "tty->index: %d, close handle.", tty->index);
		pci_vdd_close(pvs->vdd);
		up(&pvs->wsem);
		up(&pvs->rsem);
		vfree(pvs);
		tty->driver_data = NULL;
		index = tty->index;
		serial_table[index] = NULL; 
	}
	else{
		VCOM_DEBUG(1, "tty->index: %d, do not handle close.", tty->index);
		up(&pvs->rsem);
		up(&pvs->wsem);

	}
	
#else
	down(&pvs->wsem);
	down(&pvs->rsem);

	VCOM_DEBUG(1, "vcom_close %d", 0);
	up(&pvs->rsem);
	up(&pvs->wsem);

#endif
vcom_close_exit:
	return;
	
}

static int vcom_write(struct tty_struct * tty, const unsigned char *buf, int count)
{
	int retval=0,i;
	
	/* get my pci_virtual_serial struct first */
	struct pci_virtual_serial *pvs = (struct pci_virtual_serial *)tty->driver_data;
	if(pvs == NULL){
		return -ENODEV;
	}

	VCOM_DEBUG(1, "vcom_write %d", count);
	
	down(&pvs->wsem);

	if(!pvs->open_cout){
		/*tty have not been opened */
		goto vcom_write_exit;
	}
	//printk("this is vcom write!!!!!!!!!!!!!!!!\n");
	//PCI_DBG_TM_INIT();
	for(i = 0; i < count ; i++)
		VCOM_DEBUG(1, "write -- %d",buf[i]);	

	retval = pci_vdd_sendto(pvs->vdd, (unsigned char *)buf, count, 0x6);

	//PCI_DBG_TM_DIFF();
	if(count > 20)
		VCOM_DEBUG(0, "vcom_write %d", count);
	
vcom_write_exit:
	VCOM_DEBUG(1, "vcom_write out %d", retval);
	up(&pvs->wsem);

	return retval;
}

static int vcom_write_room(struct tty_struct *tty)
{
	int retval = -EINVAL;
	struct pci_virtual_serial *pvs = (struct pci_virtual_serial *)tty->driver_data;
	if(pvs == NULL){
		return -ENODEV;
	}
	
	down(&pvs->wsem);
	down(&pvs->rsem);


	if(!pvs->open_cout){
		goto vcom_write_room_out;
	}
	
	retval = VCOM_MAX_WRITE_ROOM;

	VCOM_DEBUG(1, "vcom_write_room %d", retval);

vcom_write_room_out:
	up(&pvs->wsem);
	up(&pvs->rsem);	
	
	return retval;
}

static struct tty_operations vcom_ops = {
	.open	= vcom_open,
	.close	= vcom_close,
	.write	= vcom_write,
	.write_room	= vcom_write_room,
	//.put_char = vcom_put_char,
	//.flush_buffer = vcom_flush_buffer,
	.hangup = vcom_hangup,
};

static struct tty_driver *vcom_tty_drv = NULL;

static int vcom_get_max_minor(void)
{
	if(vcom_is_host()) {
		return VCOM_TTY_MINOR + vcom_get_target_num()*VCOM_PEAR_PORTS;
	} else {
		return VCOM_TTY_MINOR + VCOM_PEAR_PORTS;
	}
}

static void vcom_remove_ttys(struct tty_driver *tty_drv)
{
	int i;

	for(i=VCOM_TTY_MINOR; i<vcom_get_max_minor(); i++) {
		VCOM_DEBUG(1, "%d", i);
		tty_unregister_device(tty_drv, i);
	}
}

static int __init vcom_probe(struct tty_driver *tty_drv)
{
	int ret = 0;
	int i;

	for(i=VCOM_TTY_MINOR; i< vcom_get_max_minor(); i++) {
		VCOM_DEBUG(1, "%d", i);
		tty_register_device(tty_drv, i, NULL);
	}

	return ret;
}

static void cfmakeraw (struct termios *t)
{
  t->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
  t->c_oflag &= ~OPOST;
  t->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
  t->c_cflag &= ~(CSIZE|PARENB);
  t->c_cflag |= CS8;
  t->c_cc[VMIN] = 1;		/* read returns when one char is available.  */
  t->c_cc[VTIME] = 0;
}

static int __init vcom_init(void)
{
	int ret = 0;
	VCOM_DEBUG(0, "tty_register_driver ret %d", ret);
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	/* alloc tty driver struct */
	vcom_tty_drv = alloc_tty_driver(VCOM_TTY_NR);

	/* stuff */
	vcom_tty_drv->owner		= THIS_MODULE;
	vcom_tty_drv->driver_name	= "pci_serial";
	vcom_tty_drv->name		= "vcom";
	vcom_tty_drv->devfs_name	= "vcom/ttyS";
	vcom_tty_drv->major		= VCOM_TTY_MAJOR;
	vcom_tty_drv->type		= TTY_DRIVER_TYPE_SERIAL;
	vcom_tty_drv->subtype		= SERIAL_TYPE_NORMAL;
	vcom_tty_drv->flags		= TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS;
	vcom_tty_drv->init_termios	= tty_std_termios;
	vcom_tty_drv->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL |PARENB | PARODD;
	vcom_tty_drv->init_termios.c_cflag &= ~CSTOPB;
	vcom_tty_drv->init_termios.c_oflag  &= ~OPOST;
	cfmakeraw(&vcom_tty_drv->init_termios);
	tty_set_operations(vcom_tty_drv, &vcom_ops);

	ret = tty_register_driver(vcom_tty_drv);
	VCOM_DEBUG(0, "tty_register_driver ret %d", ret);
	if(ret == 0) {
		ret = vcom_probe(vcom_tty_drv);
		VCOM_DEBUG(0, "vcom_probe ret %d", ret);
		if(ret) {
			vcom_remove_ttys(vcom_tty_drv);
		}
	}
	
	if(ret) {
		tty_unregister_driver(vcom_tty_drv);
		put_tty_driver(vcom_tty_drv);
	}

	return ret;
}

static void __exit vcom_exit(void)
{
	vcom_remove_ttys(vcom_tty_drv);
	tty_unregister_driver(vcom_tty_drv);
	put_tty_driver(vcom_tty_drv);
}

module_init(vcom_init);
module_exit(vcom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liujiandong");
MODULE_DESCRIPTION("Hisilicon virtual serial port over pci");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
