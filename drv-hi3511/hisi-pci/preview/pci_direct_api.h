#ifndef __PCI_TEST_API_H_
#define __PCI_TEST_API_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

//extern unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
//extern unsigned long simple_strtoul(const char *,char **,unsigned int);
extern unsigned long ioctl(int,int,unsigned long);
extern size_t strlen(const char * s);

struct PCI_TRANS
{
	unsigned int   target;
	unsigned long *dest;
	unsigned long *source;
	unsigned int   len;
	
};

int hi_pci_init(void);
int hi_pci_close(void);
int hi_pci_dma_write(struct PCI_TRANS *trans_descriptor);
int hi_pci_dma_read(struct PCI_TRANS *trans_descriptor);
int hi_pci_mmcpy(struct PCI_TRANS *trans_descriptor);
int hi_pci_check(int slot);
int hi_pci_slave_dma_write(struct PCI_TRANS *trans_descriptor);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__PCI_TEST_API_H*/


