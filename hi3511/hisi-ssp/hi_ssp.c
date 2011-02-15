/*  extdrv/interface/ssp/hi_ssp.c
 *
 * Copyright (c) 2006 Hisilicon Co., Ltd. 
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
 * along with this program; 
 *
 * History: 
 *      21-April-2006 create this file
 */
 
#include <linux/module.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/hardware.h>
#include <linux/kcom.h>
#include <kcom/hidmac.h>
#include "hi_ssp.h"
	
	
#define  ssp_writew(addr,value)      ((*(volatile unsigned int *)(addr)) = (value))
#define  ssp_readw(addr,ret)           (ret =(*(volatile unsigned int *)(addr)))

#define SSP_BASE	0x101F4000

/* SSP register definition .*/
#define SSP_CR0              IO_ADDRESS(SSP_BASE + 0x00)
#define SSP_CR1              IO_ADDRESS(SSP_BASE + 0x04)
#define SSP_DR               IO_ADDRESS(SSP_BASE + 0x08)
#define SSP_SR               IO_ADDRESS(SSP_BASE + 0x0C)
#define SSP_CPSR             IO_ADDRESS(SSP_BASE + 0x10)
#define SSP_IMSC             IO_ADDRESS(SSP_BASE + 0x14)
#define SSP_RIS              IO_ADDRESS(SSP_BASE + 0x18)
#define SSP_MIS              IO_ADDRESS(SSP_BASE + 0x1C)
#define SSP_ICR              IO_ADDRESS(SSP_BASE + 0x20)
#define SSP_DMACR            IO_ADDRESS(SSP_BASE + 0x24)
#define SCTL_SSP_DEMUX       IO_ADDRESS(0x101e0020) 

int ssp_dmac_rx_ch = -1 ,ssp_dmac_tx_ch = -1;
/*
 * enable SSP routine.
 *
 */

void hi_ssp_enable(void)
{
    int ret = 0;
    ssp_readw(SSP_CR1,ret);
    ret = (ret & 0xFFFD) | 0x2;
    ssp_writew(SSP_CR1,ret);
}

/*
 * disable SSP routine.
 *
 */

void hi_ssp_disable(void)
{
    int ret = 0;
    ssp_readw(SSP_CR1,ret);
    ret = ret & 0xFFFD;
    ssp_writew(SSP_CR1,ret);
}	

/*
 * select the ssp mode: master or slave.
 * 
 *
 */
void hi_ssp_mod_select(enum mod_select mode)
{
    int ret = 0;
    ssp_readw(SSP_CR1,ret);
    
    //if ssp is busy , selecting will fail;
    if( 1== (ret & 0x2) )
    {
        printk("ssp is  busy,you can't change mode of ssp!\n"); 
        return;
    }
    
    if( (MASTER!=mode) && (SLAVE!=mode) )
    {
        printk("invalid ssp mode!\n");	
        return ;
    }
    ret &= (~0x4);  
    ret |= (mode << 2);
    ssp_writew(SSP_CR1,ret);
}	

void hi_ssp_slave_tx_disorenable(enum slave_tx_disorenable mode)
{
    int ret = 0;
    ssp_readw(SSP_CR1,ret);
    
    //if ssp is under master mod ,operation will fail;
    if( 1== (ret & 0x2) )
    {
        printk("ssp is  busy,you can't change mode of ssp!\n"); 
        return;
    }
    
    if( (ENABLE!=mode) && (DISABLE!=mode) )
    {
        printk("invalid input!\n");	
        return ;
    }
    ret &= (~0x8);   
    ret |= (mode <<3);
    ssp_writew(SSP_CR1,ret);
}
/*
 * set SSP frame form routine.
 *
 * @param framemode: frame form
 * 00: Motorola SPI frame form. 
 * when set the mode,need set SSPCLKOUT phase and SSPCLKOUT voltage level. 
 * 01: TI synchronous serial frame form
 * 10: National Microwire frame form
 * 11: reserved
 * @param sphvalue: SSPCLKOUT phase (0/1)
 * @param sp0: SSPCLKOUT voltage level (0/1)
 * @param datavalue: data bit 
 * 0000: reserved    0001: reserved    0010: reserved    0011: 4bit data
 * 0100: 5bit data   0101: 6bit data   0110:7bit data    0111: 8bit data
 * 1000: 9bit data   1001: 10bit data  1010:11bit data   1011: 12bit data
 * 1100: 13bit data  1101: 14bit data  1110:15bit data   1111: 16bit data
 * 
 * @return value: 0--success; -1--error.
 *
 */

int hi_ssp_set_frameform(unsigned char framemode,unsigned char spo,unsigned char sph,unsigned char datawidth)
{
    int ret = 0;
    ssp_readw(SSP_CR0,ret);
    if(framemode > 3) 
    {
        printk("set frame parameter err.\n");
        return -1;
    }
    ret = (ret & 0xFFCF) | (framemode << 4);
    if((ret & 0x30) == 0)
    {
        if(spo > 1)
        {
            printk("set spo parameter err.\n");
            return -1;
        }
        if(sph > 1)
        {
            printk("set sph parameter err.\n");
            return -1;
        }
        ret = (ret & 0xFF3F) | (sph << 7) | (spo << 6);
    }
    if((datawidth > 16) || (datawidth < 4))
    {
        printk("set datawidth parameter err.\n");
        return -1;
    }
    ret = (ret & 0xFFF0) | (datawidth -1);
    ssp_writew(SSP_CR0,ret);
    return 0;
}

/*
 * set SSP serial clock rate routine.
 *
 * @param scr: scr value.(0-255,usually it is 0)
 * @param cpsdvsr: Clock prescale divisor.(2-254 even) 
 *
 * @return value: 0--success; -1--error.
 *  
 */

int hi_ssp_set_serialclock(unsigned char scr,unsigned char cpsdvsr)
{
    int ret = 0;
    ssp_readw(SSP_CR0,ret);
    ret = (ret & 0xFF) | (scr << 8);
    ssp_writew(SSP_CR0,ret);    
    if((cpsdvsr & 0x1))
    {
        printk("set cpsdvsr parameter err.\n");
        return -1;        
    }
    ssp_writew(SSP_CPSR,cpsdvsr);
    return 0;
}

/*
 * set SSP interrupt routine.
 *
 * @param regvalue: SSP_IMSC register value.(0-255,usually it is 0)
 *
 */
void hi_ssp_set_inturrupt(unsigned char regvalue)
{
    
    ssp_writew(SSP_IMSC,(regvalue&0x0f));
}

/*
 * clear SSP interrupt routine.
 *
 */

void hi_ssp_interrupt_clear(void)
{
    ssp_writew(SSP_ICR,0x3);
}

/*
 * enable SSP dma mode routine.
 *
 */

void hi_ssp_dmac_enable(void)
{
    ssp_writew(SSP_DMACR,0x3);
}

/*
 * disable SSP dma mode routine.
 *
 */

void hi_ssp_dmac_disable(void)
{
    ssp_writew(SSP_DMACR,0);
}



/*
 * check SSP busy state routine.
 *
 * @return value: 0--free; 1--busy.
 *
 */

unsigned int hi_ssp_busystate_check(void)
{
    int ret = 0;
    ssp_readw(SSP_SR,ret);
    if((ret & 0x10) != 0x10)
        return 0;
    else 
        return 1;
}

/*  
 *  write SSP_DR register rountine.
 *
 *  @param  sdata: data of SSP_DR register 
 *   
 */
 
 
void hi_ssp_writedata(unsigned short sdata)
{
    ssp_writew(SSP_DR,sdata);
}	

/*  
 *  read SSP_DR register rountine.
 *  
 *  @return value: data from SSP_DR register readed
 * 
 */
 
int hi_ssp_readdata(void)
{
    int ret = 0;
    ssp_readw(SSP_DR,ret);
    return ret;
}



/*
 * check SSP busy state routine.
 * @param prx_dmac_hook : dmac rx interrupt function pointer
 * @param ptx_dmac_hook : dmac tx interrupt function pointer
 *
 * @return value: 0--success; -1--error.
 *
 */
 
int hi_ssp_dmac_init(void * prx_dmac_hook,void * ptx_dmac_hook)
{
	ssp_dmac_rx_ch = dmac_channel_allocate(prx_dmac_hook);  
	if(ssp_dmac_rx_ch < 0)
	{
	    printk("SSP no available rx channel can allocate.\n");
	    return -1;  
	}
	ssp_dmac_tx_ch = dmac_channel_allocate(ptx_dmac_hook);
	if(ssp_dmac_tx_ch < 0)
	{
	    printk("SSP no available tx channel can allocate.\n");
	    dmac_channel_free(ssp_dmac_rx_ch);
	    ssp_dmac_rx_ch = -1;
	    return -1;  
	}
	return 0;
}


/*
 * SSP dma mode data transfer routine.
 * @param phy_rxbufaddr : rxbuf physical address
 * @param phy_txbufaddr : txbuf physical address
 * @param transfersize : transfer data size
 *
 * @return value: 0--success; -1--error.
 *
 */
int hi_ssp_dmac_transfer(unsigned int phy_rxbufaddr,unsigned int phy_txbufaddr,unsigned int transfersize)
{
	int ret=0;

	ret=dmac_start_m2p(ssp_dmac_rx_ch,phy_rxbufaddr, DMAC_SSP_RX_REQ, transfersize,0);
	if(ret != 0)
	{
		return(ret);
    }
	ret=dmac_start_m2p(ssp_dmac_tx_ch,phy_txbufaddr, DMAC_SSP_TX_REQ, transfersize,0);
	if(ret != 0)
	{
		return(ret);
    }
	dmac_channelstart(ssp_dmac_rx_ch);
	dmac_channelstart(ssp_dmac_tx_ch);
		
	return 0;
}


/*
 * SSP dma mode exit
 *
 * @return value: 0 is ok
 *
 */
void hi_ssp_dmac_exit(void)
{
	if (0 <= ssp_dmac_rx_ch)
	{
		dmac_channel_free(ssp_dmac_rx_ch);
		ssp_dmac_rx_ch = -1;
	}
	if (0 <= ssp_dmac_tx_ch)
	{
		dmac_channel_free(ssp_dmac_tx_ch);
		ssp_dmac_tx_ch = -1;
	}
}
	

static unsigned int  sspinitialized =0;
DECLARE_KCOM_HI_DMAC();
/*
 * initializes SSP interface routine.
 *
 * @return value:0--success.
 *
 */
static int __init hi_ssp_init(void)
{

    int ret;
    ret = KCOM_HI_DMAC_INIT();
    if(ret)
    {
        printk("DMAC module is not load.\n");
        return -1;
    }
    ret = kcom_hi_ssp_register();

    if(0 != ret)
    {
        KCOM_HI_DMAC_EXIT();
        printk("SSP init is error.\n");
        return -1;
    }
    //switch the pin from gpio to ssp
 
    ssp_readw(SCTL_SSP_DEMUX,ret);
    ret &= (~0x1000);
    ret |= 0x400; 
    ssp_writew(SCTL_SSP_DEMUX,ret);

    if(sspinitialized == 0)
    {
        sspinitialized = 1;
        printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
        return 0;
    }
    else
    {
        printk("SSP has been initialized.\n");
        return 0;
    }
}

static void __exit hi_ssp_exit(void)
{
    sspinitialized =0;
    hi_ssp_dmac_exit();
    kcom_hi_ssp_unregister();
    KCOM_HI_DMAC_EXIT();
}

module_init(hi_ssp_init);
module_exit(hi_ssp_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);





