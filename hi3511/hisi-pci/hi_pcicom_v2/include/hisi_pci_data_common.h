#ifndef _HISI_DATA_COMMON_H_ 
#define _HISI_DATA_COMMON_H_
#include "hisi_pci_hw.h"
#include "hisi_pci_proto.h"

#define HISI_PCI_SHARED_RECMEM_SIZE	( 125 * 1024 )
#define HISI_PCI_SHARED_SENDMEM_SIZE	( 125 * 1024 )

int hisi_shared_mem_init(struct hisi_pcicom_dev *dev);
int hisi_is_priority_buffer_empty(struct hisi_pcicom_dev *dev);
int hisi_is_normal_buffer_empty(struct hisi_pcicom_dev *dev);
void *hisi_get_normal_head(struct hisi_pcicom_dev *dev); 
void *hisi_get_priority_head(struct hisi_pcicom_dev *dev);
void hisi_normal_data_recieved(struct hisi_pcicom_dev *dev, void *data, unsigned int len);
void hisi_priority_data_recieved(struct hisi_pcicom_dev *dev, void *data, unsigned int len);

int hisi_send_priority_data(struct hisi_pcicom_dev *dev, const void *buf, unsigned int len, 
			struct hisi_pci_transfer_head *head);

int hisi_send_normal_data(struct hisi_pcicom_dev *dev, const void *buf, unsigned int len, 
			struct hisi_pci_transfer_head *head);
#endif

