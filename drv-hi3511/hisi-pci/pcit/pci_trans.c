/*copyright (c) 2008 Hisilicon Co., Ltd. 
 *
 * This program is free software; you can redistribute it and/or modify
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
 * 20080630 Kanglin Chen <c42025@huawei.com>
 */


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/arch/hardware.h>
#include <linux/kthread.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/poll.h>

#include <linux/string.h>
#include <linux/kcom.h>
#include <kcom/mmz.h>

#include "../drv/include/hi3511_pci.h"
#include "pci_trans.h"

#define hiwdt_readl(x)		readl(IO_ADDRESS(HIWDT_REG(x)))
#define hiwdt_writel(v,x)	writel(v, IO_ADDRESS(HIWDT_REG(x)))

#define PCIT_IRQ_NR 0x1e

void slave_int_door_disable(void)
{
    hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)&0xfff0ffff));
}

void slave_int_door_clear(void)
{
   // hi3511_bridge_ahb_writel(CPU_ISTATUS,0x000f0000);
    hi3511_bridge_ahb_writeb(CPU_ISTATUS + 0x2,0x0f);
}

void slave_int_door_enable(void)
{
    hi3511_bridge_ahb_writel(CPU_IMASK,(hi3511_bridge_ahb_readl(CPU_IMASK)|0x000f0000));
}

void slave_int_dma_disable(void)
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

void ss_doorbell_triggle(unsigned int addr, unsigned int value)
{
    writel(value << 16, addr + PCI_ICMD);
}
EXPORT_SYMBOL(ss_doorbell_triggle);


int hi3511_slave_interrupt_check(void)
{
    unsigned int status;
    status=PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3;
    if(interrupt_check(status)){
        if(hi3511_bridge_ahb_readl(CPU_ISTATUS)&(PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3))
            return HI_HOST;
    }
    return -1;
}


/*************************************************************************/
  
/*************************************************************************/

#define PCIT_DEBUG  4
#define PCIT_INFO   3
#define PCIT_ERR    2
#define PCIT_FATAL  1
#define PCIT_CURR_LEVEL 0
#define PCIT_PRINT(level, s, params...)   do{ \
	if(level <= PCIT_CURR_LEVEL)  \
	printk("[%s, %d]: " s "\n", __FUNCTION__, __LINE__, ##params); \
}while(0)

#define PCIT_DMA_DIR(t) (t->dir & 1)
#define PCIT_DMA_TASK_NR 8
#define PCIT_DMA_TASK_CUR list_entry(busy_task_head.next,struct pcit_dma_task, list)

#define PCIT_EVENTS_NR 4
#define PCIT_DEVICE_NR (HI_PCIT_MAX_SLOT * HI_PCIT_MAX_BUS)
#define PCIT_SUBSCRIBER_NR (PCIT_DEVICE_NR << 1)

#define PCIT_MK_DEVIDX(bus,slot) (slot + bus * HI_PCIT_MAX_SLOT)
#define PCIT_DEVINX2BUS(idx) (idx/HI_PCIT_MAX_SLOT)
#define PCIT_DEVINX2SLOT(idx) (idx%HI_PCIT_MAX_SLOT)

struct pcit_subscriber_desc
{
    char name[16];   /* name of this subscriber */
    void (*notify)(unsigned int bus, unsigned int slot, struct pcit_event *, void *);
    void *data;
    
    struct list_head subscriber;  /* all subscribers list */
    struct pcit_dev_desc *devs[PCIT_DEVICE_NR];  /* all interestin devices */
    unsigned int event_mask_table[PCIT_DEVICE_NR]; /* all interesting events  */

    struct pcit_event events[PCIT_EVENTS_NR]; /* events buffer */
    unsigned int write;
    unsigned int read;
};

struct pcit_dev_desc
{
    unsigned int bus;
    unsigned int slot;
    wait_queue_head_t wqh;

    struct hi3511_dev *hidev;

    void  *shm_virt_addr;
    unsigned long shm_phys_addr;
    unsigned int shm_size;    
    int         (*is_host)(int slot);
};

extern struct hi3511_dev hi3511_device_head;

static struct list_head free_task_head = LIST_HEAD_INIT(free_task_head);
static struct list_head busy_task_head = LIST_HEAD_INIT(busy_task_head);
static struct pcit_dma_task task_array[PCIT_DMA_TASK_NR];
static int is_host = 0;

DECLARE_MUTEX(dma_task_mutex);

static struct pcit_subscriber_desc *all_subscribers[PCIT_SUBSCRIBER_NR] = {0};
static struct pcit_dev_desc *all_pcit_devs[PCIT_DEVICE_NR] = {0};
static pcit_subscriber_id_t app_subscriber_id = HI_PCIT_INVAL_SUBSCRIBER_ID;

DECLARE_MUTEX(subscriber_sema);


/* share memory size, only on device. */
/*static int shm_size = 0;
module_param(shm_size, uint, S_IRUGO);

static unsigned long shm_phys_addr = 0;
module_param(shm_phys_addr, ulong, S_IRUGO);
*/
void start_dma_task(struct pcit_dma_task *new_task);
static int g_taskNum = 0;

#define PCIT_DMA_TIMEOUT_JIFFIES 10
static struct timer_list g_stDmaTimeOutTimer;
void PCIT_TimerProc(unsigned long  __data)
{
    unsigned long lockflag;
    
    local_irq_save(lockflag);
    if(!list_empty(&busy_task_head))
    {
        printk("PCI lost local DMA interrupt. Restart again! %d \n", g_taskNum);
        start_dma_task(PCIT_DMA_TASK_CUR);
    }
    local_irq_restore(lockflag);
}


void start_dma_task(struct pcit_dma_task *new_task) 
{
    unsigned int control;

    g_stDmaTimeOutTimer.expires = jiffies + PCIT_DMA_TIMEOUT_JIFFIES;
    g_stDmaTimeOutTimer.function = PCIT_TimerProc;
    add_timer(&g_stDmaTimeOutTimer);
     
    if (PCIT_DMA_DIR(new_task) == HI_PCIT_DMA_WRITE){
        control = (new_task->len << 8 ) | 0x79;
		hi3511_bridge_ahb_writel(WDMA_PCI_ADDR, new_task->dest);
		hi3511_bridge_ahb_writel(WDMA_AHB_ADDR, new_task->src);
		hi3511_bridge_ahb_writel(WDMA_CONTROL, control);
		return ;
	}

	control =(new_task->len << 8) | 0x69;
    hi3511_bridge_ahb_writel(RDMA_PCI_ADDR, new_task->src);
    hi3511_bridge_ahb_writel(RDMA_AHB_ADDR, new_task->dest);
    hi3511_bridge_ahb_writel(RDMA_CONTROL, control);
    
    
    return ;
}

struct pcit_dma_task * __pcit_create_task(struct pcit_dma_task *task)
{
    struct list_head *list;
    struct pcit_dma_task *new_task;
    unsigned long flags;
    /*host dma enable
    if (is_host) {
        PCIT_PRINT(PCIT_ERR,"_not_ support DMA operation on host!\n");
        return NULL;        
    }*/

    local_irq_save(flags);
    if (list_empty(&free_task_head)){
        PCIT_PRINT(PCIT_ERR,"too many busy task!\n");
        local_irq_restore(flags);
        return NULL;
    }
    list = free_task_head.next;
    list_del(list);
    new_task = list_entry(list, struct pcit_dma_task, list);
    memcpy(new_task, task, sizeof(*task));

    g_taskNum++;
    
    /* if no task in busy list, we should start the task. */
    if (list_empty(&busy_task_head))
    {
        list_add_tail(list, &busy_task_head);
        start_dma_task(new_task); 
    }
    else
    {
        list_add_tail(list, &busy_task_head);
    }
      
    local_irq_restore(flags);
    return new_task;
}


int pcit_create_task(struct pcit_dma_task *task)
{
    if (NULL == __pcit_create_task(task))
        return -1;
    return 0;
}
EXPORT_SYMBOL(pcit_create_task);

unsigned long pcit_get_window_base(int slot, int flag)
{

    unsigned long addr = 0;
    
    switch(flag){
        case HI_PCIT_PCI_NP:
            addr = all_pcit_devs[slot]->hidev->np_base_addr_phy;
            break;
        case HI_PCIT_PCI_PF:
            addr = all_pcit_devs[slot]->hidev->pf_base_addr_phy;
            break;
        case HI_PCIT_PCI_CFG:
            addr = all_pcit_devs[slot]->hidev->cfg_base_addr_phy;
            break;
        case HI_PCIT_AHB_PF:
            addr = all_pcit_devs[slot]->hidev->shm_phys_addr;
            break;
    default :
            break;
    }
    return addr;
}
EXPORT_SYMBOL(pcit_get_window_base);




void default_task_finish(struct pcit_dma_task *task)
{
    PCIT_PRINT(PCIT_DEBUG, "wake up a task.\n");
    wake_up(&all_pcit_devs[(unsigned int)task->private_data]->wqh);
    return ;
}

int do_dma_req(struct pcit_dma_req __user *req, int dir)
{
    struct pcit_dev_desc *dev_desc;
    struct pcit_dma_task dma_task, *new_task;
    struct pcit_dma_req dma_req;
    DECLARE_WAITQUEUE(wait, current);
    int ret = 0;
    /*host dma enable
    if (is_host){
        PCIT_PRINT(PCIT_ERR, "Not support inquire bus in device!\n");
        return -EOPNOTSUPP;
    }*/
    if (copy_from_user(&dma_req, req, sizeof(dma_req))){
        PCIT_PRINT(PCIT_ERR,"bad address\n");
        return -EFAULT;                
    }
   
    dma_req.bus = HI_PCIT_HOST_BUSID;
    dma_req.slot = HI_PCIT_HOST_SLOTID; 
    
    dev_desc = all_pcit_devs[PCIT_MK_DEVIDX(dma_req.bus, dma_req.slot)];
    dma_task.dir = dir;
    dma_task.src = dma_req.source;
    dma_task.dest = dma_req.dest;
    dma_task.len = dma_req.len;    
    dma_task.state = 0;
    dma_task.finish = default_task_finish;
    dma_task.private_data = (void *)PCIT_MK_DEVIDX(dma_req.bus, dma_req.slot);

    PCIT_PRINT(PCIT_DEBUG, "do_dma_req: bus = %u, slot = %u, dir = %d,"
        "src = %lx, dst = %lx, len = %u \n", dma_req.bus, dma_req.slot,
        dir, dma_req.source, dma_req.dest, dma_req.len);

    down(&dma_task_mutex);       
    add_wait_queue(&dev_desc->wqh, &wait);
    new_task = __pcit_create_task(&dma_task);
    if (! new_task)
        goto err1;        

    PCIT_PRINT(PCIT_DEBUG, "waiting a task.....\n");

    /* If this DMA task has bee competed so soon, we need no wait. */
    while (new_task->state != 2) {
        set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(10);
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto err1;
		}
        PCIT_PRINT(PCIT_DEBUG, "state = %d.\n",new_task->state);    
    }
    PCIT_PRINT(PCIT_DEBUG, "finished a task.\n");

err1:
	remove_wait_queue(&dev_desc->wqh, &wait);
	up(&dma_task_mutex);
	return ret;
}

void default_user_notify(unsigned int bus, unsigned int slot, 
    struct pcit_event *event, void *data)
{
    unsigned int write;
    struct pcit_subscriber_desc *user = all_subscribers[app_subscriber_id];

    /* if this buffer is full, we will overwrite the oldest one. */
    if (PCIT_EVENTS_NR - 1 == user->write- user->read)
        user->read++;

    write = (user->write) & (PCIT_EVENTS_NR - 1);
    memcpy(&user->events[write], event, sizeof(*event));
    user->write++;
    wake_up(&all_pcit_devs[PCIT_MK_DEVIDX(bus,slot)]->wqh);

    PCIT_PRINT(PCIT_DEBUG, "wake up : r = %u, w = %u\n",user->read, user->write);
}

int pcit_subscribe_event(pcit_subscriber_id_t id, unsigned int bus,
    unsigned int slot, struct pcit_event *pevent)
{
    struct pcit_subscriber_desc *user;
    struct pcit_dev_desc *dev_desc;
    unsigned int dev_idx;

    if (! is_host) {
        bus = HI_PCIT_HOST_BUSID;
        slot = HI_PCIT_HOST_SLOTID;
    }
    dev_idx = PCIT_MK_DEVIDX(bus, slot);

    if (id < 0 || id >= PCIT_SUBSCRIBER_NR) {
        PCIT_PRINT(PCIT_ERR, "id error!\n");       
        return -1;
    }
    down(&subscriber_sema);
    user = all_subscribers[id];
    if (! user) {
        PCIT_PRINT(PCIT_ERR, "no such subscriber!\n");       
        goto id_err;
    }

    if (bus >= HI_PCIT_MAX_BUS || slot >= HI_PCIT_MAX_SLOT) {
        PCIT_PRINT(PCIT_ERR, "bus (%u), or slot (%u) illegal!\n", bus, slot);       
        goto id_err;
    }
    dev_desc = all_pcit_devs[dev_idx];
    if (! dev_desc) {
        PCIT_PRINT(PCIT_ERR, "so such dev_idx :bus (%u), or slot (%u)!\n", bus, slot);
        goto id_err;
    }

    user->event_mask_table[dev_idx] |= pevent->event_mask;
    user->devs[dev_idx] = dev_desc;
    up(&subscriber_sema);  
    return 0;
    
id_err:    
    up(&subscriber_sema);    
    return -1;
}

EXPORT_SYMBOL(pcit_subscribe_event);

int pcit_unsubscribe_event(pcit_subscriber_id_t id, unsigned int bus,
    unsigned int slot, struct pcit_event *pevent)
{
    struct pcit_subscriber_desc *user;
    struct pcit_dev_desc *dev_desc;
    unsigned int dev_idx;

    if (! is_host) {
        bus = HI_PCIT_HOST_BUSID;
        slot = HI_PCIT_HOST_SLOTID;
    }
    dev_idx = PCIT_MK_DEVIDX(bus, slot);
    
    if (id < 0 || id >= PCIT_SUBSCRIBER_NR) {
        PCIT_PRINT(PCIT_ERR, "id <%d> error!\n", id);       
        return -1;
    }
    down(&subscriber_sema);
    user = all_subscribers[id];
    if (! user) {
        PCIT_PRINT(PCIT_ERR, "no such subscriber!\n");       
        goto id_err;
    }
    if (bus >= HI_PCIT_MAX_BUS || slot >= HI_PCIT_MAX_SLOT){
        PCIT_PRINT(PCIT_ERR, "bus (%u), or slot (%u) illegal!\n", bus, slot);       
        goto id_err;
    }
    /* PCI device do _NOT_ support hot P&P, so this's safe. */
    dev_desc = user->devs[dev_idx];
    if (! dev_desc){
        PCIT_PRINT(PCIT_ERR, "so such device :bus (%u), or slot (%u)!\n", bus, slot);
        goto id_err;
    }

    /* if user unsubscribe all event on this device ,
     * we wak up process on this device.
     */
    user->event_mask_table[dev_idx] &= ~ pevent->event_mask;
    if (! user->event_mask_table[dev_idx]) {        
        //wake_up(&dev_desc->wqh);
        user->devs[dev_idx] = NULL;
    }
    up(&subscriber_sema);  
    return 0;
    
id_err:    
    up(&subscriber_sema);    
    return -1;
}

EXPORT_SYMBOL(pcit_unsubscribe_event);

pcit_subscriber_id_t pcit_subscriber_register(struct pcit_subscriber *subscriber)
{
    struct pcit_subscriber_desc *new_user;
    int i;

    down(&subscriber_sema);
    for (i = 0; i < PCIT_SUBSCRIBER_NR; i++) {
        if(all_subscribers[i])
            continue;
        break;
    }
    if (PCIT_SUBSCRIBER_NR == i) {
        PCIT_PRINT(PCIT_ERR, "too many subscirber!\n");
        goto no_id;
    }   

    new_user = kmalloc(sizeof(*new_user), GFP_KERNEL);
    if (!new_user){
        PCIT_PRINT(PCIT_ERR, "no memory for new subscriber!\n");
        goto no_mem;
    }
    all_subscribers[i] = new_user;
    memset(new_user, 0, sizeof(*new_user));
    
    /* Don't check if there has the same name. */
    strncpy(new_user->name, subscriber->name, 16);
    new_user->notify = subscriber->notify;
    new_user->data = subscriber->data;
    up(&subscriber_sema);    
    return i;

no_mem:    
no_id:
    up(&subscriber_sema);
    return HI_PCIT_INVAL_SUBSCRIBER_ID;
}
EXPORT_SYMBOL(pcit_subscriber_register);

int pcit_subscriber_deregister(pcit_subscriber_id_t id)
{
    struct pcit_subscriber_desc *user;
    struct pcit_event pevent = {0xffffffff, 0};
    int i;

    if (id < 0 || id >= PCIT_SUBSCRIBER_NR) {
        PCIT_PRINT(PCIT_ERR, "id <%d> error!\n", id);       
        return -1;
    }
    PCIT_PRINT(PCIT_DEBUG, "deregister : %d\n", id);
    
    down(&subscriber_sema);
    user = all_subscribers[id];
    if (! user) {
        PCIT_PRINT(PCIT_ERR, "no such subscriber!\n");       
        goto id_err;
    }    
    for (i = 0; i < PCIT_DEVICE_NR; i++){ 
        if (user->devs[i]) {
            up(&subscriber_sema);                
            pcit_unsubscribe_event(id, user->devs[i]->bus, 
                user->devs[i]->slot, &pevent);
            down(&subscriber_sema);
        }
    }
    kfree(user);
    all_subscribers[id] = NULL;
    
id_err:    
    up(&subscriber_sema);    
    return -1;

}
EXPORT_SYMBOL(pcit_subscriber_deregister);


static int hi3511_pcit_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int hi3511_pcit_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t hi3511_pcit_read(struct file *file, char *buf,
		size_t count, loff_t *ppos)
{
	return -ENOSYS;
}


static ssize_t hi3511_pcit_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	return -ENOSYS;
}

static int hi3511_pcit_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
    PCIT_PRINT(PCIT_DEBUG, "cmd = %d", _IOC_NR(cmd));
    switch (_IOC_NR(cmd)) {
        case _IOC_NR(HI_IOC_PCIT_INQUIRE):{

            unsigned int bus_nr = (unsigned int)((struct pcit_bus_dev *)arg)->bus_nr;
			struct pcit_bus_dev bus_devs = {0};
            struct hi3511_dev *hidev;
			unsigned int i = 0;
                        unsigned int j = 0;
            for (i = 0; i < HI_PCIT_MAX_SLOT; i++)
                bus_devs.devs[i].slot = HI_PCIT_NOSLOT;

            if (! is_host){
                hidev = all_pcit_devs[0]->hidev;
                bus_devs.devs[0].pf_phys_addr = all_pcit_devs[0]->shm_phys_addr;
                bus_devs.devs[0].pf_size = all_pcit_devs[0]->shm_size;
                PCIT_PRINT(PCIT_DEBUG, "Inquire pf: addr = %lx, size = %lu\n", 
                    bus_devs.devs[0].pf_phys_addr, bus_devs.devs[0].np_size);
                goto end;
            }
            
            bus_devs.bus_nr = bus_nr;
            for (i = 0; i < PCIT_DEVICE_NR; i++) {
                if (!all_pcit_devs[i] || all_pcit_devs[i]->bus != bus_nr)
                    continue;
                
                hidev = all_pcit_devs[i]->hidev;
                    j = PCIT_DEVINX2SLOT(i);
		    bus_devs.devs[j].slot = all_pcit_devs[i]->slot;
		    bus_devs.devs[j].np_phys_addr = pci_resource_start(hidev->pdev, 3);
		    bus_devs.devs[j].np_size = pci_resource_len(hidev->pdev, 3);
		    bus_devs.devs[j].pf_phys_addr = pci_resource_start(hidev->pdev, 4);
		    bus_devs.devs[j].pf_size = pci_resource_len(hidev->pdev, 4);
		    bus_devs.devs[j].cfg_phys_addr = pci_resource_start(hidev->pdev, 5);
		    bus_devs.devs[j].cfg_size = pci_resource_len(hidev->pdev, 5);

		    bus_devs.devs[j].vendor_id = hidev->pdev->vendor;
		    bus_devs.devs[j].device_id = hidev->pdev->device;
		    PCIT_PRINT(PCIT_DEBUG, "hidev addr = 0x%lx, bus_dev slot  = 0x%lx np_phys_addr 0x%lx pf_phys_addr 0x%lx cfg_phys_addr 0x%lx\n",(unsigned long)hidev,(unsigned long)bus_devs.devs[j].slot,(unsigned long)bus_devs.devs[j].np_phys_addr,(unsigned long)bus_devs.devs[j].pf_phys_addr, (unsigned long)bus_devs.devs[j].cfg_phys_addr ); 
            }
end:
			if (copy_to_user((void *)arg, &bus_devs, sizeof(bus_devs))){
				PCIT_PRINT(PCIT_ERR,"bad address\n");
				return -EFAULT;
			}
			return 0;
		}
		case _IOC_NR(HI_IOC_PCIT_DMARD):
            return do_dma_req((struct pcit_dma_req __user *)arg, HI_PCIT_DMA_READ);
		case _IOC_NR(HI_IOC_PCIT_DMAWR):
            return do_dma_req((struct pcit_dma_req __user *)arg, HI_PCIT_DMA_WRITE);
        case _IOC_NR(HI_IOC_PCIT_BINDDEV): {
            unsigned int devid;
            if (! is_host){
                file->private_data = all_pcit_devs[0];
                return 0;
            }
            
            if (get_user(devid, (unsigned int __user *)arg))
                return -EFAULT;

            if (HI_PCIT_DEV2BUS(devid) >= HI_PCIT_MAX_BUS ||
                HI_PCIT_DEV2SLOT(devid) > HI_PCIT_MAX_SLOT ||
                ! all_pcit_devs[HI_PCIT_DEV2SLOT(devid) + HI_PCIT_DEV2BUS(devid) * HI_PCIT_MAX_SLOT] ){
                PCIT_PRINT(PCIT_ERR, "dev (%u) bus (%u), slot (%u) is illegal",
                    devid, HI_PCIT_DEV2BUS(devid), HI_PCIT_DEV2SLOT(devid));
                return -EINVAL;
            }
            file->private_data = all_pcit_devs[HI_PCIT_DEV2SLOT(devid) + HI_PCIT_DEV2BUS(devid) * HI_PCIT_MAX_SLOT];
            return 0;
        }
		case _IOC_NR(HI_IOC_PCIT_DOORBELL):{
            struct pcit_dev_desc *dev_desc = (struct pcit_dev_desc *)file->private_data;
            unsigned int event;

            if (! is_host) {
                dev_desc = all_pcit_devs[0];
            }
            if (get_user(event, (unsigned int __user *)arg))
                return -EFAULT;
            if (! dev_desc) {
                PCIT_PRINT(PCIT_ERR, "unkonw device!\n");
                return -EINVAL;
            }            
            if (is_host && (! dev_desc->hidev)){
                panic("kao.........!\n");
                return -EINVAL;
            }            
            if ( event > HI_PCIT_EVENT_DOORBELL_7) {
                PCIT_PRINT(PCIT_ERR, "unsupport event!\n");
                return -EINVAL;
            }
            PCIT_PRINT(PCIT_DEBUG, "send doorbell to device(%u, %u) %p!\n",
                dev_desc->bus, dev_desc->slot, dev_desc->hidev);
            if (is_host) {
                unsigned long status;
                unsigned long timeout = 10;
                while (timeout--) {
                    status = readl(dev_desc->hidev->cfg_base_addr + PCI_ISTATUS);                    
                    if (! (status & 0xf0000))
                        break;
                    schedule_timeout(1);
                }
                writel((event << 16) & 0xf0000,
                    dev_desc->hidev->cfg_base_addr + PCI_ICMD);
            } else {
                /* we _NOT_ gurantee this doorbell received successfully by host. 
                 * application has to make sure that this event is handled by host.
                 */
                hi3511_bridge_ahb_writel(CPU_ICMD, (event << 20) & 0xf00000);
            }
            return 0;
		}
        case _IOC_NR(HI_IOC_PCIT_SUBSCRIBE) : {
            struct pcit_dev_desc *dev_desc = (struct pcit_dev_desc *)file->private_data;
            struct pcit_event pevent;
            unsigned int event;

            if (! is_host) {
                dev_desc = all_pcit_devs[0];
            }
            if (get_user(event, (unsigned int __user *)arg))
                return -EFAULT;
            if (! dev_desc) {
                PCIT_PRINT(PCIT_ERR, "unknown device!\n");
                return -EINVAL;
            }
            pevent.event_mask = event;
            return pcit_subscribe_event(app_subscriber_id,
                dev_desc->bus, dev_desc->slot, &pevent);
        }        
        case _IOC_NR(HI_IOC_PCIT_UNSUBSCRIBE) :{
            struct pcit_dev_desc *dev_desc = (struct pcit_dev_desc *)file->private_data;
            struct pcit_event pevent;
            unsigned long event;

            if (! is_host) {
                dev_desc = all_pcit_devs[0];
            }            
            if (get_user(event, (unsigned int __user *)arg))
                return -EFAULT;
            if (! dev_desc) {
                PCIT_PRINT(PCIT_ERR, "unkonw device!\n");
                return -EINVAL;
            }
            pevent.event_mask = event;
            return pcit_unsubscribe_event(app_subscriber_id,
                dev_desc->bus, dev_desc->slot, &pevent);            
        }        
        case _IOC_NR(HI_IOC_PCIT_LISTEN):{
            struct pcit_dev_desc *dev_desc;
            struct pcit_subscriber_desc *user;
            struct pcit_event tmp_event;
            unsigned long flags;
            unsigned int read;
            int ret = 0;

            DECLARE_WAITQUEUE(wait, current);
            PCIT_PRINT(PCIT_DEBUG, "enter....\n");

            user = all_subscribers[app_subscriber_id];
            dev_desc = (struct pcit_dev_desc *)file->private_data;
            if (! is_host) {
                dev_desc = all_pcit_devs[0];
            }            
            if (! dev_desc) {
                PCIT_PRINT(PCIT_ERR, "unknown device!\n");
                return -EINVAL;
            }            
            //down(&subscriber_sema);
            add_wait_queue(&dev_desc->wqh, &wait);
            local_irq_save(flags);
            
            PCIT_PRINT(PCIT_DEBUG, "w = %u, r = %u\n", user->write, user->read);
            while (user->write == user->read) {
                local_irq_restore(flags);
    			set_current_state(TASK_INTERRUPTIBLE);
    			schedule_timeout(10);
    			if (signal_pending(current)) {
    				ret = -ERESTARTSYS;
    				goto err;
    			}   
                /* if waked up by unsubcribing */
                if (! user->devs[PCIT_MK_DEVIDX(dev_desc->bus, dev_desc->slot)]) {
                    PCIT_PRINT(PCIT_DEBUG, "waked up by unsubcribing!\n");
                    ret = -ERESTARTSYS;
                    goto err;
                }
            }
            read = user->read & (PCIT_EVENTS_NR - 1);
            memcpy(&tmp_event, &user->events[read],sizeof(struct pcit_event));            
            user->read++;
            local_irq_restore(flags); 

            /* maybe we lost a event, but now we ignore it. */
            if (copy_to_user((void *)arg, &tmp_event, sizeof(struct pcit_event))) {
                ret = -EFAULT;            
                goto err;
            }
err:
            remove_wait_queue(&dev_desc->wqh, &wait);
            //up(&subscriber_sema);
            return ret;
		}
		default:
		       return -ENOIOCTLCMD;
	}
    return 0;
}

unsigned int hi3511_pcit_poll(struct file * file, poll_table *wait) 
{
    return 0;
}

static struct file_operations hi3511_dma_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.read		= hi3511_pcit_read,
	.write		= hi3511_pcit_write,
	.ioctl		= hi3511_pcit_ioctl,
	.open		= hi3511_pcit_open,
	.release	= hi3511_pcit_release,
	.poll       = hi3511_pcit_poll,
};

static struct miscdevice hi3511_dma_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "hi3511_pcit",
	.fops		= &hi3511_dma_fops,
};

irqreturn_t pci_host_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    struct pcit_subscriber_desc *user;
    struct pcit_event pevent;
    struct hi3511_dev *hidev;
    unsigned long status,ahb_status,ahb_dma_event,ahb_event, doorbell, lockflag;
    struct pcit_dma_task *cur_task;
    unsigned int bus, slot, hislot;
    unsigned int event;
    int i, handled = 0;

    PCIT_PRINT(PCIT_DEBUG, "Host receive a interrupt!\n");  

    /* check interrupt status, doorbell or DMA ? */
    ahb_status = hi3511_bridge_ahb_readl(CPU_ISTATUS);  

    /* handle doorbell interrupt and PCI DMA interrupt */
    ahb_event = ahb_status & PCIINT0;
    ahb_dma_event = ahb_status & (DMAREADEND|DMAWRITEEND);
        
    handled = 0;
    /* */
    if(ahb_event ){
                
        /* find out which device triger this interrupt */
        for (i = 0; i < PCIT_DEVICE_NR; i++) {
            if (!all_pcit_devs[i])
                continue;
            bus = all_pcit_devs[i]->bus;
            slot = all_pcit_devs[i]->slot; 
            hidev = all_pcit_devs[i]->hidev;
            hislot = hidev->slot;
            status = readl(hidev->cfg_base_addr + PCI_ISTATUS);
            if (! status)
                continue;

            PCIT_PRINT(PCIT_DEBUG, "hidev->cfg_base_addr =  %lx!,status is %lx", hidev->cfg_base_addr,status);
            
            status = readl(hidev->cfg_base_addr + PCI_ISTATUS);
            event  = 0;

            /* doorbell 15 is ignored here  */
            doorbell = (status & 0xf00000) >> 20;
            if((doorbell != 0) && (doorbell != 0xf)){
                PCIT_PRINT(PCIT_DEBUG, "Host received doorbell <%u>!", hislot);
                clear_int_hi_slot(hislot);
                disable_int_hi_slot(hislot);
                event = 1 << doorbell;
                handled++;
            }

            if(status & 0x101) 
            {
                handled++;
                PCIT_PRINT(PCIT_DEBUG, "Host received DMA!");
                clear_int_dma_hi_slot(hislot);
                disable_int_dma_hi_slot(hislot); /* added by c42025 */
                event |= (status & 0x100) ? 
                    (1 << HI_PCIT_EVENT_DMARD_0) : 0;
                event |= (status & 0x1) ? 
                    (1 << HI_PCIT_EVENT_DMAWR_0) : 0;
            }
            pevent.event_mask = event;

            /* travel all subscribers */
            local_irq_save(lockflag);
            for (i = 0; i < PCIT_SUBSCRIBER_NR; i++) {
                user = all_subscribers[i];
                if (! user) 
                    continue;
                if (event & user->event_mask_table[PCIT_MK_DEVIDX(bus, slot)]) {
                    user->notify(bus,slot,&pevent, user->data);            
                }
            }        
            local_irq_restore(lockflag);
            
            if((doorbell != 0) && (doorbell != 0xf)){
                enable_int_hi_slot(hislot);
            }

            if(status & 0x101) {
                enable_int_dma_hi_slot(hislot);
            }
        }
    }
    
    PCIT_PRINT(PCIT_DEBUG, "host pci dma interrupt itself: task = %p, ahb_status = %lx, ahb_event = %x", 
            PCIT_DMA_TASK_CUR, ahb_status, ahb_event);

    if(!ahb_dma_event)
    {   
        return IRQ_HANDLED;
        //return handled ? IRQ_HANDLED : IRQ_NONE;
    }
    clear_int_hi_dma();

    /* handle dma interrupt */

    if(list_empty(&busy_task_head))
    {
        printk("--- Core Dump-----\n");
        return IRQ_HANDLED;
    }

    local_irq_save(lockflag);
    
    cur_task = PCIT_DMA_TASK_CUR;    
    cur_task->state = 2;
    cur_task->finish(cur_task);
    list_del(&cur_task->list);
    list_add_tail(&cur_task->list, &free_task_head);
    g_taskNum--;
    
    if(!list_empty(&busy_task_head))
    {
        start_dma_task(PCIT_DMA_TASK_CUR);
    }
    local_irq_restore(lockflag);

    return IRQ_HANDLED;
}

irqreturn_t pci_device_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    struct pcit_subscriber_desc *user;
    struct pcit_event this_event = {0};
    struct pcit_dma_task *cur_task;
    unsigned long status, lockflag;
    unsigned int event,dma_event;
    int i;

    /* check interrupt status, doorbell or DMA ? */
    status = hi3511_bridge_ahb_readl(CPU_ISTATUS);  
    
    /* handle doorbell interrupt */
    event = status & (PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3);
    dma_event = status & (DMAREADEND|DMAWRITEEND);
    if (event) {
        PCIT_PRINT(PCIT_DEBUG, "device doorbell interrupt: status = %lx, event = %x",
            status, event);
        slave_int_door_clear();
        this_event.event_mask |= 1 << (event >> 16);
        
        local_irq_save(lockflag);
        for (i = 0; i < PCIT_SUBSCRIBER_NR; i++) {
            user = all_subscribers[i];
            if (! user)
                continue;

            if (user->event_mask_table[0] & this_event.event_mask) {
                user->notify(HI_PCIT_HOST_BUSID, HI_PCIT_HOST_SLOTID,
                    &this_event, user->data);
            }
        }
        local_irq_restore(lockflag);
        
        PCIT_PRINT(PCIT_DEBUG, "Handled doorbell!\n");
    }
    PCIT_PRINT(PCIT_DEBUG, "device dma interrupt: task = %p, status = %lx, event = %x", 
        PCIT_DMA_TASK_CUR, status, event);

    if(!dma_event)
    {
        return IRQ_HANDLED;
    }
    
    slave_int_dma_clear();
    
    /* handle dma interrupt */
    if(list_empty(&busy_task_head))
    {
        printk("--- Core Dump-----\n");
        return IRQ_HANDLED;
    }
    
    local_irq_save(lockflag);
    
    cur_task = PCIT_DMA_TASK_CUR;
    cur_task->state = 2;
    cur_task->finish(cur_task);
    list_del(&cur_task->list);
    list_add_tail(&cur_task->list, &free_task_head);

    g_taskNum--;
    
    if (!list_empty(&busy_task_head))
    {
        start_dma_task(PCIT_DMA_TASK_CUR);
    }
    local_irq_restore(lockflag);
    
    
    return IRQ_HANDLED;
}

static int __init hi3511_pcit_init(void)
{
    int ret = 0;
    int i;
    struct list_head *list;
    struct hi3511_dev *hidev;
    struct pcit_dev_desc *dev_desc;
    unsigned int bus, slot;

    struct pcit_subscriber subscriber;

    memset(all_pcit_devs, 0, sizeof(all_pcit_devs));
    memset(all_subscribers, 0, sizeof(all_subscribers));
    init_timer(&g_stDmaTimeOutTimer);
    
    /* register subscriber for application */        
    sprintf(subscriber.name, "application");
    subscriber.notify = default_user_notify;
    subscriber.data = NULL;
    app_subscriber_id  = pcit_subscriber_register(&subscriber);

    ret = misc_register(&hi3511_dma_miscdev);
    if (ret) {
		PCIT_PRINT(PCIT_ERR, "cannot register miscdev on minor=%d (err=%d)\n",
				MISC_DYNAMIC_MINOR, ret);
		return -1;
	}

	/* init dma task */
	for (i = PCIT_DMA_TASK_NR - 1; i >= 0; i--)
		list_add_tail(&task_array[i].list,&free_task_head);

    is_host = HI_PCIT_IS_HOST;
    PCIT_PRINT(PCIT_DEBUG, "is_host = %x\n", is_host);
    if (is_host) {     
        PCIT_PRINT(PCIT_DEBUG, "On host!\n");
        clear_int_hi_dma();        

        ret = request_irq(PCIT_IRQ_NR, pci_host_irq_handler,
            SA_SHIRQ,"hi35xx_pci_host",(void *)task_array);
        if (ret) {
            PCIT_PRINT(PCIT_ERR, "request irq failed!\n");
            goto reqirq_failed;
        } 
        list_for_each(list, &(hi3511_device_head.node)) {
            hidev = list_entry(list, struct hi3511_dev, node);
            bus = hidev->pdev->bus->number;
            slot = PCI_SLOT(hidev->pdev->devfn);           
            dev_desc = (struct pcit_dev_desc *)kmalloc(sizeof(struct pcit_dev_desc), GFP_KERNEL);            
            if (! dev_desc) {
                PCIT_PRINT(PCIT_ERR, "no memory for dev_desc!\n");
                goto no_mem;
            }
            memset(dev_desc, 0, sizeof(*dev_desc));          
            dev_desc->bus = bus;
            dev_desc->slot = slot;   
            dev_desc->hidev = hidev;
            dev_desc->is_host = hidev->is_host;
            init_waitqueue_head(&dev_desc->wqh);
            all_pcit_devs[PCIT_MK_DEVIDX(bus, slot)] = dev_desc; 

            clear_int_dma_hi_slot(hidev->slot);
            clear_int_hi_slot(hidev->slot);

            /* enable doorbell interrupt on every slot */
            enable_int_hi_slot(hidev->slot);

            /* enable DMA interrupt on every slot. We don't need the DMA interupte. */
            //enable_int_dma_hi_slot(hidev->slot);
            PCIT_PRINT(PCIT_DEBUG, "bus = %u, slot = %u, hislot = %d",
                bus, slot, hidev->slot);
            /* WARNING! only probe one device */
            //break;
        }       
        enable_int_hi_dma();
        PCIT_PRINT(PCIT_DEBUG, "hi3511 pcit init successfully!\n");
        return 0;
    }

    slave_int_door_clear();
    slave_int_dma_clear();

    PCIT_PRINT(PCIT_DEBUG, "On device!\n");
    ret = request_irq(PCIT_IRQ_NR, pci_device_irq_handler,
            SA_SHIRQ,"hi35xx_pci_device", (void *)task_array);
    if (ret) {
        PCIT_PRINT(PCIT_ERR, "request irq failed!\n");
        goto reqirq_failed;
    }   

    /* On device, the only target is HOST. */
    list_for_each(list, &(hi3511_device_head.node)) {
    hidev = list_entry(list, struct hi3511_dev, node);
    dev_desc = (struct pcit_dev_desc *)
        kmalloc(sizeof(struct pcit_dev_desc), GFP_KERNEL);            
    if (! dev_desc) {
        PCIT_PRINT(PCIT_ERR, "no memory for dev_desc!\n");
        goto no_mem;
    }
    memset(dev_desc, 0, sizeof(*dev_desc));          
    dev_desc->bus = HI_PCIT_HOST_BUSID;
    dev_desc->slot = HI_PCIT_HOST_SLOTID;
    dev_desc->hidev = hidev;
    init_waitqueue_head(&dev_desc->wqh);

    /* alloc share memory. */

    dev_desc->shm_size = hidev->shm_size;
    dev_desc->shm_phys_addr = hidev->shm_phys_addr;
    dev_desc->is_host = hidev->is_host;
    all_pcit_devs[0] = dev_desc;
    
    PCIT_PRINT(PCIT_DEBUG, "share meory: phys addr (%lx), size(%x)\n",
        dev_desc->shm_phys_addr, dev_desc->shm_size);
    }
//    hi3511_bridge_ahb_writel(PCIAHB_ADDR_PF, dev_desc->shm_phys_addr + 1);
   
    /* TODO: enable doorbell interrupt  */
    slave_int_dma_enable();
    slave_int_door_enable();
    return 0;
  
no_mem:
    for (i = 0; i < PCIT_DEVICE_NR; i++) {
        if (all_pcit_devs[i]) {
            kfree(all_pcit_devs[i]);
            all_pcit_devs[i] = NULL;
        }
    }
    free_irq(PCIT_IRQ_NR, (void *)task_array);
    
reqirq_failed:  
    misc_deregister(&hi3511_dma_miscdev);
    return -1;
}


static void __exit hi3511_pcit_exit(void)
{
    int i;
    
    free_irq(PCIT_IRQ_NR, (void *)task_array);
    
    for (i = 0; i < PCIT_SUBSCRIBER_NR; i++)
        pcit_subscriber_deregister(i);
    
    for (i = 0; i < PCIT_DEVICE_NR; i++){
        if (! all_pcit_devs[i])
            continue;
        kfree(all_pcit_devs[i]);
    }    
    misc_deregister(&hi3511_dma_miscdev);
}

module_init(hi3511_pcit_init);
module_exit(hi3511_pcit_exit);

MODULE_AUTHOR("chenkanglin");
MODULE_DESCRIPTION("Hisilicon Hi3511 PCI Driver Adpater");
MODULE_LICENSE("GPL");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
