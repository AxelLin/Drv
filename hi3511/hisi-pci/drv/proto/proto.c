#include <linux/moduleparam.h>
#include <asm/hardware.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <linux/vmalloc.h>
//#include <asm-arm/arch-hi3510_v100_p01/media-mem.h>
#include "../include/hi3511_pci.h"
#include "datacomm.h"
#include "proto.h"
#include "../../include/pci_vdd.h"

#include <linux/kcom.h>
#include <kcom/mmz.h>
struct kcom_object *kcom;
struct kcom_media_memory *kcom_media_memory;


#ifdef CONFIG_PCI_HOST
// host mutex
static DECLARE_MUTEX( hipci_hostsemlock );
void *host_recv_buffer;
unsigned long host_sharebuffer_start;
extern unsigned long hi3511_slave_interrupt_flag;
extern void *hi3511_slave_interrupt_flag_virt;	
atomic_t   host2slave_irq[HI3511_PCI_SLAVE_NUM];	
#else
//slave mutex
static DECLARE_MUTEX( hipci_slavesemlock );
void *datarelay;
unsigned long settings_addr_physics;
void *slave_recv_buffer;
unsigned long slave_sharebuffer_start;
unsigned long hi3511_slave_interrupt_flag; 
atomic_t   slave2host_irq;
#endif

// host and slave share mutex
static DECLARE_MUTEX( hipci_sharedlock );

//extern struct hi3511_dev dev_head;

struct hi3511_pci_data_transfer_handle_table *hi3511_pci_handle_table;

struct pci_vdd_info hi3511_pci_vdd_info;


extern struct hi3511_dev hi3511_device_head;

struct   hi3511_dev  *hi3511_dev_head = &hi3511_device_head;

#define HI_PCI_DEVICE_NUMBER 5




struct timeval s_old_time;
#if 0
static int PCI_PrintTimeDiff(const char *file, const char *fun, const int line)
{
       struct timeval cur;
       long diff = 0;  /* unit: millisecond */  
       do_gettimeofday(&cur);
       diff = (cur.tv_sec - s_old_time.tv_sec) * 1000 + (cur.tv_usec - s_old_time.tv_usec)/1000;
       printk("\nfile:%s, function:%s, line:%d, millisecond:%ld\n", file, fun, line, diff); 
       s_old_time = cur;
}
#endif

#define PCI_DBG_TM_INIT()  do_gettimeofday(&s_old_time)
//#define PCI_DBG_TM_DIFF()  PCI_PrintTimeDiff(__FILE__, __FUNCTION__, __LINE__)



struct mem_addr_couple hi3511_mem_map[HI_PCI_DEVICE_NUMBER];


atomic_t   lostpacknum;
atomic_t   irq_num;
atomic_t   readpack_num;
struct task_struct *precvthread;

spinlock_t hi3511_pci_irq_spinlock;	/* Spinlock */

atomic_t threadexitflag;

void *exchange_buffer_addr;
unsigned long exchange_buffer_addr_physics;

void *fifo_stuff;
unsigned long fifo_stuff_addr_physics;
//void *head_image;

wait_queue_head_t send_tranfic_control_queue;
volatile unsigned long tranfic_control_flag = 0;

#ifdef CONFIG_PCI_HOST

extern struct Hi3511_pci_slave2host_mmap_info Hi3511_slave2host_mmap_info;

#else

extern struct Hi3511_pci_slave_mmap_info Hi3511_slave_mmap_info;
DECLARE_MUTEX(pci_dma_sem);
EXPORT_SYMBOL(pci_dma_sem);
#endif

extern unsigned long prewake;


#ifdef CONFIG_PCI_HOST
/*
*	this function used after share memory mapped
*/
static irqreturn_t pci_host_irq_step2(void)
{
	struct hi3511_dev *pNode = NULL;
	struct list_head *node;	
	int slot = 0;
	int number = 0;
	int i = 0;
	// check the status register of this share interrupt 
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"interrupt handler step2 %d",0);
	slot =hi3511_interrupt_check();

	//slot = 2;
	number = (slot & 0xff000000)>>24;

	slot = slot & 0xff;
	//when use two interrupt line
	if(slot == HI_NONE){
		return IRQ_NONE;
	}

	if( (slot != HI_SLOT1) && (slot != HI_SLOT2) && (slot != HI_SLOT3) && (slot != HI_SLOT4)&&(slot != HI_DMA_END)
		&& (slot != HI_SLOT11) && (slot!= HI_SLOT12) && (slot != HI_SLOT13) && (slot != HI_SLOT14)){

		// handle wrong irq report
		//hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"*****interrupt error status 0x%lx****",READ_INT_HI_SLOT(2));
		hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"*****interrupt error value of slot 0x%x****",slot);
		
		list_for_each(node, &hi3511_dev_head->node){

			pNode = list_entry(node, struct hi3511_dev, node);

			//CLEAR_INT_HI_SLOT(pNode->slot);
			clear_int_hi_slot(pNode->slot);
			writel(0xdadadada,hi3511_slave_interrupt_flag_virt + 4*i);

			i++;
		}
		
		// add by chanjinn for IRQ_HANDLED
		return IRQ_HANDLED;
	}

	if( slot == HI_DMA_END){

		//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"interrupt handler for dma %d",0);
		// clear
		disable_int_hi_dma();
			
		// wake up dma wait queue
		wake_up(&hi3511_pci_handle_table->dmawaitqueue);

		clear_int_hi_dma();

		enable_int_hi_dma();

		return IRQ_HANDLED;
	}

	disable_int_hi_slot(slot);

	clear_int_hi_slot(slot);

	writel(0xdadadada,hi3511_slave_interrupt_flag_virt + number * 4);

	wake_up_process(precvthread);

	atomic_inc(&irq_num);
	
	if(tranfic_control_flag)	{
		wake_up(&send_tranfic_control_queue);
	}
	
	enable_int_hi_slot(slot);
	return IRQ_HANDLED;
}

/*
*	this function used before share memory mapped
*/
static irqreturn_t pci_host_irq_handle_step1(void)
{
	struct Hi3511_pci_data_transfer_head pri_head;
	struct Hi3511_pci_initial_host2pci init_package;
	struct Hi3511_pci_slave2host_mmap_info *pmmap_info = &Hi3511_slave2host_mmap_info;
	struct hi3511_dev *pNode;
	struct list_head *node;	
	int headsize = sizeof(struct Hi3511_pci_data_transfer_head);
	int slot = HI_NONE;
	int i = 0;

	// check the status register of this share interrupt 
	//slot = hi3511_interrupt_check();  // for test not use yet
	list_for_each(node, &hi3511_dev_head->node){
		
		pNode = list_entry(node, struct hi3511_dev, node);
			
		if(read_int_hi_slot(pNode->slot) == 0x00f00000)
			slot = pNode->slot;
	}

	//when use two interrupt line
	if(slot == HI_NONE){
		return IRQ_NONE;
	}

	if( (slot != HI_SLAVE_UNMAP) && (slot != HI_SLOT1) && (slot!= HI_SLOT2) && (slot != HI_SLOT3) && (slot != HI_SLOT4) && (slot != HI_DMA_END)
		&& (slot != HI_SLOT11) && (slot!= HI_SLOT12) && (slot != HI_SLOT13) && (slot != HI_SLOT14)){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"this is the value of slot when return IRQ_NONE %d",slot);

		return IRQ_NONE;
	}

	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"2slot number in pci_host_irq_handle_step1is %d",slot);
	if( slot == HI_DMA_END ){
		
		// wake up dma wait queue;
		wake_up(&hi3511_pci_handle_table->dmawaitqueue);

		clear_int_hi_dma();
		
		enable_int_hi_dma();

		return IRQ_HANDLED;
	}

	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"3slot number in pci_host_irq_handle_step1is %d",slot);

	disable_int_hi_slot(slot);

	clear_int_hi_slot(slot);

	for(i = 0 ; i < HI3511_PCI_SLAVE_NUM; i++){
		if(pmmap_info->mmapinfo[i].slot == slot)
			break;
	}

	/* set head */
	// send a high priority package to slave know the slot num, the mem size , the send addr  and the recv addr 
	/* host to slave initial package */
	pri_head.src = 0;
	pri_head.pri = 0x3;
	pri_head.length = sizeof(struct Hi3511_pci_initial_host2pci);
	pri_head.reserved = 0x2D5A;

	pri_head.target = i + 1;        // target_id = i+1				
	init_package.slaveid = i + 1;
	init_package.interrupt_flag_addr = hi3511_slave_interrupt_flag + 4*i;
	init_package.slot = slot;

	/* copy head to share memory*/
	memcpy((char *)(pmmap_info->mmapinfo[i].send.pripackageaddr  - headsize),&pri_head,headsize);
				
	memcpy((char *)pmmap_info->mmapinfo[i].send.pripackageaddr, &init_package, pri_head.length);

	// set pri memory been writen flag
	__raw_writew(PCI_UNWRITEABLE_PRIPACKAGE_FLAG, (char *)pmmap_info->mmapinfo[i].send.pripackageflagaddr);
	udelay(20);

	hi3511_pci_host2slave_irq(i);
	//master_to_slave_irq(pmmap_info->mmapinfo[i].slot);

	wake_up_process(precvthread);

	atomic_inc(&irq_num);

	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"end of pci_host_irq_handle_step1 %d",0);
	hi3511_pci_vdd_info.mapped |= 0x10 << slot;

	hi3511_pci_vdd_info.slave_map |= 1 << slot;

	// every slot initial mask mapped (0x10 << slot) ,4 slot full than mapped
	if((hi3511_pci_vdd_info.mapped & 0xf0) == 0xf0)
		hi3511_pci_vdd_info.mapped = 1;

	enable_int_hi_slot(slot);

	return IRQ_HANDLED;
}


/*
*	host irq function
*/
#ifndef  CONFIG_PCI_TEST
static 
#endif
irqreturn_t pci_host_irq_handle(int irq, void *dev_id, struct pt_regs *regs)
{
	irqreturn_t ret;
//	static DEFINE_SPINLOCK(irqlock);
	
//	spin_lock_irq(&irqlock);

	/* before memory mapped */
	if(hi3511_pci_vdd_info.mapped != 1){
			ret = pci_host_irq_handle_step1();
	}
	/* memory mapped */
	else{
		ret = pci_host_irq_step2();
	}
//	spin_unlock_irq(&irqlock);
	return ret;
}

/*
*	input: void
*	output:NULL
*	return: 0 success,others failed
*	Notes:
*	this initial function create handle table, create kernel receive thread, 
*	timer initial,register irq and so on
*	
*/
int pci_vdd_init(void)
{
	struct pci_vdd_info *pci_info = &hi3511_pci_vdd_info;

	int memsize = sizeof(struct hi3511_pci_data_transfer_handle_table);

	struct hi3511_dev *pNode = NULL;

	struct list_head *node;	
	int irq_number ;	
	unsigned long j;

	int result = 0;

	int slot = 0;
	int i = 0;
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	kcom_getby_uuid(UUID_MEDIA_MEMORY, &kcom);

        if(kcom ==NULL) {

                printk(KERN_ERR "MMZ: can't access mmz, start mmz service first.\n");

                return -1;
        }

	kcom_media_memory = kcom_to_instance(kcom, struct kcom_media_memory, kcom);
	
	host_sharebuffer_start = kcom_media_memory->new_mmb("host_share_buffer", host_sharebuffer_size, 0, NULL);

	if(host_sharebuffer_start == 0){

		PCI_HAL_DEBUG(0, "hil_mmb_alloc error %d", 1);
	}

	host_recv_buffer = kcom_media_memory->remap_mmb(host_sharebuffer_start);

	memset(host_recv_buffer ,0x0, host_sharebuffer_size);

	exchange_buffer_addr_physics = kcom_media_memory->new_mmb("exchange_buffer", exchange_buffer_size, 0,NULL);

	if(exchange_buffer_addr_physics == 0){

		PCI_HAL_DEBUG(0, "hil_mmb_alloc error %d", 1);
	}

	exchange_buffer_addr = kcom_media_memory->remap_mmb(exchange_buffer_addr_physics);

	memset(exchange_buffer_addr ,0x0, exchange_buffer_size);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"host pci_vdd_init %d",0);

	//create transfer handle table
 	// get the mem size
 	hi3511_pci_handle_table = kmalloc(memsize,GFP_KERNEL);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_handle_table kmalloc %d",0);
	
	if(hi3511_pci_handle_table == NULL) {
		printk("hi3511_pci_handle_table kmalloc error \n");
		result = -1;
		goto pci_vdd_init_out;
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"memset %d",0);
	
	memset(&hi3511_pci_vdd_info,0,sizeof(struct pci_vdd_info));
	
	memset(hi3511_pci_handle_table, 0 , memsize);

	//host id =0
	pci_info->version = 0;
	
	pci_info->id = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"init timer %d",0);

	/* initial timer */
	init_timer(&hi3511_pci_handle_table->timer);
	
	/* initial timer */
	init_timer(&hi3511_pci_handle_table->slavemap_timer);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"add timer %d",0);

	/* set send timer !*/
	j = jiffies;

	hi3511_pci_vdd_info.mapped = 0;
	
	hi3511_pci_handle_table->slavemap_timer.expires = j + HI3511_PCI_MAPPED_TIMER_DELAY;
		
	hi3511_pci_handle_table->slavemap_timer.function = hi3511_mapped_info; //set slave interrupt func

	add_timer(&hi3511_pci_handle_table->slavemap_timer);

	/*initial dma wait queue */
	init_waitqueue_head(&hi3511_pci_handle_table->dmawaitqueue);

	irq_number =  list_entry(hi3511_dev_head->node.next, struct hi3511_dev, node)->irq;

	/*
	@ param PCI_IRQ pci irq number
	@ param pci_irq_handle irq service
	*/
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"irq %d",irq_number);
	
	// kernel thread recv
	// should not create here! just wake up
	precvthread = kthread_create(hi3511_pci_kernelthread_deamon,NULL, "hi3511_pci_recv");

	if(IS_ERR(precvthread) <0) 
	{
		printk("create recv thread failed \n");
		result =  -1;
		goto pci_vdd_init_out;
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"kthread_createed %d",0);

	atomic_set(&threadexitflag,1);
	
	for(slot = 0; slot < HI3511_PCI_SLAVE_NUM; slot++)
		atomic_set(&host2slave_irq[slot],0);

	slot = 0;
	
	hi3511_init_sharebuf(slot, PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE, PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE,SHARE_MEMORY_SETSIZEFLAG);

	/*irq register not need when test*/
	result = request_irq(irq_number,pci_host_irq_handle,SA_SHIRQ,"hi35xx_pci_host",(void *)hi3511_pci_handle_table);
	if( result ){
		printk("Hi3511 PCI comm protocal can't get assigned irq %d\n",irq_number);
		result = -1;
		goto pci_vdd_init_out;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"request_irq ok %d",0);

	i = 0;
	// find the slave in slot, set it mem
	list_for_each (node, &hi3511_dev_head->node){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"before enable_int_hi_slot slot %d",0);
		pNode = list_entry(node, struct hi3511_dev, node);
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"before enable_int_hi_slot slot %d",1);
		
		hi3511_mem_map[i].base_addr_phy=pci_resource_start(pNode->pdev, 4);
		hi3511_mem_map[i].base_addr_virt=pNode->pf_base_addr;
		hi3511_mem_map[i].pdev=pNode->pdev;
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"before enable_int_hi_slot slot %d",2);

		enable_int_hi_slot(pNode->slot);
		//ENABLE_INT_HI_SLOT(pNode->slot);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"enable_int_hi_slot slot %d",pNode->slot);
	}

//	ENABLE_INT_HI_SLOT(HI_SLOT2);
//	ENABLE_INT_HI_SLOT(HI_SLOT3);

pci_vdd_init_out:

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"host pci_vdd_init end %d",result);

	while(hi3511_pci_vdd_info.mapped != 1)
		msleep(100);

	return result;
}
EXPORT_SYMBOL(pci_vdd_init);

/*
*	Host open Function
*	input: target_id   message_id   port: value[0,15] 
*	output:NULL
*	return: hanlde success, NULL failed
*	notes:
*	This function will find the handle by targetid messageid  and port,
*	input parameters value must <= 0xF
*	a handle can only opened once
*
*/
void *pci_vdd_open(int target_id, int message_id,int port)
{
	struct Hi3511_pci_data_transfer_handle *phandle;
	char *tmpbuf = NULL;
	int handle_pos = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"host pci_vdd_open %d",target_id);
	
	// target 3 bits, message 3 bits, port 4 bits
	//we can use handle_num in handle table
	if(target_id > HI3511_PCI_SLAVE_NUM){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"target id over load %d",target_id);
		return NULL;
	}

	// handle pos in handle table
	handle_pos = Hi3511_pci_handle_pos(target_id,message_id,port) ;

	if(handle_pos >= HI3511_PCI_HANDLE_TABLE_SIZE){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"target msg id should no more than %d",HI3511_PCI_HANDLE_TABLE_SIZE);
		return NULL;
	}
	
	// we should use mutex here for multithread open the same handle 	
	if(down_interruptible(&hipci_hostsemlock)){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"down_interruptible error %d",0);
	
		return NULL;
	}

	phandle = hi3511_pci_handle_table->handle[handle_pos];
	
	init_waitqueue_head(&send_tranfic_control_queue);

	if(NULL == phandle){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"kmalloc handle %d",0);

		phandle = kmalloc(sizeof(struct Hi3511_pci_data_transfer_handle),GFP_KERNEL);
		
		if(phandle == NULL){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"handle malloc error %d",0);
			return phandle;
		}
		memset((char *)phandle,0,sizeof(struct Hi3511_pci_data_transfer_handle));
		phandle->target = target_id & 0xF;
		phandle->msg = message_id & 0xF ;
		phandle->port =  port & 0xF;
		phandle->recvflage = 0;

		// set syn or asyn		
		phandle->synflage = 0;
		phandle->packnum = 0;
		phandle->packsize = 0;
		
		/* handle buffer initial */
		tmpbuf = vmalloc(sizeof(struct HI3511_pci_handle_buffer) + HI3511_PCI_HANDLE_BUFFER_SIZE + sizeof(struct Hi3511_pci_data_transfer_head));
		if(NULL == tmpbuf){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"phandle->buffer malloc error %d",0);
			kfree(phandle);
			return NULL;
		}
		phandle->buffer = (struct HI3511_pci_handle_buffer *) tmpbuf;
		phandle->buffer->buffer =(char *)(tmpbuf + sizeof(struct HI3511_pci_handle_buffer) + sizeof(struct Hi3511_pci_data_transfer_head)) ;
		
		phandle->buffer->buffersize = HI3511_PCI_HANDLE_BUFFER_SIZE;
		phandle->buffer->readpointer = 0;
		phandle->buffer->writepointer = 0;
		
		init_MUTEX(&phandle->buffer->read);
		init_MUTEX(&phandle->buffer->write);
		
		/* should initial sendto and receive func */
		phandle->pci_vdd_notifier_sendto = NULL;

		phandle->pci_vdd_notifier_recvfrom = NULL ;

		/*initial wait queue */
		init_waitqueue_head(&phandle->waitqueuehead);

		/* initial semaphore */
		init_MUTEX(&phandle->recvsemaphore);
		
		init_MUTEX(&phandle->sendsemaphore);
		
		hi3511_pci_handle_table->handle[handle_pos] = phandle;

	}
	else	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2," target %d msg %d port %d handle has been opened\n",target_id,message_id,port);
		phandle = NULL;
	}
	
	// release lock
	up(&hipci_hostsemlock);
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"host pci_vdd_open end %d",0);

	return (void *)phandle;
}

EXPORT_SYMBOL(pci_vdd_open);

int pci_vdd_target2slot(int target)
{
	if((target < 0) || (target > HI3511_PCI_SLAVE_NUM))
		return -1;
	
	return Hi3511_slave2host_mmap_info.mmapinfo[target - 1].slot;
}
EXPORT_SYMBOL(pci_vdd_target2slot);
#else

/***************************************
		slave code here 
****************************************/

/*
*	slave irq function
*/
#ifndef  CONFIG_PCI_TEST
static 
#endif
irqreturn_t pci_slave_irq_handle(int irq, void *dev_id, struct pt_regs *regs)
{
	int status = HI_HOST;
//	static DEFINE_SPINLOCK(irqlock);
	
//	spin_lock_irq(&irqlock);
	
#ifdef  CONFIG_PCI_TEST
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_slave_irq_handle %d",irq);	
#endif

	// check the status register of this share interrupt 
	// status if 
	status = hi3511_slave_interrupt_check();

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"status %d",status);
	switch(status){
		case HI_HOST: break;
		default :{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"*****interrupt error value status %d****",status);
			return IRQ_HANDLED;
		}
	}

	slave_int_disable();

	//add by c61104
	slave_int_clear();

	wake_up_process(precvthread);
	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"after wakeup receive thread %d",0);

	atomic_inc(&irq_num);

	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"irq_num is %d",irq_num.counter);

	if(tranfic_control_flag){
		wake_up(&send_tranfic_control_queue);
	}
	
//	spin_unlock_irq(&irqlock);

	//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave interrupt handler return",0);

	return IRQ_HANDLED;
}

EXPORT_SYMBOL(pci_slave_irq_handle);

/*
*	input: void
*	output:NULL
*	return: 0 success,others failed
*	Notes:
*	this initial function create handle table, create kernel receive thread, 
*	timer initial,register irq and so on
*	
*/
int pci_vdd_init(void)
{
	struct pci_vdd_info *pci_info = &hi3511_pci_vdd_info;
	unsigned long send_buffer;
	int  memsize;
	int result = 0;
	int irq_number =  30/*pci irq for slave is a const number */;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave pci_vdd_init %d",0);

	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	kcom_getby_uuid(UUID_MEDIA_MEMORY, &kcom);

	if(kcom ==NULL) {
		printk(KERN_ERR "MMZ: can't access mmz, start mmz service first.\n");
		return -1;
	}

	kcom_media_memory = kcom_to_instance(kcom, struct kcom_media_memory, kcom);
	
	slave_sharebuffer_start = kcom_media_memory->new_mmb("slave_share_buffer", PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE + fifo_stuff_size, SZ_64K, NULL);

	if(slave_sharebuffer_start == 0){

		PCI_HAL_DEBUG(0, "hil_mmb_alloc error %d", 1);
		result = -1;
		goto pci_vdd_init_out;
	}

	slave_recv_buffer = kcom_media_memory->remap_mmb(slave_sharebuffer_start);

	memset(slave_recv_buffer ,0x0, PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE + fifo_stuff_size);

	// set slave shared memory address to PCI bus
	hi3511_bridge_ahb_writel(PCIAHB_ADDR_PF, (slave_sharebuffer_start|0x00000001));

	exchange_buffer_addr_physics = kcom_media_memory->new_mmb("exchange_buffer", exchange_buffer_size, 0,NULL);

	if(exchange_buffer_addr_physics == 0){

		printk("hil_mmb_alloc error \n");
		result = -1;
		goto pci_vdd_init_out;
	}

	exchange_buffer_addr = kcom_media_memory->remap_mmb(exchange_buffer_addr_physics);

	memset(exchange_buffer_addr ,0x0, exchange_buffer_size);

	settings_addr_physics = kcom_media_memory->new_mmb("slave_datarelay", settings_buffer_size, 0,NULL);

	if(settings_addr_physics == 0){

		printk("hil_mmb_alloc error %d\n", 1);
		result = -1;
		goto pci_vdd_init_out;
		
	}

	datarelay = kcom_media_memory->remap_mmb(settings_addr_physics);

	memset(datarelay ,0x0, settings_buffer_size);

	init_hi3511_slave_device();

//	datarelay = ioremap(settings_addr_physics, settings_buffer_size);
//		if (datarelay == NULL) return -ENOMEM;
//		memset(datarelay, 0 , settings_buffer_size);


//	exchange_buffer_addr=ioremap(exchange_buffer_addr_physics, exchange_buffer_size);
//		if (exchange_buffer_addr == NULL) return -ENOMEM;
//		memset(exchange_buffer_addr, 0 , exchange_buffer_size);

 	//create transfer handle table
 	// get the mem size
 	memsize = sizeof(struct hi3511_pci_data_transfer_handle_table);

 	hi3511_pci_handle_table = kmalloc(memsize,GFP_KERNEL);

	if(hi3511_pci_handle_table == NULL){
		printk("hi3511_pci_handle_table kmalloc error\n");
		result = -1;
		goto pci_vdd_init_out;
	}
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"memset %d",0);
	
	memset(&hi3511_pci_vdd_info,0,sizeof(struct pci_vdd_info));
	
	memset(hi3511_pci_handle_table, 0 , memsize);
	
	// slave id
	pci_info->version = 0;
	
	pci_info->id = 1; 
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"init timer %d",0);

	/* initial timer */
	init_timer(&hi3511_pci_handle_table->timer);
	
	/* initial timer */
	init_timer(&hi3511_pci_handle_table->slavemap_timer);

	/*initial dma wait queue */
	init_waitqueue_head(&hi3511_pci_handle_table->dmawaitqueue);

	precvthread = kthread_create(hi3511_pci_kernelthread_deamon, NULL, "hi3511_pci_rec");

	if(IS_ERR(precvthread) <0)	{
		printk("create recv thread failed \n");
		result = -1;
		goto pci_vdd_init_out;
	}

	atomic_set(&threadexitflag,1);

	atomic_set(&slave2host_irq,0);
	
#ifndef  CONFIG_PCI_TEST

	/*irq register not need when test*/
	/*
	@param PCI_IRQ pci irq number
	@param pci_irq_handle irq service
	*/
	result = request_irq(irq_number,pci_slave_irq_handle,SA_INTERRUPT,"hi35xx_pci_slave",(void *)hi3511_pci_handle_table);

	if( result ){
		printk("Hi3511 PCI comm protocal can't get assigned irq %ld\n",(unsigned long)irq_number);
		result = -1;
		goto pci_vdd_init_out;
	}
#endif

	slave_int_enable();

	while (!(send_buffer = hi3511_bridge_ahb_readl(CPU_CIS_PTR))){
		msleep(100);
		// enable  host interrupt
	}

	hi3511_bridge_ahb_writel(CPU_CIS_PTR,0);

	hi3511_init_sharebuf(PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE, PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE, send_buffer, SHARE_MEMORY_SETSIZEFLAG);

	hi3511_pci_vdd_info.mapped = 1;
	
	//trigger an interrupt to inform host that slave's share buffer has been inited
	hi3511_bridge_ahb_writel(CPU_ICMD,0x00f00000);

pci_vdd_init_out:
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_vdd_info id %lu",hi3511_pci_vdd_info.id);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave pci_vdd_init end %d",result);
	
	return result;
}



EXPORT_SYMBOL(pci_vdd_init);

/*
*	Host open Function
*	input: target_id   message_id   port: value[0,15] 
*	output:NULL
*	return: hanlde success, NULL failed
*	notes:
*	This function will find the handle by targetid messageid  and port,
*	input parameters value must <= 0xF
*	a handle can only opened once
*
*/
void *pci_vdd_open(int target_id, int message_id,int port)
{
	struct Hi3511_pci_data_transfer_handle *phandle = NULL;
	unsigned char *buf = NULL;
	int handle_pos = 0;
	int ret;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave pci_vdd_open %d",target_id);
	
	// target 3 bits, message 3 bits, port 4 bits
	//we can use handle_num in handle table
	handle_pos = Hi3511_pci_handle_pos(target_id,message_id,port);

	if(handle_pos >= HI3511_PCI_HANDLE_TABLE_SIZE){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"target msg id should no more than %d",HI3511_PCI_HANDLE_TABLE_SIZE);
		return NULL;
	}

	// we should use mutex here for multithread open the same handle 	
	if(down_interruptible(&hipci_slavesemlock)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"slave down interruptible error %d",0);
		return NULL;
	}

	phandle = hi3511_pci_handle_table->handle[handle_pos];
	
	init_waitqueue_head(&send_tranfic_control_queue);

	if(phandle == NULL){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hanlde empty %d",0);

		phandle = kmalloc(sizeof(struct Hi3511_pci_data_transfer_handle),GFP_KERNEL);
		if(phandle == NULL){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"hanlde malloc error %d",0);
			ret = -1;	
		}

		memset((char *)phandle,0,sizeof(struct Hi3511_pci_data_transfer_handle));//c61104 20071121


		phandle->target = target_id&0x0F;//c61104 20071121
		phandle->msg = message_id&0x0F;//c61104 20071121
		phandle->port =  port&0x0F;//c61104 20071121
		phandle->recvflage = 0;
		
		phandle->packnum = 0;
		phandle->packsize = 0;	
		phandle->synflage = 0;

		/* handle buffer initial */
		buf = vmalloc(sizeof(struct HI3511_pci_handle_buffer) + sizeof(struct Hi3511_pci_data_transfer_head) + HI3511_PCI_HANDLE_BUFFER_SIZE);
		if(NULL == buf){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"phandle->buffer malloc error %d",0);
			kfree(phandle);
			return NULL;
		}
		
		phandle->buffer = (struct HI3511_pci_handle_buffer *) buf;
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->buffer 0x%8lX",(unsigned long)(phandle->buffer));		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hanlde buffer start 0x%8lX",(unsigned long)(buf + (sizeof(struct HI3511_pci_handle_buffer) + sizeof(struct Hi3511_pci_data_transfer_head))));
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hanlde buffer end 0x%8lX",(unsigned long)(buf + sizeof(struct HI3511_pci_handle_buffer) + sizeof(struct Hi3511_pci_data_transfer_head) + HI3511_PCI_HANDLE_BUFFER_SIZE));

		phandle->buffer->buffer =(char *)(buf + (sizeof(struct HI3511_pci_handle_buffer) + sizeof(struct Hi3511_pci_data_transfer_head)));

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hanlde buffer start 0x%8lX",(unsigned long)(phandle->buffer->buffer));
		phandle->buffer->buffersize = HI3511_PCI_HANDLE_BUFFER_SIZE;
		memset(phandle->buffer->buffer,0,HI3511_PCI_HANDLE_BUFFER_SIZE);
		phandle->buffer->readpointer = 0;
		phandle->buffer->writepointer = 0;
		
		init_MUTEX(&phandle->buffer->read);
		init_MUTEX(&phandle->buffer->write);
		
		/*initial wait queue */
		init_waitqueue_head(&phandle->waitqueuehead);

		/* initial semaphore */
		init_MUTEX(&phandle->recvsemaphore);
		
		init_MUTEX(&phandle->sendsemaphore);

		hi3511_pci_handle_table->handle[handle_pos] = phandle;
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle is 0x%lx",(unsigned long)phandle);

	}
	else	{
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1," target %d msg %d port %d handle has been opened\n",target_id,message_id,port);
		phandle = NULL;
	}
	
	// release lock
	up(&hipci_slavesemlock);	

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"slave pci_vdd_open end %d",0);
	
	return (void *)phandle;

}
EXPORT_SYMBOL(pci_vdd_open);

int pci_vdd_target2slot(int target)
{
	if((target < 0) || (target > HI3511_PCI_SLAVE_NUM))
		return -1;
	
	return Hi3511_slave_mmap_info.mmapinfo.slot;
}
EXPORT_SYMBOL(pci_vdd_target2slot);
#endif

/*
*	Send data Function
*	input: handle 	- genrate by pci_vdd_open fucntion
*		   buf	- data buffer address
*		  len		- data length
*		  flags 	-  send flage  flag & 0x1 == 0x1 asyn mode
*				    		      flag  & 0x2== 0x2  HIGH PRI package
*							flag  & 0x4 == DMA mode (buf should be phy addr, slave can not use dma mode)
*                                            flag & 0x8 == buffer addr type, 1 for physics address and 0 for virtual address	//c61104
*     output:NULL
*	return: hanlde success, NULL failed
*	notes:
*	This function will find the handle by targetid messageid  and port,
*	input parameters value must <= 0xF
*	a handle can only opened once
*
*/
int pci_vdd_sendto(void *handle, void *buf, int len, int flags)
{
	struct Hi3511_pci_data_transfer_handle *phandle;
	static struct Hi3511_pci_data_transfer_head head;
	
	int writeable = 0;
	int repeat = 0;	
	int lock = 0;
	int ret = 0;
	
	phandle= (struct Hi3511_pci_data_transfer_handle *)handle;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_sendto len %d",len);
	
	lock = down_interruptible(&phandle->sendsemaphore);
	if(lock) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"down handle send sema error %d",0);
		return -ERESTARTSYS;
	}
	
	if(0 == len){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"send size 0 %d",0);
	}

	if((phandle == NULL)||(buf == NULL)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"handle or buf empty %d",0);
		return -1;
	}
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"headsize %d head target %d msg %d port %d",sizeof(struct Hi3511_pci_data_transfer_head ),phandle->target,phandle->msg,phandle->port);
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"sendto flags %d",flags);

	if((flags & HI3511_PCI_HANDLEBUF_ADDR_TYPE)==0){		
		memcpy((char *)exchange_buffer_addr, buf, len);
		buf = (char *)exchange_buffer_addr_physics;
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"exchange buffer's address is 0x%lx \n",(unsigned long)buf);
	}

	if(flags & HI3511_PCI_PACKAGE_PRI_FLAG){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"PRI package %d",0);

		// may sleep here~!
		lock = hi3511_pci_locksend_priority_buffer(phandle->target);
		if(lock) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt hi3511_pci_add2buffer normal send %d",0);
			return -ERESTARTSYS;
		}

		// construct handle
		head.target = phandle->target & 0xF;
		head.msg = phandle->msg & 0xF;
		head.port = phandle->port & 0xF;
		head.src = hi3511_pci_vdd_info.id;
		head.length = len;
		head.oob = flags >> 20;
		head.reserved = 0x2D5A;
		
		// PRI is set by a bit of flag
		head.pri = HI3511_PCI_PACKAGE_PRI_LEVEL_2;	

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_locksend_priority_buffer %d",0);

		repeat = 0;

		do{		
			writeable = hi3511_pci_getsend_priority_rwpointer(head.target);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getsend_priority_rwpointer %d",writeable);
			if( writeable == PCI_WRITEABLE_PRIPACKAGE_FLAG ) {
				break;
			}
		
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"unwriteable %d",writeable);

			// PRI send out in 100 ms
			if(repeat >= 100){
				//printk("Value of repeat is %d in priority\n",repeat);
				atomic_add(1,&lostpacknum);
				hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"give up package! totel %d now\n",lostpacknum.counter);
				hi3511_pci_unlocksend_priority_buffer(phandle->target);

#ifdef 	CONFIG_PCI_HOST	
					// enable interrupt to send
				//printk("Value of phandle->target-1 is %x in priority",phandle->target - 1+2);
				// slot + 1 = target
				master_to_slave_irq(Hi3511_slave2host_mmap_info.mmapinfo[head.target - 1].slot);
#else
				slave_to_master_irq(0);
#endif
				ret =  -1;

				goto pci_vdd_sendto_out;
			}

			if(repeat >= 2)
			{
				// modify here to sleep in step : first in waitqueue, second sched()
				// sleep wait for data come wake up

				unsigned long flags;
				wait_queue_t wait;
	
				tranfic_control_flag = 1;
		
				init_waitqueue_entry(&wait, current);

				spin_lock_irqsave(&send_tranfic_control_queue.lock,flags);
				__add_wait_queue(&send_tranfic_control_queue, &wait);
				current->state = TASK_INTERRUPTIBLE;
				spin_unlock_irqrestore(&send_tranfic_control_queue.lock,flags);
			
				up(&phandle->sendsemaphore);

				schedule();
				
				tranfic_control_flag = 0;

				lock = down_interruptible(&phandle->sendsemaphore);
				if(lock) {
					hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"down handle send sema error %d",0);
					return -ERESTARTSYS;
				}
			
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"wake up by received interrupt! %d",0);

				spin_lock_irqsave(&send_tranfic_control_queue.lock,flags);
				__remove_wait_queue(&send_tranfic_control_queue, &wait);
				spin_unlock_irqrestore(&send_tranfic_control_queue.lock, flags);
			}		

			repeat++;

		}while(1);

		ret = hi3511_pci_add2buffer_priority(&head,buf,len,flags);
		if(ret < 0){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"hi3511_pci_add2buffer_priority error %d",0);
		}

		if(ret == PCI_PRI_SHARE_MEMORY_NOTENOUGH){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"send larger than buffer %d",0);
			hi3511_pci_unlocksend_priority_buffer(phandle->target);
			ret =  -1;
			goto pci_vdd_sendto_out;
		}

		//writeable = hi3511_pci_getsend_priority_rwpointer(head.target);//20071113
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"priority rwpointer %d",writeable);
			
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_add2buffer_priority %d",ret);	

		// send a priority package means timer need reset
		if(timer_pending(&hi3511_pci_handle_table->timer)){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"del_timer %d",0);
			del_timer(&hi3511_pci_handle_table->timer);
		}

		hi3511_pci_unlocksend_priority_buffer(phandle->target);

	}
	else {
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"normal package target %d",phandle->target);

		repeat = 0;
		
		do{
			// may sleep here~! 
			lock = hi3511_pci_locksend_normal_buffer(phandle->target);		
			if(lock){
				hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"user interrupt hi3511_pci_add2buffer normal send %d",0);
				ret = -ERESTARTSYS;
				goto pci_vdd_sendto_out;
			}
			
			// construct handle
			head.target = phandle->target & 0xF;
			head.msg = phandle->msg & 0xF;
			head.port = phandle->port & 0xF;
			head.src = hi3511_pci_vdd_info.id;
			head.length = len;
			head.oob = flags >> 20;
			head.reserved = 0x2D5A;
			head.pri = HI3511_PCI_PACKAGE_PRI_LEVEL_1;

			ret = hi3511_pci_add2buffer_normal(&head,buf,len,flags);
			
			hi3511_pci_unlocksend_normal_buffer(phandle->target);

			if(0 <  ret){
				break;
			}
			else {

				repeat++;
				
				// PRI send out in 200 ms
				if( repeat >= 100 ){
					atomic_add(1,&lostpacknum);

					hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"give up package! total %d now,ret %d repeat %d",lostpacknum.counter,ret,repeat);

					ret = -1;

#ifdef 	CONFIG_PCI_HOST	
					// enable interrupt to send
					//printk("Value of phandle->target-1 is %x in normal",phandle->target - 1+2);
					master_to_slave_irq(Hi3511_slave2host_mmap_info.mmapinfo[head.target - 1].slot);
#else		
					slave_to_master_irq(0);
#endif					
					goto pci_vdd_sendto_out;//return -1;
				}

				set_current_state(TASK_UNINTERRUPTIBLE);

				schedule_timeout(msecs_to_jiffies(1));

				if(repeat % 20 == 0){
					hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"irq to receive %d",0);
					
#ifdef 	CONFIG_PCI_HOST	
					// enable interrupt to send
					//printk("Value of phandle->target-1 is %x in normal delay",phandle->target - 1+2);
					master_to_slave_irq(Hi3511_slave2host_mmap_info.mmapinfo[head.target - 1].slot);
#else
					slave_to_master_irq(0);
#endif
				}
			}
	
		hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"repeat time is %d",repeat);

		}	while(1);
		
		if(!timer_pending(&hi3511_pci_handle_table->timer)){
			/* set send timer !*/
			hi3511_pci_handle_table->timer.expires = jiffies + msecs_to_jiffies(HI3511_PCI_TIMER_DELAY);

			hi3511_pci_handle_table->timer.data = (unsigned long) phandle->target;

#ifdef CONFIG_PCI_HOST

			hi3511_pci_handle_table->timer.function = hi3511_pci_host2slave_irq; //set slave interrupt func
#else
			hi3511_pci_handle_table->timer.function = hi3511_pci_slave2host_irq; //set slave interrupt func
#endif
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"ok ,here it is %ld!!!!\n",(unsigned long)hi3511_pci_handle_table->timer.function);	
			add_timer(&hi3511_pci_handle_table->timer);
		}
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"timer irq %lu",hi3511_pci_handle_table->timer.expires);

	}
	
pci_vdd_sendto_out:
	
	// wake up handle
	up(&phandle->sendsemaphore);
	
#ifdef CONFIG_PCI_HOST

	// add by chanjinn only for test cat problem 20071015
	/*{
#ifdef CONFIG_PCI_HOST
			struct slave2host_mmap_info *pmmapinfo ;
#else
			struct slave_mmap_info *pmmapinfo;
#endif
	
		struct hi3511_dev *pNode;
		struct list_head *node;
		int i = 0;
		int lock = 0;
		int readpointer = 0, writepointer = 0;
		unsigned long control;
		
		list_for_each(node, &hi3511_dev_head->node)
		{
				pNode = list_entry(node, struct hi3511_dev, node);
					
#ifdef CONFIG_PCI_HOST
				pmmapinfo = &Hi3511_slave2host_mmap_info.mmapinfo[i];
#else
				pmmapinfo = &Hi3511_slave_mmap_info.mmapinfo;
#endif
			
				i++ ;
			lock = down_interruptible(&pmmapinfo->send.normalsemaphore);
			if(lock) {
				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
			}
		
			// read / write data check
					// get read write pointer from share memory
#ifdef 	CONFIG_PCI_HOST	
				readpointer = __raw_readl((char *)pmmapinfo->send.readpointeraddr);
				writepointer = __raw_readl((char *)pmmapinfo->send.writepointeraddr);	
#else
				control = (4<<8)| 0x61;
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave read the host readpointer address is 0x%x \n",pmmapinfo->send.readpointeraddr);
				//hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_getsend_priority_rwpointer target %d",target);
				hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.readpointeraddr);
				hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics+read_pointer_physics_r));
				hi3511_bridge_ahb_writel(RDMA_CONTROL, control);
				//readpointer = *(unsigned long *)datarelay;
				while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
				readpointer=__raw_readl((volatile char *)(datarelay+read_pointer_physics_r));
				
				hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511 slave read the host writepointer address is 0x%x \n",pmmapinfo->send.writepointeraddr);
				hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, pmmapinfo->send.writepointeraddr);
				hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, (settings_addr_physics+write_pointer_physics_r));
				hi3511_bridge_ahb_writel(RDMA_CONTROL, control);
				//writepointer = *(unsigned long *)datarelay;
				while(hi3511_bridge_ahb_readl(RDMA_CONTROL)&0x00000001);	
				writepointer=__raw_readl((volatile char *)(datarelay+write_pointer_physics_r));					
#endif
			up(&pmmapinfo->send.normalsemaphore);	
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"writepointer %d readpointer %d after send",writepointer,readpointer);
		}
	}*/
#endif
	/*if(flags & (HI3511_PCI_HANDLEBUF_ADDR_TYPE)==0)
	{
	iounmap((char *)exchange_buffer_addr);
	}*/
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_sendto %d end",ret);

	return ret;
}

EXPORT_SYMBOL(pci_vdd_sendto);


/*
*	Receive data Function
*	input: handle 	- genrate by pci_vdd_open fucntion
*		   buf	- data buffer address
*		  len		- data length
*		  flags 	-  send flage  flag & 0x1 == 0x1 asyn mode
*				    		      flag  & 0x2== 0x2  HIGH PRI package
*							flag  & 0x4 == DMA mode (buf should be phy addr, slave can not use dma mode)
*	output:NULL
*	return: hanlde success, NULL failed
*	notes:
*	This function will find the handle by targetid messageid  and port,
*	input parameters value must <= 0xF
*	a handle can only opened once
*
*/
int pci_vdd_recvfrom(void *handle, void *buf, int len, int flags)
{
	struct Hi3511_pci_data_transfer_handle *phandle = (struct Hi3511_pci_data_transfer_handle *) handle;
	int repeat = 0;
	int ret = 0;

	// lock handle
	// syn recv by default
	int lock;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_recvfrom buffer 0x%8lX",(unsigned long)buf);
	
	if((phandle == NULL) || (buf == NULL)) {
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle or buf NULL %d",0);
		return -1;
	}

	do{
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"in while repeat is %d",repeat);
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->recvsemaphore %d",0);

		lock = down_interruptible(&phandle->recvsemaphore);
		if(lock) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"user interrupt hi3511_pci_add2buffer normal send %d",0);
			return -ERESTARTSYS;
		}

		hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"phandle->packnum %d",phandle->packnum);
		
		if(phandle->recvflage == PCI_SHARE_MEMORY_DATA_IN_PRIORITY) {

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"priority data in %d",phandle->recvflage);

			// receive pir data in share memory
			ret = hi3511_pci_readonepackfromsharebuffer_priority(phandle,buf,flags);

			phandle->recvflage = PCI_SHARE_MEMORY_DATA_NOTIN;

			break;
		}
		/* phandle buffer empty! check if any data in share memory?S */
		else if(phandle->recvflage & PCI_SHARE_MEMORY_DATA_IN_NORMAL) {

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"normal data in readonepackfromsharebuffer normal %d",0);

			ret = hi3511_pci_readonepackfromsharebuffer_normal(phandle,buf,flags);
			phandle->recvflage = PCI_SHARE_MEMORY_DATA_NOTIN;

			break;
		}
		
		if(phandle->packnum != 0){
			if((ret = hi3511_pci_recvfromhandlebuffer(phandle,buf)) >= 0){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"break %d",0);
				break;
			}
			
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->packnum %d",phandle->packnum);
		}
		
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"phandle->synflage %d",phandle->synflage);	

		if(phandle->synflage == PCI_SHARE_MEMORY_RECV_SYN) {

			// modify here to sleep in step : first in waitqueue, second sched()
			// sleep wait for data come wake up
			unsigned long flags;

			wait_queue_t wait;
			wait_queue_head_t *q = &phandle->waitqueuehead;

			init_waitqueue_entry(&wait, current);

			spin_lock_irqsave(&q->lock,flags);
			__add_wait_queue(q, &wait);
			current->state = TASK_INTERRUPTIBLE;
			spin_unlock_irqrestore(&q->lock,flags);

			// release handle
			up(&phandle->recvsemaphore);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"syn wait sleep %d",0);

			if(precvthread->state ==  TASK_INTERRUPTIBLE){
				wake_up_process(precvthread);				
			}

			schedule();

			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"wake up by demon %d",0);
			
			spin_lock_irqsave(&q->lock,flags);
			__remove_wait_queue(q, &wait);
			spin_unlock_irqrestore(&q->lock, flags);

			continue;

		}
		else {
						
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"error break %d",0);

			// asy break!
			break;
		}

		repeat++;
		if(repeat >= 2){
			atomic_add(1,&lostpacknum);
			hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"recv timeout %d",lostpacknum.counter);
			ret = -1;
			break;
		}
	}	while(1); //10 s for syn

	// release handle
	up(&phandle->recvsemaphore);
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_recvfrom end %d",ret);

	return ret;
}

EXPORT_SYMBOL(pci_vdd_recvfrom);


/*
*	close handle Function
*	input: handle 	- genrate by pci_vdd_open fucntion
*	output:NULL
*	return: hanlde success, NULL failed
*	notes:
*	This function will find the handle by targetid messageid  and port,
*	input parameters value must <= 0xF
*	a handle can only opened once
*
*/
void pci_vdd_close(void *handle)
{
	struct Hi3511_pci_data_transfer_handle *phandle = (struct Hi3511_pci_data_transfer_handle *)handle;
	int handle_pos;
	int i = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_close %d",0);
	
	if(phandle == NULL){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle null %d",0);
		return ;
	}
	
	// we should use mutex here for multithread open the same handle 	
	if(down_interruptible(&phandle->recvsemaphore)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
		return;
	}

	if(down_interruptible(&phandle->sendsemaphore)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
		return;
	}
	
	//should rec buffer data in handle 
	while(phandle->packnum!=0 ){
	
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"package not empty %d",phandle->packnum);

		up(&phandle->recvsemaphore);
		up(&phandle->sendsemaphore);
	
		//release lock!!		
		wake_up(&phandle->waitqueuehead);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"wake up handle wait %d",0);
		
		// sleep 10 ms
		msleep_interruptible(HI3511_PCI_WAIT_HANDLEBUF_EMPTY);

		// we should use mutex here for multithread open the same handle 	
		if(down_interruptible(&phandle->recvsemaphore)){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
			return;
		}

		if(down_interruptible(&phandle->sendsemaphore)){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
			return;
		}
	
		i++;

		if(i > HI3511_PCI_WAIT_TIMES_HANDLEBUF_EMPTY){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"wait time out %d",0);
			break;
		}
	}

	vfree(phandle->buffer);

	phandle->buffer = NULL;

	atomic_add(phandle->packnum,&lostpacknum);

	// clear buffer
	phandle->packnum = 0;
	
	phandle->packsize = 0;
	
	// flush flags
	phandle->recvflage = 0;

	phandle->synflage = 0;
	
	// handle pos in handle table
	handle_pos = Hi3511_pci_handle_pos(phandle->target,phandle->msg,phandle->port);
	if(handle_pos >= 1024){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"target msg id should no more than %d",1024);
		return;
	}
	
	// release lock
	up(&phandle->recvsemaphore);	
	up(&phandle->sendsemaphore);


#ifdef CONFIG_PCI_HOST
	hi3511_pci_host2slave_irq(phandle->target); //set slave interrupt func
#else
	hi3511_pci_slave2host_irq(phandle->target); //set slave interrupt func
#endif
	
	memset(phandle,0,sizeof(struct Hi3511_pci_data_transfer_handle));
	
	// free handle
	kfree(phandle);

	hi3511_pci_handle_table->handle[handle_pos] = NULL;
	
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_close end %d",0);

	return;
}
EXPORT_SYMBOL(pci_vdd_close);


/*
*	get channel handle option Function
*	input: handle 	- genrate by pci_vdd_open fucntion
*	output:opt	- channel option
*	return: 0 success, others failed
*	notes:
*	This function get the channel option by handle,
*
*/

int pci_vdd_getopt(void *handle, pci_vdd_opt_t *opt)
{
	struct Hi3511_pci_data_transfer_handle *phandle =(struct Hi3511_pci_data_transfer_handle *) handle;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_getopt flag %lx, recvflage %lx",(unsigned long )opt->flags,(unsigned long)phandle->recvflage);

	if(phandle == NULL){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle empty %d",0);
		return -1;
	}
	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"phandle is not null in getopt %d",0);


	// lock the source 
	if(down_interruptible(&phandle->recvsemaphore)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
		return -ERESTARTSYS;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"down recv success in getopt %d",0);

	if(down_interruptible(&phandle->sendsemaphore)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);
		return -ERESTARTSYS;
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"down send success in getopt %d",0);


	opt->sendto_notify = phandle->pci_vdd_notifier_sendto;
	
	opt->recvfrom_notify = phandle->pci_vdd_notifier_recvfrom;

	//c61104  Be attention: there is a difference recvflage and flag
	//flag has a bit, two statue and is used to describe the package is pri or not 
	//recvflage has two bit, four statue to describe package existance and pri or normal attribute of it
	//as an result, when package is pri, the value of recvflage is 0x2, but flag is 0x1 at the same time
	if(((phandle->recvflage)&0x3) | ((phandle->recvflage) & 0x80)){
		opt->flags |= ((((phandle->recvflage)&0x3) -1) << 1);
		opt->flags |= ((phandle->recvflage) & 0x80);
	}
	
	opt->flags |= phandle->synflage;

	opt->data = phandle->data;
	
	// unlock the source
	up(&phandle->recvsemaphore);

	up(&phandle->sendsemaphore);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_getopt end %lx",(unsigned long)opt->flags);	

	return 0;
}
EXPORT_SYMBOL(pci_vdd_getopt);


// set channel handle option
/*
*	set channel handle option Function
*	input: handle 	- genrate by pci_vdd_open fucntion
*		   opt     - channel option
*	output:NULL
*	return: 0 success, others failed
*	notes:
*	This function set the channel option by handle and option,
*
*/
int pci_vdd_setopt(void *handle, pci_vdd_opt_t *opt)
{
	struct Hi3511_pci_data_transfer_handle *phandle =(struct Hi3511_pci_data_transfer_handle *) handle;
	int i = 0;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"pci_vdd_setopt %d",0);
	
	// lock the handle
	if(down_interruptible(&phandle->recvsemaphore)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);	
		return -ERESTARTSYS;
	}
	
	if(down_interruptible(&phandle->sendsemaphore)){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);	
		return -ERESTARTSYS;
	}
	
	// data in handle buffer should send or recv first
	// buffer in handle designed for recv asyn or syn
	while( phandle->packnum != 0 ){

		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"package not empty %d",phandle->packnum);

		// unlock the source
		up(&phandle->recvsemaphore);
	
		wake_up(&phandle->waitqueuehead);

		hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"wake up handle wait %d",0);

		// sleep 10 ms
		msleep_interruptible(HI3511_PCI_WAIT_HANDLEBUF_EMPTY);

		if(down_interruptible(&phandle->recvsemaphore)){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"user interrupt %d",0);	
			return -ERESTARTSYS;
		}
		
		i++;

		if(i > HI3511_PCI_WAIT_TIMES_HANDLEBUF_EMPTY){
			hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"wait time out %d",0);
			break;
		}
	}

	if( phandle->packnum != 0 )
	{
		atomic_add(phandle->packnum,&lostpacknum);
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"opt set %d",0);
	
	// after rec buffer in handle, set opt
	phandle->pci_vdd_notifier_sendto = opt->sendto_notify ;
	
	phandle->pci_vdd_notifier_recvfrom = opt->recvfrom_notify;

	phandle->recvflage = (opt->flags & 0x2 ) >> 1;

	if(opt->flags & 0x80){
		phandle->recvflage |= 0x80; //c61104 20071120 for received package.
	}
	else{
		phandle->recvflage &= 0xffffff7f;
	}
	
	phandle->synflage = (opt->flags & 0x1);

	phandle->packnum = 0;

	phandle->packsize = 0;

	phandle->data = opt->data;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"synflage %d ,recvflage %x",phandle->synflage,phandle->recvflage);
	
	// unlock the source
	up(&phandle->recvsemaphore);	

	up(&phandle->sendsemaphore);
	
	return 0;
}
EXPORT_SYMBOL(pci_vdd_setopt);


/*
*	get pci info Function
*	input: NULL 
*	output:info - pci protocal info,such as version,slot number,id
*	return: 0 success, others failed
*	notes:
*	This function get the channel option by handle,
*
*/
int pci_vdd_getinfo(pci_vdd_info_t *info)
{
	if(info == NULL)
		return -1;

#ifndef 	CONFIG_PCI_TEST
	// not mapped yet
	if(!hi3511_pci_vdd_info.mapped){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"memory not mapped %d",0);	
//		return -1;
	}
#endif
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_vdd_info %ld",hi3511_pci_vdd_info.id);
	info->version = hi3511_pci_vdd_info.version;
	info->id         = hi3511_pci_vdd_info.id;
	info->bus_slot = hi3511_pci_vdd_info.bus_slot;
	info->slave_map = hi3511_pci_vdd_info.slave_map;
	info->mapped = hi3511_pci_vdd_info.mapped;
	return 0;
}

EXPORT_SYMBOL(pci_vdd_getinfo);


/*
*	module exit Function
*	input: NULL 
*	output:NULL
*	return: 0 success, others failed
*	notes:
*	pci protocal module exit when this function called
*
*/
void pci_vdd_cleanup(void)
{
	struct Hi3511_pci_data_transfer_handle * phandle;
	unsigned long i;
	int retval;
	unsigned long ret_mmz;
#ifdef CONFIG_PCI_HOST	
	int irqnumber = 0;
#endif

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_vdd_cleanup %d",0);	

	if(hi3511_pci_handle_table == NULL) {
		return;
	}
	
	for(i = 0; i < HI3511_PCI_HANDLE_TABLE_SIZE; i++) {
		
		phandle = hi3511_pci_handle_table->handle[i];
		
		// clear handle
		if( NULL != phandle ) {
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle %ld not null",i);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle msg %d not null",phandle->msg);
			
			if(phandle->packnum != 0) {

				hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"list package buffer %d",phandle->packnum);
			}

			vfree(phandle->buffer);

			phandle->buffer = NULL;
			
			atomic_add(phandle->packnum,&lostpacknum);

			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle free %ld not null",(unsigned long)phandle);			

			kfree(phandle);
			
			hi3511_pci_handle_table->handle[i] = NULL;	

		}
		else{
			hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"handle %ld null",i);
		}
	}

	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"del timer %d",0);

	retval = del_timer(&hi3511_pci_handle_table->timer);
	if(retval){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_handle_table->timer has been run %d",retval);	
	}
	
	retval = del_timer(&hi3511_pci_handle_table->slavemap_timer);
	if(retval){
		hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"hi3511_pci_handle_table->slavemap_timer has been run %d",retval);	
	}

	atomic_sub(1,&threadexitflag);
#ifdef  CONFIG_PCI_TEST

	prewake = jiffies;
#endif
	hi3511_dbgmsg(HI3511_DBUG_LEVEL1,"wake up and exit thread %d",0);

	kthread_stop(precvthread);
//	wake_up_process(precvthread);

//	msleep(10);
	
#ifdef CONFIG_PCI_HOST
	irqnumber = list_entry(hi3511_dev_head->node.next, struct hi3511_dev, node)->irq;
	// free host irq
	free_irq(irqnumber,(void *)hi3511_pci_handle_table);

	//iounmap(host_recv_buffer);
	
	ret_mmz = kcom_media_memory->unmap_mmb(host_recv_buffer);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"ret of host recv buffer mmz unmap is %lu",ret_mmz);
	kcom_media_memory->delete_mmb(host_sharebuffer_start);


#else
	kcom_media_memory->unmap_mmb(slave_recv_buffer);

	kcom_media_memory->delete_mmb(slave_sharebuffer_start);
    
	free_irq(30,(void *)hi3511_pci_handle_table);

#endif
	kfree(hi3511_pci_handle_table);

	hi3511_pci_handle_table = NULL;

	hi3511_dbgmsg(HI3511_DBUG_LEVEL3,"total  %d packs lost",lostpacknum.counter);
	lostpacknum.counter = 0;
	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"pci_vdd_cleanup end %d",0);

	ret_mmz = kcom_media_memory->unmap_mmb(exchange_buffer_addr);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"ret of exchange buffer mmz unmap is %lu",ret_mmz);
	kcom_media_memory->delete_mmb(exchange_buffer_addr_physics);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"after exchange buffer delete %d",0);
//iounmap(exchange_buffer_addr);

#ifndef CONFIG_PCI_HOST

//	iounmap(datarelay);
	kcom_media_memory->unmap_mmb(datarelay);

        kcom_media_memory->delete_mmb(settings_addr_physics);

//	hi3511_slave_device_cleanup();
#else
	#ifdef CONFIG_PCI_TEST
		hi3511_slave_device_cleanup();
	#endif
#endif

/*	hi3511_dbgmsg(HI3511_DBUG_LEVEL5,"before fifo stuff %d",0);
	kcom_media_memory->unmap_mmb(fifo_stuff);

	kcom_media_memory->delete_mmb(fifo_stuff_addr_physics);
*/
	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"before kcom put %d",0);
	kcom_put(kcom);

	hi3511_dbgmsg(HI3511_DBUG_LEVEL2,"after kcom put %d",0);
//	return;
}
EXPORT_SYMBOL(pci_vdd_cleanup);

module_init(pci_vdd_init);
module_exit(pci_vdd_cleanup);

MODULE_AUTHOR("chanjinn");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PCI communicate interface for Hi3511 Solution");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
