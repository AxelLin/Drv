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
#include <linux/pci.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>
#include "hisi_pci_hw.h"
#include "hisi_pci_data_common.h"
#include "hisi_pci_proto.h"
#include "hi3511_pci.h"

struct hisi_pcidev_map hisi_map_pcidev;

/* pci communicate handle table */
struct hisi_pci_transfer_handle *hisi_handle_table[HISI_PCI_HANDLE_TABLE_SIZE] = {0};

extern struct hisi_pci_dev_list hisi_pci_dev;
extern void *vmalloc(unsigned long size);
static void slave_mapped_info(unsigned long data)
{
	struct hisi_pcicom_dev *dev = NULL;
	unsigned int i = 0;
	
	/* host map */	
	if(!hisi_pci_vdd_info.id && !hisi_pci_vdd_info.mapped ){
		printk("\nVDD WARNING: slave memory map time out!\n");
		printk("VDD WARNING: host find %d slaves,but mapped %u\n",hisi_pci_dev.list_num,hisi_map_pcidev.num);
		for(; i < HISI_MAX_PCI_DEV; i++){
			if(hisi_map_pcidev.map[i].dev){
				dev = hisi_map_pcidev.map[i].dev;
				printk("mapped slave:dev 0x%8lx target %u irq %d pdev 0x%8lx devfn %u bus number %u\n",dev, dev->slot_id,dev->irq,dev->pdev,  dev->pdev->devfn, dev->pdev->bus->number);
			}
		}
		/* timeout mapped */
		hisi_pci_vdd_info.mapped = 1;
	}
}

/*
*  initdata_to_slave only send initial info from host to slave after slave memory initialled
*  send  slot_id to the slave
*/
static void initdata_to_slave(struct hisi_pcicom_dev *dev)
{
        void *handle;
        int ret;
	int val;
        handle = pci_vdd_open(dev->slot_id, 0, HISI_PCI_PRIORITY_PACK);
        if(!handle){
                printk("INITIAL MEMORY ERROR: pci_vdd_open error\n");
                return;
        }

	val = dev->slot_id;
        ret = pci_vdd_sendto(handle, (void *)&val, sizeof(unsigned int), HISI_PCI_INITIAL_MEM_DATA);
        if(ret < 0){
                printk("INITIAL MEMORY ERROR: pci_vdd_sendto error %d\n",ret);
        }
        pci_vdd_close(handle);
}

static void recv_priority(struct hisi_pcicom_dev *dev)
{
	struct hisi_pci_transfer_handle *handle = NULL;
	struct hisi_pci_transfer_head *head = NULL;
	unsigned int ret, pos = 0;
	unsigned int len;
	void *data ;

	mcc_trace(1, "pos  %u\n", pos);
	while(!hisi_is_priority_buffer_empty(dev)){
		head = hisi_get_priority_head(dev);
		
		/* recv data just under head */
		data = (void *)head + sizeof(struct hisi_pci_transfer_head);
		len = head->length;
		mcc_trace(1, "head->magic 0x%x head 0x%8lx len %d \n", head->magic,(unsigned long)head, head->length);
		if(head->magic == HISI_HEAD_MAGIC_INITIAL){
			/* means initial data from host */
			dev->slot_id = readl(data);

			/* info id == slot_id */
			hisi_pci_vdd_info.id = dev->slot_id;
			mcc_trace(1, "dev->slot_id 0x%lx, data 0x%8lx\n", (unsigned long)dev->slot_id, (unsigned long)data);
		}
		else{
			pos = head->src << HISI_PCI_PORT_NEEDBITS | head->port;
			handle = hisi_handle_table[pos];
			mcc_trace(1, "handle 0x%8lx\n", (unsigned long) handle);
			if(handle && handle->pci_vdd_notifier_recvfrom){
				ret = handle->pci_vdd_notifier_recvfrom(handle, data, len);
			}
	 	}
		/* modifiy normal read point */
		hisi_priority_data_recieved(dev, data, len);
	}
}

static void recv_normal(struct hisi_pcicom_dev *dev)
{
	struct hisi_pci_transfer_handle *handle = NULL;
	struct hisi_pci_transfer_head *head = NULL;
	unsigned int pos,ret;
	void *data;
	while(!hisi_is_normal_buffer_empty(dev)){
		mcc_trace(1, "dev->slot_id 0x%8lx\n", (unsigned long)dev->slot_id);
		head = hisi_get_normal_head(dev);
		pos = head->src << HISI_PCI_PORT_NEEDBITS | head->port;
		handle = hisi_handle_table[pos];
		data = (void *)head + sizeof(struct hisi_pci_transfer_head);

		mcc_trace(1, "src %u port %u\n", head->src, head->port);
		mcc_trace(1, "pos %d handle 0x%8lx\n", pos, (unsigned long)handle);

		if(handle && handle->pci_vdd_notifier_recvfrom){
			mcc_trace(1, "hanlde 0x%8lx notify recv\n", (unsigned long)handle);
			ret = handle->pci_vdd_notifier_recvfrom(handle, data, head->length);
		}

		/* modifiy normal read point */
		hisi_normal_data_recieved(dev, data, head->length);
	}
}

static irqreturn_t host_isr_before_map(void)
{
	struct list_head *entry;
	unsigned int status = -1;
	irqreturn_t flage = IRQ_NONE;
	list_for_each(entry, &hisi_pci_dev.list){
		struct hisi_pcicom_dev *dev = list_entry(entry, struct hisi_pcicom_dev, node);
		status = dev->check_irq(dev->slot_id - 1);
		//status = (readl(dev->cfg_base_addr + PCI_ISTATUS) == 0x00f00000 ? 1:0);
		mcc_trace(1, "irq status %d\n",status);
		if (status){
			/* disable and clear outer irq */
			//hisi_host_int_disable(dev);
			dev->disable_irq(dev->slot_id - 1);
	
			/* put struct pcicom dev to map */
			hisi_map_pcidev.map[dev->slot_id].dev = (void *)dev;

			/* get phy addr from pci dev */
			hisi_map_pcidev.map[dev->slot_id].base_addr_phy = pci_resource_start(dev->pdev, 4);

			hisi_map_pcidev.map[dev->slot_id].base_addr_virt = dev->pf_base_addr;

			hisi_map_pcidev.num ++;
	
			hisi_shared_mem_init(dev);
			
			initdata_to_slave(dev);

			//hisi_host_int_clear(dev);
			dev->clear_irq(dev->slot_id - 1);
			
			/* enable outer irq */
			//hisi_host_int_enable(dev);
			dev->enable_irq(dev->slot_id - 1);

			flage = IRQ_HANDLED;
		}
	}
	
	mcc_trace(1, "mapped num %u  total num %u\n",hisi_map_pcidev.num, hisi_pci_dev.list_num);
	/* all dev have been mapped, we need change to after mapped irq function */
	if ((hisi_map_pcidev.num == hisi_pci_dev.list_num) && (hisi_pci_dev.list_num)){
		mcc_trace(1, "slave mapped %d\n",status);
		hisi_pci_vdd_info.mapped = 1;
	}

	return flage;
}

static irqreturn_t host_isr_after_map(void)
{
	irqreturn_t flage = IRQ_NONE;
        struct list_head *entry;
	unsigned long irq_flage;
        int status;
	struct hisi_pcicom_dev *dev;
	int i;
#if 0
	list_for_each(entry, &hisi_pci_dev.list){
		local_irq_save(irq_flage);
		/* slot_id from 0 */
                dev = list_entry(entry, struct hisi_pcicom_dev, node);
		status = dev->check_irq(dev->slot_id - 1);

                if (status > 0){
			/* disable and clear outer irq */
			dev->disable_irq(dev->slot_id - 1);
			dev->clear_irq(dev->slot_id - 1);
		}
		local_irq_restore(irq_flage);

		mcc_trace(1, "irq status 0x%8lx dev->slot_id %d\n",status, dev->slot_id);
                if (status > 0){
			recv_priority(dev);
			recv_normal(dev);

			/* enable outer irq */
			dev->enable_irq(dev->slot_id - 1);
			flage = IRQ_HANDLED;
		}
	}
#else
	for(i = 0; i < HISI_MAX_PCI_DEV; i++){
		if(hisi_map_pcidev.map[i].dev){
			dev = hisi_map_pcidev.map[i].dev;
			local_irq_save(irq_flage);
			status = dev->check_irq(dev->slot_id - 1);
			//printk("--- %08x ----\n", status);
			if((status&0xf00000) == 0xf00000){
				/* disable and clear outer irq */
				dev->disable_irq(dev->slot_id - 1);
				dev->clear_irq(dev->slot_id - 1);
			}
			local_irq_restore(irq_flage);
			mcc_trace(1, "irq status 0x%8lx dev->slot_id %d\n",status, dev->slot_id);
			if((status&0xf00000) == 0xf00000){
				recv_priority(dev);
				recv_normal(dev);
				
				/* enable outer irq */
				dev->enable_irq(dev->slot_id - 1);
				flage = IRQ_HANDLED;
			}
		}
	}
#endif
#if 0
    if(flage != IRQ_HANDLED)
        printk("(");
    else
        printk("[");
#endif
	return flage;
}

static irqreturn_t hisi_pci_isr_host(int irq, void *dev_id, struct pt_regs *regs)
{
	if(hisi_pci_vdd_info.mapped)
		return host_isr_after_map();	
	else
		return host_isr_before_map();	
}

static irqreturn_t hisi_pci_isr_slave(int irq, void *dev_id, struct pt_regs *regs)
{
	irqreturn_t flage = IRQ_NONE;
        struct list_head *entry;
	unsigned long irq_flage;
        int status = -1;
	struct hisi_pcicom_dev *dev;

        list_for_each(entry, &hisi_pci_dev.list){
		local_irq_save(irq_flage);

		dev = list_entry(entry, struct hisi_pcicom_dev, node);
		/* slot_id from 0 */
                //status = hisi_slave_int_check(dev);
		status = dev->check_irq(0);
		//printk("--- %08x ----\n", status);
		if((status&0xf0000) == 0xf0000){
			/* disable and clear outer irq */
			//hisi_slave_int_disable();
			dev->disable_irq(0);

			//hisi_slave_int_clear();
			dev->clear_irq(0);
		}
		local_irq_restore(irq_flage);

		mcc_trace(1, "irq status %d\n",status);
        if ((status&0xf0000) == 0xf0000){

			recv_priority(dev);
			recv_normal(dev);

			/* enable outer irq */
			//hisi_slave_int_enable();
			dev->enable_irq(0);
			flage = IRQ_HANDLED;
		}
	}
	return flage;
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
	irqreturn_t (*handler)(int, void *, struct pt_regs *);
	struct hisi_pcicom_dev *dev;
	struct list_head *entry;
	int ret;

	memset(&hisi_map_pcidev, 0 , sizeof(struct hisi_pcidev_map));

        /* host info id is zero */
        /* shared memory in slave */
        if(hisi_pci_vdd_info.id){
		/* slave irq func */
		handler = hisi_pci_isr_slave;
		mcc_trace(1, "slave handler 0x%8lx\n",(unsigned long) handler);
	}
	else{
		handler = hisi_pci_isr_host;
		mcc_trace(1, "host handler 0x%8lx\n",(unsigned long) handler);
	}

	list_for_each(entry, &hisi_pci_dev.list){
		dev = list_entry(entry, struct hisi_pcicom_dev, node);
		mcc_trace(1, "irq register %d slot_id %d\n",dev->irq, dev->slot_id);
		
		/* host share pci irq, slave share dma and doorbell irq*/
		ret = request_irq(dev->irq, handler, SA_SHIRQ, "hisi pci irq", dev);
		if (ret){
			printk(KERN_ERR "PCI: pci_vdd_init request irq error!\n");
			goto pci_vdd_init_failture;
		}
		/* slave have been initialled memory, send a irq to host initial */
        	if(hisi_pci_vdd_info.id){
			hisi_shared_mem_init(dev);

			/*FIXME chanjinn */
			/* slave enable host's irq */
			writel(0x00f00000, PCI_VIRT_BASE + CPU_ICMD);
			//hisi_slave_int_enable();
			dev->enable_irq(0);

			/* slave only have host[0] */
			if(!hisi_map_pcidev.map[0].dev){
				hisi_map_pcidev.map[0].dev = dev;
			}
		}
	}

	/* irq function need to change when some slave memory mapped time out */
	init_timer(&hisi_map_pcidev.map_timer);

	hisi_map_pcidev.map_timer.expires = jiffies + HISI_PCI_MAX_MAP_TIME_OUT;
	hisi_map_pcidev.map_timer.function = slave_mapped_info;
	add_timer(&hisi_map_pcidev.map_timer);
	
	return 0;

pci_vdd_init_failture:
	return ret;
}

/*
*	module exit Function
*	input: NULL 
*	output: NULL
*	return: NULL 
*	notes:
*	pci protocal module exit when this function called
*
*/
void pci_vdd_cleanup(void)
{
	memset(&hisi_map_pcidev, 0, sizeof(struct hisi_pcidev_map));
}

/*
*	Host open Function
*	input: target_id [0: 5]  port: value[0,5] 
	       priority 0 normal, 1 imediatelly
*	output:NULL
*	return: hanlde success, NULL failed
*	notes:
*	This function will find the handle by targetid and port,
*	input parameters value must <= 0x3F
*	a handle can only opened once
*
*/
void *pci_vdd_open(int target_id, int port, int priority)
{
	struct hisi_pci_transfer_handle *handle = NULL;
	unsigned int pos;
	
	if(target_id < 0 || target_id >= HISI_MAX_PCI_DEV){
                printk(KERN_ERR "VDD ERROR: target_id %d out of range [0 - %d].\n",target_id, HISI_MAX_PCI_DEV - 1);
		return NULL; 
	}

	if(port < 0 || port >= HISI_MAX_PCI_PORT){
                printk(KERN_ERR "VDD ERROR: port %d out of range [0 - %d].\n",port, HISI_MAX_PCI_PORT - 1);
                return NULL;
	} 

	if(!hisi_map_pcidev.map[target_id].dev){
		printk(KERN_ERR "VDD ERROR: pci device in target %d not exist\n",target_id);
		return NULL;
	}

	pos = target_id << HISI_PCI_PORT_NEEDBITS | port;
	handle = hisi_handle_table[pos];
	mcc_trace(1, "pos %d handle 0x%8lx\n",pos, (unsigned long)handle);
	if(handle){
                printk(KERN_ERR "VDD ERROR: handle already exist target_id %d port %d\n",target_id, port);
                return NULL;
	}

	handle = kmalloc(sizeof(struct hisi_pci_transfer_handle), GFP_KERNEL);
	if(!handle){
                printk(KERN_ERR "VDD ERROR: kmalloc error\n");
                return NULL;		
	}

	handle->target_id = target_id & HISI_PCI_DEV_BITS;	/* target_id 6 bits*/
	handle->port = port & HISI_PCI_PORT_BITS;		/* port 10 bits */
	handle->priority = priority;				/* channel priority attribution */
	handle->pci_vdd_notifier_recvfrom = NULL;
	atomic_set(&handle->sendmutex, 0); 			/* sendmutex set 0 */

	hisi_handle_table[pos] = handle;

	return handle;
}
EXPORT_SYMBOL(pci_vdd_open);

/*
*	is host check Function
*	input: NULL 
*	output:NULL
*	return: 0 host, slot_id slave
*	notes:
*
*/
int pci_vdd_localid(void)
{
	return hisi_pci_vdd_info.id; 
}
EXPORT_SYMBOL(pci_vdd_localid);

int pci_vdd_remoteids(int ids[])
{
        struct hisi_pcicom_dev *dev;
        struct list_head *entry;
	int i = 0;
	list_for_each(entry, &hisi_pci_dev.list){
		dev = list_entry(entry, struct hisi_pcicom_dev, node);
		if(hisi_pci_vdd_info.id){
			/* slave */
			ids[i] = 0;
		}else{
			/* host */
			ids[i] = dev->slot_id;
		}
		i++;
	}
	return i;
}
EXPORT_SYMBOL(pci_vdd_remoteids);


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
	struct hisi_pci_transfer_handle *p = handle;
	unsigned int pos = 0;
	if(handle){
		pos = (p->target_id << HISI_PCI_PORT_NEEDBITS | p->port);
		mcc_trace(1, "handle  0x%8lx \n",(unsigned long) handle);
		hisi_handle_table[pos] = 0;
		kfree(handle);
	}
}
EXPORT_SYMBOL(pci_vdd_close);

/*
*	Send data Function
*	input: handle 	- genrate by pci_vdd_open fucntion
*	buf	- data buffer address
*	len	- data length
*	flage 	- HISI_PCI_NORMAL_MEM_DATA normal data, HISI_PCI_INITIAL_MEM_DATA initial data from host 
*       output:NULL
*	return: send data length if success , error number if failed
*	notes:
*	This function can not sleep or schedule
*
*/
int pci_vdd_sendto(void *handle, const void *buf, unsigned int len, int flage)
{
	struct hisi_pci_transfer_handle *p = handle;
	struct hisi_pcicom_dev *dev;
	struct hisi_pci_transfer_head head;
	int init = 0;
	int ret;
    unsigned long flags;
    
	memset(&head, 0, sizeof(struct hisi_pci_transfer_head));

	if(!buf){
		printk(KERN_ERR "VDD ERROR: sendto buffer NULL!\n");
		return -1;
	}
	if(!handle){
                printk(KERN_ERR "VDD ERROR: sendto handle NULL!\n");
                return -1;
	}
	if (len < 0 || len >= HISI_PCI_SHARED_SENDMEM_SIZE ){
                printk(KERN_ERR "VDD ERROR: sendto length out of range!\n");
                return -1;
	}
	
	local_irq_save(flags);
		
	atomic_inc(&p->sendmutex);
	if(atomic_read(&p->sendmutex) != 1){
		atomic_dec(&p->sendmutex);
		local_irq_restore(flags);
		printk(KERN_ERR "VDD ERROR: sendto handle used right now!\n");
		return -1;
	}

	head.target_id = p->target_id;
	head.port = p->port;
	head.src = hisi_pci_vdd_info.id;
	head.length = len;
	head.check = HISI_HEAD_CHECK_FINISHED; 

	if(flage & HISI_PCI_INITIAL_MEM_DATA){
		head.magic = HISI_HEAD_MAGIC_INITIAL;
		init = 1;
	}

	dev = hisi_map_pcidev.map[p->target_id].dev;

	if((p->priority & HISI_PCI_PRIORITY_PACK) || (init)){
		/* priority package need send immediatelly */
		mcc_trace(1, "send priority len %u \n",len);
		ret = hisi_send_priority_data(dev, buf, len, &head);
	}
	else{
		mcc_trace(1, "send normal len %u \n",len);
		/* normal package need send when buffer full or timeout */
		ret = hisi_send_normal_data(dev, buf, len, &head);
	}
	atomic_dec(&p->sendmutex);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(pci_vdd_sendto);

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
	struct hisi_pci_transfer_handle *p = handle;
	unsigned long flags;
	if (!p){
		printk(KERN_ERR "VDD ERROR: getopt handle NULL!\n");
		return -1;
	}
	local_irq_save(flags);
	opt->recvfrom_notify = p->pci_vdd_notifier_recvfrom;
	opt->data = p->data;
	local_irq_restore(flags);

	return 0;
}
EXPORT_SYMBOL(pci_vdd_getopt);

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
        struct hisi_pci_transfer_handle *p = handle;
        unsigned long flags;
        if (!p){
                printk(KERN_ERR "VDD ERROR: getopt handle NULL!\n");
                return -1;
        }
        local_irq_save(flags);
	
	p->data = opt->data;
	p->pci_vdd_notifier_recvfrom = opt->recvfrom_notify;	

        local_irq_restore(flags);

        return 0;
}
EXPORT_SYMBOL(pci_vdd_setopt);

module_init(pci_vdd_init);
module_exit(pci_vdd_cleanup);
MODULE_AUTHOR("chanjinn");
MODULE_DESCRIPTION("PCI communicate vdd layer for Hisilicon PCI Communicate Solution");

