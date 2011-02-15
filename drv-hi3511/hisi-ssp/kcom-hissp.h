#ifndef __KCOM_HI_SSP_H__
#define __KCOM_HI_SSP_H__

#define UUID_KCOM_HI_SSP_V1_0_0_0	"hi-ssp-1.0.0.0"

enum mod_select {MASTER,SLAVE}; 
enum slave_tx_disorenable {ENABLE,DISABLE}; 

struct kcom_hi_ssp {
	struct kcom_object kcom;
	void (*hi_ssp_enable)(void);
	void (*hi_ssp_disable)(void);
	void (*hi_ssp_mod_select)(enum mod_select mode);
	void (*hi_ssp_slave_tx_disorenable)(enum slave_tx_disorenable mode);
	int (*hi_ssp_set_frameform)(unsigned char framemode,unsigned char spo,
	                             unsigned char sph,unsigned char datawidth);
	int (*hi_ssp_set_serialclock)(unsigned char scr,unsigned char cpsdvsr);
	void (*hi_ssp_set_inturrupt)(unsigned char regvalue);
	void (*hi_ssp_interrupt_clear)(void);
	void (*hi_ssp_dmac_enable)(void);
	void (*hi_ssp_dmac_disable)(void);
	unsigned int (*hi_ssp_busystate_check)(void);
	int (*hi_ssp_readdata)(void);
	void (*hi_ssp_writedata)(unsigned short sdata);
	int (*hi_ssp_dmac_init)(void * prx_dmac_hook,void * ptx_dmac_hook);
	int (*hi_ssp_dmac_transfer)(unsigned int phy_rxbufaddr,unsigned int phy_txbufaddr,
	                             unsigned int transfersize);
	void (*hi_ssp_dmac_exit)(void);
	
};

#ifndef __KCOM_HI_SSP_INTER__

extern struct kcom_hi_ssp *p_kcom_hi_ssp;

#define DECLARE_KCOM_HI_SSP() struct kcom_hi_ssp *p_kcom_hi_ssp
#define KCOM_HI_SSP_INIT()	KCOM_GET_INSTANCE(UUID_KCOM_HI_SSP_V1_0_0_0, p_kcom_hi_ssp)
#define KCOM_HI_SSP_EXIT()	KCOM_PUT_INSTANCE(p_kcom_hi_ssp)

#define hi_ssp_enable	p_kcom_hi_ssp->hi_ssp_enable
#define hi_ssp_disable	p_kcom_hi_ssp->hi_ssp_disable
#define hi_ssp_mod_select        p_kcom_hi_ssp->hi_ssp_mod_select
#define hi_ssp_slave_tx_disorenable  p_kcom_hi_ssp->hi_ssp_slave_tx_disorenable
#define hi_ssp_set_frameform	p_kcom_hi_ssp->hi_ssp_set_frameform
#define hi_ssp_set_serialclock		p_kcom_hi_ssp->hi_ssp_set_serialclock
#define hi_ssp_set_inturrupt	p_kcom_hi_ssp->hi_ssp_set_inturrupt	
#define hi_ssp_interrupt_clear	p_kcom_hi_ssp->hi_ssp_interrupt_clear
#define hi_ssp_dmac_enable		p_kcom_hi_ssp->hi_ssp_dmac_enable	
#define hi_ssp_dmac_disable		p_kcom_hi_ssp->hi_ssp_dmac_disable	
#define hi_ssp_busystate_check		p_kcom_hi_ssp->hi_ssp_busystate_check
#define hi_ssp_readdata		p_kcom_hi_ssp->hi_ssp_readdata
#define hi_ssp_writedata		p_kcom_hi_ssp->hi_ssp_writedata
#define hi_ssp_dmac_init		p_kcom_hi_ssp->hi_ssp_dmac_init
#define hi_ssp_dmac_transfer		p_kcom_hi_ssp->hi_ssp_dmac_transfer
#define hi_ssp_dmac_exit		p_kcom_hi_ssp->hi_ssp_dmac_exit

#endif

#endif

