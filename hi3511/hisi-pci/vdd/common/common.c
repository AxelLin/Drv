
#include <linux/config.h>
#include <linux/delay.h>
#include <vdd-utils.h>
#include "../../include/pci_vdd.h"
int vdd_get_info(struct pci_vdd_info *pvi)
{
	int wait_cycles = 100, i;
	/* wait for pci info */
	for(i=0; i<wait_cycles && pci_vdd_getinfo(pvi); i++) {
		msleep(10);
	}
	if(i==wait_cycles)
		return -1;
	
	return 0;
}

int vdd_is_host(void)
{
	static int is_host = -1;

	if(!(is_host < 0)) {
		return is_host;
	} else {
		struct pci_vdd_info pvi;

		if(vdd_get_info(&pvi))
			return -1;
		is_host = pvi.id==0 ? 1:0;
		//printk("pvi.id %ld\n",pvi.id);
	}
	//printk("is_host %d\n",is_host);
	
	return is_host;
}

int vdd_get_myid(void)
{
	static int myid = -1;

	if(!(myid <0)) {
		return myid;
	} else {
		struct pci_vdd_info pvi;

		if(vdd_get_info(&pvi))
			return -1;
		myid = pvi.id;
	}

	return myid;
}

int vdd_get_target_num(void)
{
	static int target_nr = 0;
	struct pci_vdd_info pvi;
	if(target_nr)
		return target_nr;

	if(!vdd_is_host()) {
		return 0;
	} else {
		unsigned long i;
		if(vdd_get_info(&pvi))
			return -1;
		/* get slave nr */
		i = pvi.slave_map;
//		printk("slave_map %d\n",pvi.slave_map);
		while(i) {
			if(i&0x01)
				target_nr++;
			i = i>>1;
		}
	}
//	printk("target_nr %d\n",target_nr);
	return target_nr;
}


