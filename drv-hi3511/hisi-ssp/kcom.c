#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/kcom.h>
#include "hi_ssp.h"

void kcom_hi_ssp_enable(void)
{
    hi_ssp_enable();
}

void kcom_hi_ssp_disable(void)
{
    hi_ssp_disable();
}

void kcom_hi_ssp_mod_select(enum mod_select mode)
{
    hi_ssp_mod_select(mode);
}

void kcom_hi_ssp_slave_tx_disorenable(enum slave_tx_disorenable mode)
{
    hi_ssp_slave_tx_disorenable(mode);    
}

	
int kcom_hi_ssp_set_frameform(unsigned char framemode,unsigned char spo,
	                             unsigned char sph,unsigned char datawidth)
{
    return hi_ssp_set_frameform(framemode, spo, sph, datawidth);
}
	                   
int kcom_hi_ssp_set_serialclock(unsigned char scr,unsigned char cpsdvsr)
{
    return hi_ssp_set_serialclock(scr,cpsdvsr); 	                        
}
	                    
void kcom_hi_ssp_set_inturrupt(unsigned char regvalue)
{
    hi_ssp_set_inturrupt(regvalue);    
}
	                                     	                    
void kcom_hi_ssp_interrupt_clear(void)
{
    return hi_ssp_interrupt_clear();
}
	                           
void kcom_hi_ssp_dmac_enable(void)
{
    return hi_ssp_dmac_enable();
                   
}
                
void kcom_hi_ssp_dmac_disable(void)
{
    hi_ssp_dmac_disable(); 

}

unsigned int kcom_hi_ssp_busystate_check(void)
{
    return hi_ssp_busystate_check();
}

int kcom_hi_ssp_readdata(void)
{
    return hi_ssp_readdata();
}

void kcom_hi_ssp_writedata(unsigned short sdata)
{
    hi_ssp_writedata(sdata);
}

int kcom_hi_ssp_dmac_init(void * prx_dmac_hook,void * ptx_dmac_hook)
{
    return hi_ssp_dmac_init(prx_dmac_hook,ptx_dmac_hook);
}

int kcom_hi_ssp_dmac_transfer(unsigned int phy_rxbufaddr,unsigned int phy_txbufaddr,
	                             unsigned int transfersize)
{
    return hi_ssp_dmac_transfer(phy_rxbufaddr,phy_txbufaddr,transfersize);
}	                            


void kcom_hi_ssp_dmac_exit(void)
{
    hi_ssp_dmac_exit();
}
	                             
struct kcom_hi_ssp kcom_hissp = {
	.kcom = KCOM_OBJ_INIT(UUID_KCOM_HI_SSP_V1_0_0_0, UUID_KCOM_HI_SSP_V1_0_0_0, NULL, THIS_MODULE, KCOM_TYPE_OBJECT, NULL),
    .hi_ssp_enable = kcom_hi_ssp_enable,
    .hi_ssp_disable = kcom_hi_ssp_disable,
    .hi_ssp_mod_select = kcom_hi_ssp_mod_select,
    .hi_ssp_slave_tx_disorenable = kcom_hi_ssp_slave_tx_disorenable,
    .hi_ssp_set_frameform = kcom_hi_ssp_set_frameform,
    .hi_ssp_set_serialclock = kcom_hi_ssp_set_serialclock,
    .hi_ssp_set_inturrupt = kcom_hi_ssp_set_inturrupt,
    .hi_ssp_interrupt_clear  = kcom_hi_ssp_interrupt_clear,
    .hi_ssp_dmac_enable = kcom_hi_ssp_dmac_enable,
    .hi_ssp_dmac_disable = kcom_hi_ssp_dmac_disable,
    .hi_ssp_busystate_check = kcom_hi_ssp_busystate_check,
    .hi_ssp_readdata = kcom_hi_ssp_readdata,
    .hi_ssp_writedata = kcom_hi_ssp_writedata,
    .hi_ssp_dmac_init = kcom_hi_ssp_dmac_init,
    .hi_ssp_dmac_transfer = kcom_hi_ssp_dmac_transfer,
    .hi_ssp_dmac_exit = kcom_hi_ssp_dmac_exit
};

int kcom_hi_ssp_register(void)
{
	return kcom_register(&kcom_hissp.kcom);
}

void kcom_hi_ssp_unregister(void)
{
	kcom_unregister(&kcom_hissp.kcom);
}

               
