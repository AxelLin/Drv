#include <linux/module.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/himedia.h>
#include "hi_mcc_usrdev.h"
#include "hisi_pci_proto.h"

int hios_mcc_close(hios_mcc_handle_t *handle)
{
	if(handle){
		pci_vdd_close((void *)handle->handle);
		kfree(handle);
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(hios_mcc_close);

hios_mcc_handle_t *hios_mcc_open(struct hi_mcc_handle_attr *attr)
{
	hios_mcc_handle_t *handle = NULL;
	unsigned long data;
	data = (unsigned long) pci_vdd_open(attr->target_id, attr->port, attr->priority);
	if(data){
		handle = kmalloc(sizeof(hios_mcc_handle_t), GFP_KERNEL);
		handle->handle = data; 
	}
	return handle;
}
EXPORT_SYMBOL(hios_mcc_open);

int hios_mcc_sendto(hios_mcc_handle_t *handle, const void *buf, unsigned int len)
{
	if(handle)
		return pci_vdd_sendto((void *)handle->handle, buf , len, HISI_PCI_MEM_DATA); 
	else
		return -1;
}

EXPORT_SYMBOL(hios_mcc_sendto);

int hios_mcc_getopt(hios_mcc_handle_t *handle, hios_mcc_handle_opt_t *opt)
{
	pci_vdd_opt_t vdd_opt;
	pci_vdd_getopt((void *)handle->handle, &vdd_opt);
	opt->recvfrom_notify = vdd_opt.recvfrom_notify; 
	opt->data = vdd_opt.data;
	return 0;
}
EXPORT_SYMBOL(hios_mcc_getopt);

int hios_mcc_setopt(hios_mcc_handle_t *handle, const hios_mcc_handle_opt_t *opt)
{
	pci_vdd_opt_t vdd_opt;
	if(opt){
		vdd_opt.recvfrom_notify = opt->recvfrom_notify;
		vdd_opt.data = opt->data;
		return pci_vdd_setopt((void *)handle->handle, &vdd_opt);
	}
	return -1;
}
EXPORT_SYMBOL(hios_mcc_setopt);

int hios_mcc_getlocalid(void)
{
	return pci_vdd_localid();
}
EXPORT_SYMBOL(hios_mcc_getlocalid);

int hios_mcc_getremoteids(int ids[])
{
	return pci_vdd_remoteids(&ids[0]);
}
EXPORT_SYMBOL(hios_mcc_getremoteids);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("chanjinn");

