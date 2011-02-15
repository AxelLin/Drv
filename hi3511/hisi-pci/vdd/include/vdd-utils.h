
#ifndef __VDD_UTILS_H__
#define __VDD_UTILS_H__
#include "../../include/pci_vdd.h"
int vdd_get_info(struct pci_vdd_info *pvi);
int vdd_is_host(void);
int vdd_get_target_num(void);
int vdd_get_myid(void);

#endif

