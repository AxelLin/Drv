#include <linux/module.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/miscdevice.h>
#include "hi_mcc_usrdev.h"

extern void *vmalloc(unsigned long size);
extern void vfree(void *addr);

/* mem_list using for handle mem clean */
struct hios_mem_list
{
        struct list_head head;
	void *data;
	unsigned int data_len;
};

/* handle_list using for relase func clean handle */
static struct semaphore handle_sem; 

static int hios_mcc_notifier_recv(void *pci_handle, void *buf, unsigned int length)
{
	hios_mcc_handle_t mcc_handle;
	hios_mcc_handle_t *_handle;
	hios_mcc_handle_opt_t opt;

	struct hios_mem_list *mem;
        void *data;
	mcc_handle.handle = (unsigned long) pci_handle;

	hios_mcc_getopt(&mcc_handle, &opt);

	_handle = (hios_mcc_handle_t *)opt.data;

        data = kmalloc(length + sizeof(struct hios_mem_list), GFP_ATOMIC);
	if(!data){
		hi_mcc_trace(2, "nortifier error %d\n", 0);
		return -1;
	}
	hi_mcc_trace(1, "nortifier_recv addr 0x%8lx len %d\n", (unsigned long)buf, length);
	
	mem = (struct hios_mem_list *)data;
	mem->data = data + sizeof(struct hios_mem_list);

        memcpy(mem->data, buf, length);
#if (HI_MCC_DBG_LEVEL > 2)
	{
		int i;
		printk("buf 0x%8lx ", (unsigned long)buf);
		printk("\n");
		for(i = 0; i < 8; i++){
			printk("0x%x ", *(char *)(buf + i));
		}
		printk("\n");
	}	
#endif
	mem->data_len = length;
	list_add_tail(&mem->head, &_handle->mem_list);
	wake_up_interruptible(&_handle->wait);
        return 0;
}

static void usrdev_setopt_recv(hios_mcc_handle_t *handle)
{
	hios_mcc_handle_opt_t opt;
	opt.recvfrom_notify = &hios_mcc_notifier_recv;
	opt.data = (unsigned long) handle;
	hios_mcc_setopt(handle, &opt);	
}

static void usrdev_setopt_null(hios_mcc_handle_t *handle)
{
	hios_mcc_handle_opt_t opt;
	opt.recvfrom_notify = NULL;
	opt.data = 0;
	hios_mcc_setopt(handle, &opt);	
}

static int hi_mcc_userdev_open(struct inode *inode, struct file *file)
{
	file->private_data = 0;
	init_MUTEX(&handle_sem);
	return 0;
}

static void _del_mem_list(hios_mcc_handle_t *handle)
{
	struct list_head *entry , *tmp;
	struct hios_mem_list *mem;
        /* if mem list empty means no data comming*/
	if (!list_empty(&handle->mem_list)){
		list_for_each_safe(entry,tmp, &handle->mem_list){
			mem = list_entry(entry, struct hios_mem_list, head);
			list_del(&mem->head);
			kfree(mem);
			hi_mcc_trace(2, "handle 0x%8lx not empty\n", (unsigned long)handle);
               	}
	}
}

static int hi_mcc_userdev_release(struct inode *inode, struct file *file)
{
        hios_mcc_handle_t *handle = (hios_mcc_handle_t *) file->private_data;
	handle = (hios_mcc_handle_t *)file->private_data;
	hi_mcc_trace(1, "close 0x%8lx\n", (unsigned long)handle);
		
	usrdev_setopt_null(handle);

	down(&handle_sem);

        /* if mem list empty means no data comming*/
	if (!list_empty(&handle->mem_list)){
		_del_mem_list(handle);
	}
	hios_mcc_close(handle);
	file->private_data = 0;

	up(&handle_sem);
	hi_mcc_trace(2, "release success 0x%d\n", 0);
	return 0;
}

static int hi_mcc_userdev_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
        hios_mcc_handle_t *handle = (hios_mcc_handle_t *) file->private_data;
	struct list_head *entry, *tmp;
	unsigned int len = 0;

	hi_mcc_trace(2, "read  empty %d handle 0x%8lx\n", list_empty(&handle->mem_list), (unsigned long) handle);
        /* if mem list empty means no data comming*/
        if (!list_empty(&handle->mem_list)){
		list_for_each_safe(entry, tmp, &handle->mem_list){
			struct hios_mem_list *mem = list_entry(entry, struct hios_mem_list, head);
			len = mem->data_len;
			if (len > count)
				len = count;
			copy_to_user(buf, mem->data, len);
			list_del(&mem->head);
			hi_mcc_trace(2, "read %d\n", len);
			kfree(mem);
			break;
		}
	}

	hi_mcc_trace(1, "read success %d\n", len);
        return len;
}

static int hi_mcc_userdev_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
        int ret=0;
	char *kbuf;
	hios_mcc_handle_t *handle = (hios_mcc_handle_t *)file->private_data;
	kbuf = vmalloc(count);

	copy_from_user(kbuf,buf,count);

	ret = hios_mcc_sendto(handle, kbuf, count);
	if(ret < 0){
		hi_mcc_trace(2, "sendto error! %d\n",0);
	}
	vfree(kbuf);
	hi_mcc_trace(1, "send success %d\n", ret);
        return ret;
}

static int hi_mcc_userdev_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	hios_mcc_handle_t *handle;
	struct hi_mcc_handle_attr attr;

	if(_IOC_TYPE(cmd) == 'M'){
		switch(_IOC_NR(cmd)){
			case _IOC_NR(HI_MCC_IOC_CONNECT):
				down(&handle_sem);
				if(copy_from_user((void *)&attr, (void *)arg, sizeof(struct hi_mcc_handle_attr))){
					hi_mcc_trace(2,"can not get the parameter when connect %d\n",0);
					up(&handle_sem);
					return -1;
				}
				hi_mcc_trace(1, "open %d\n", 0);
				handle = hios_mcc_open(&attr);
				if(handle){
					INIT_LIST_HEAD(&handle->mem_list);
					init_waitqueue_head(&handle->wait);
					file->private_data = (void *)handle;
					usrdev_setopt_recv(handle);
					hi_mcc_trace(1, "open success 0x%8lx\n", (unsigned long)handle);
				}
				up(&handle_sem);
				break;

			case _IOC_NR(HI_MCC_IOC_DISCONNECT):
			        break;
			default:
				hi_mcc_trace(2, "warning not defined cmd %d\n",0);
				break;
		}
	}
	return 0;
}

static unsigned int hi_mcc_userdev_poll(struct file *file, struct poll_table_struct *table)
{
	hios_mcc_handle_t *handle = (hios_mcc_handle_t *) file->private_data;

	poll_wait(file, &handle->wait, table);

	/* if mem list empty means no data comming*/
	if (!list_empty(&handle->mem_list)){
		hi_mcc_trace(2, "poll not empty handle 0x%8lx\n",(unsigned long)handle);
		return POLLIN | POLLRDNORM;
	}
	
	return 0;
}

static struct file_operations mcc_userdev_fops = {
        .owner  = THIS_MODULE,
        .open   = hi_mcc_userdev_open,
        .release= hi_mcc_userdev_release,
        .ioctl  = hi_mcc_userdev_ioctl,
	.write = hi_mcc_userdev_write,
	.read = hi_mcc_userdev_read,
        .poll = hi_mcc_userdev_poll,
};

static struct miscdevice hi_mcc_userdev = {
        .minor  = MISC_DYNAMIC_MINOR,
        .fops   = &mcc_userdev_fops,
        .name   = "mcc_userdev"
};


int __init hi_mmc_userdev_init(void)
{
        misc_register(&hi_mcc_userdev);
        return 0;
}

void __exit hi_mmc_userdev_exit(void)
{
        misc_deregister(&hi_mcc_userdev);
}

module_init(hi_mmc_userdev_init);
module_exit(hi_mmc_userdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chanjinn");

