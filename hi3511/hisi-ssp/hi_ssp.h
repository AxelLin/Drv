/*
 * extdrv/include/hi_ssp.h for Linux .
 *
 * History: 
 *      2006-4-11 create this file
 */ 
 
#ifndef __HI_SSP_H__
#define __HI_SSP_H__

#define __KCOM_HI_SSP_INTER__
#include "kcom-hissp.h"    

#define OSDRV_MODULE_VERSION_STRING "HISI_SSP-MDC030002 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:53:13"

	void hi_ssp_enable(void);
	void hi_ssp_disable(void);
	void hi_ssp_slave_tx_disorenable(enum slave_tx_disorenable mode);
	void hi_ssp_mod_select(enum mod_select mode);
	int hi_ssp_set_frameform(unsigned char,unsigned char,unsigned char,unsigned char);
	int hi_ssp_set_serialclock(unsigned char,unsigned char);
	void hi_ssp_set_inturrupt(unsigned char);
	void hi_ssp_interrupt_clear(void);
	void hi_ssp_dmac_enable(void);
	void hi_ssp_dmac_disable(void);
	unsigned int hi_ssp_busystate_check(void);
	int hi_ssp_readdata(void);
	void hi_ssp_writedata(unsigned short);
	int hi_ssp_dmac_init(void *,void *);
	int hi_ssp_dmac_transfer(unsigned int,unsigned int,unsigned int);
	void hi_ssp_dmac_exit(void);
	
    int kcom_hi_ssp_register(void);
    void kcom_hi_ssp_unregister(void);	
#endif 

