/*
 *  linux/drivers/mmc/pxa.c - PXA MMCI driver
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This hardware is really sick:
 *   - No way to clear interrupts.
 *   - Have to turn off the clock whenever we touch the device.
 *   - Doesn't tell you how many data blocks were transferred.
 *  Yuck!
 *
 *	1 and 3 byte data transfers not supported
 *	max block length up to 1023
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/scatterlist.h>
#include <asm/sizes.h>
#include "w_debug.h"
#include <linux/workqueue.h>


//#include <asm/arch/mmc.h>

#include "himci.h"
#include <linux/kcom.h>
#include <kcom/hidmac.h>

extern int  read_write_blkdata(int, unsigned long,
                        unsigned long,
                        unsigned char  *,
  	                    unsigned char  *,
		                unsigned char  *,
		                void  (*)(unsigned long),
		                int ,
		                int );

extern int form_send_cmd(int,int,unsigned long,unsigned char *,int);
extern void unregister_dma_hook(void);
extern void rca_consistent(int,int);
static void himci_dma_irq_hook(unsigned long,unsigned long);
extern void register_dma_hook(void *);
extern int set_clk_frequency(int,int,unsigned char *);
extern int sd_set_mode(int,int);
extern void sd_register_detect_hook(int, void *);
extern int set_ip_parameters(int ,unsigned long *);
extern void unregister_dma_hook(void);

extern int sd_mmc_check_card(int);
extern int sd_mmc_check_ro(int);
#define DBG(x...)


#define NR_SG	1
#define DMAC_MMC_TX_REQ 13
#define DMAC_MMC_RX_REQ 12
#define DMAC_CHANNEL_INVALID 101

struct himci_host {
	struct mmc_host		*mmc;
	spinlock_t		lock;
	struct mmc_request	*mrq;
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	dma_addr_t		sg_dma;
	struct hi_dma_desc	*sg_cpu;
	int card_insert;
	unsigned int dma_num;
	unsigned int	dma_len;
      int  dma_count;
	unsigned int	dma_dir;
	struct workqueue_struct *himci_queue;
};

struct himci_host *himci_host_g;

//static struct kcom_object *kcom;
//static struct kcom_hi_dmac *kcom_hidmac;
DECLARE_KCOM_HI_DMAC();
	
void sd_setup_dmac(struct himci_host *host){ 
      int ulChnn;
      unsigned int PeripheralId;
	ulChnn =  host->dma_num;

	if(host->dma_dir == DMA_TO_DEVICE)
		PeripheralId = DMAC_MMC_TX_REQ; //DMAC_MMC_TX_REQ
	else
		PeripheralId = DMAC_MMC_RX_REQ; //DMAC_MMC_RX_REQ

	if(host->dma_dir == DMA_TO_DEVICE)	
		dmac_start_m2p(ulChnn, host->sg_cpu[0].dsadr, PeripheralId, host->sg_cpu[host->dma_count].dlen, 0);
      else
		dmac_start_m2p(ulChnn, host->sg_cpu[0].dtadr, PeripheralId, host->sg_cpu[host->dma_count].dlen, 0);      	 

	if(dmac_channelstart(ulChnn)!= 0) 
	{
		printk("DMAC Channel Start Error.\n");
		return ;
	}
	host->dma_count++;   

}

static void himci_setup_data(struct himci_host *host, struct mmc_data *data)
{
	int i,j;

	host->data = data;

	if (data->flags & MMC_DATA_READ) {
		host->dma_dir = DMA_FROM_DEVICE;
	} else {
		host->dma_dir = DMA_TO_DEVICE;
	}
	host->dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
				   host->dma_dir); 
	for (i = 0,j = 0; i < host->dma_len; i++,j++) {
		if (data->flags & MMC_DATA_READ) {
			host->sg_cpu[i].dtadr = sg_dma_address(&data->sg[j]);
		} else {
			host->sg_cpu[i].dsadr = sg_dma_address(&data->sg[j]);
		}
		if((sg_dma_len(&data->sg[j]))>0xffc)
		{ 
		    host->sg_cpu[i].dlen = ((sg_dma_len(&data->sg[j]))>>1);
		    host->sg_cpu[i].ddadr = host->sg_dma + (i + 1) *sizeof(struct hi_dma_desc);   		    
		    host->dma_len +=1;		    
		      i++;
	    		if (data->flags & MMC_DATA_READ) {
	    			host->sg_cpu[i].dtadr = sg_dma_address(&data->sg[j])+host->sg_cpu[i-1].dlen;
	    		} else {
	    			host->sg_cpu[i].dsadr = sg_dma_address(&data->sg[j])+host->sg_cpu[i-1].dlen;		
	    		}
		    host->sg_cpu[i].dlen = sg_dma_len(&data->sg[j]) - host->sg_cpu[i-1].dlen;		
		    host->sg_cpu[i].ddadr = host->sg_dma + (i + 1) *sizeof(struct hi_dma_desc);    				    
		}   
		else
		{
		    host->sg_cpu[i].dlen = sg_dma_len(&data->sg[j]);		
		    host->sg_cpu[i].ddadr = host->sg_dma + (i + 1) *sizeof(struct hi_dma_desc); 		    
		}            
	}
	host->sg_cpu[host->dma_len - 1].ddadr = 0;	
	host->dma_count = 0;
	wmb();

    register_dma_hook(himci_dma_irq_hook);
    sd_setup_dmac(host);
}

/*
static void himci_finish_request(struct himci_host *host, struct mmc_request *mrq)
{
	DBG("PXAMCI: request done\n");
	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;
	mmc_request_done(host->mmc, mrq);
}
*/
static int himci_cmd_done(struct himci_host *sd_host, int stat)
{
    struct mmc_command *cmd;

	if(sd_host == NULL)
		return 1;
	
     cmd = sd_host->cmd;
	sd_host->cmd = NULL;	

	if (stat) {
		cmd->error = MMC_ERR_TIMEOUT;
	} else{
		cmd->error = MMC_ERR_NONE;
	}


//	himci_finish_request(sd_host, sd_host->mrq);

	return 1;
}

static int himci_data_done(struct himci_host *sd_host, int stat)
{ 
	struct mmc_data *data;

	  data = sd_host->data;

	if (stat)
	{
		data->error = MMC_ERR_TIMEOUT;
             data->bytes_xfered = 0;
	}
	else
	{
	    data->error = MMC_ERR_NONE;
	    data->bytes_xfered = data->blocks << data->blksz_bits;
	}

	dma_unmap_sg(mmc_dev(sd_host->mmc), data->sg, sd_host->dma_len,
		     sd_host->dma_dir);

	sd_host->data = NULL;

		//himci_finish_request(sd_host, sd_host->mrq);

	return 1;
}


/***************************************************************
retval = Read_Write_BlkData(0, 
                            (io_request->sector * io_request->block_len), 
                            (request->nob * request->block_len),
                            request->buffer,
                            request->response,
                            NULL,
                            NULL,
                            0,
                            (io_request->cmd == READ ? 0x50 : 0xd0) 
                            );


***************************************************************/

static void himci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{

	struct himci_host *sd_host = NULL;  

      	sd_host = mmc_priv(mmc); 
	sd_host->mrq = mrq;
	sd_host->cmd = mrq->cmd;

	if (mrq->data) {
	     int ret;
	     unsigned char response[16];
	     himci_setup_data(sd_host, mrq->data);
	     ret=read_write_blkdata(0,
	                           (unsigned long)mrq->cmd->arg,   //数据起始地址，以字节为单位
	                           (unsigned long)(mrq->data->blocks<<mrq->data->blksz_bits),  //要传输的数据长度
	                           response,
	                           (unsigned char *)sd_host->cmd->resp,
	                           NULL,
	                           NULL,
	                           0,
	                           ((mrq->data->flags & MMC_DATA_READ)?0x50 : 0xd0)                   
	                           );
	     himci_data_done(sd_host,ret);
	}
	else
	{
	    int ret;
	    ret = mrq->cmd->opcode;
	    if ((ret >=3 && ret <= 10)||ret==13||ret==15||ret==55)
	    {
               rca_consistent(0,(int)(mrq->cmd->arg>>16));
	    }

        if(ret==51)   
        {
             char *virt;
	    	virt = page_address(mrq->data->sg->page) + mrq->data->sg->offset;	    	
	    	virt[0]=0;
	    	virt[1]=5;
	    	himci_cmd_done(sd_host,0);
	    	return;
        } 
	    ret=form_send_cmd(0,
	                  (int)mrq->cmd->opcode,
	                  (unsigned long)mrq->cmd->arg,
	                  (unsigned char *)sd_host->cmd->resp,
	                  1
	                  );
	    himci_cmd_done(sd_host,ret);
	}
}


static void himci_dma_irq_hook(unsigned long chanel,unsigned long ret)
{ 
   if(*((int *)ret)>0){
	   if(himci_host_g->sg_cpu[himci_host_g->dma_count - 1].ddadr != 0)
	   {
		      if(himci_host_g->dma_dir == DMA_TO_DEVICE)	
				dmac_start_m2p(himci_host_g->dma_num, himci_host_g->sg_cpu[himci_host_g->dma_count].dsadr, DMAC_MMC_TX_REQ, himci_host_g->sg_cpu[himci_host_g->dma_count].dlen, 0);
		      else
				dmac_start_m2p(himci_host_g->dma_num, himci_host_g->sg_cpu[himci_host_g->dma_count].dtadr, DMAC_MMC_RX_REQ, himci_host_g->sg_cpu[himci_host_g->dma_count].dlen, 0);      	 

			if(dmac_channelstart(himci_host_g->dma_num)!= 0) 
			{
				printk("DMAC Channel Start Error.\n");
				return;
			}
			himci_host_g->dma_count++;
	   }
	   else
	   {
	   	unregister_dma_hook();
	   }
  }
   else
   {
       printk("himci_dma_irq_hook : DMA error!\n");
   }
}


static void himci_detect_irq_hook(void)
{     

//	printk("chiptest himci_detect_irq_hook come in!\n");
     if(himci_host_g->card_insert)
      	{
//      		printk("chiptest himci_detect_irq_hook himci_host_g->card_insert!\n");
      	       himci_host_g->card_insert = sd_mmc_check_card(0);    	    
      	       HI_TRACE(4,"himci_host_g->card_insert=%d\n",himci_host_g->card_insert);      	       
       	mmc_detect_change(himci_host_g->mmc,0); 
      	}
      else
      	{
   //   		printk("chiptest himci_detect_irq_hook not card insert!\n");
      		himci_host_g->card_insert = sd_mmc_check_card(0);
      	      	HI_TRACE(4,"himci_host_g->card_insert=%d\n",himci_host_g->card_insert);      		
      		if(himci_host_g->card_insert){
      			mmc_detect_change(himci_host_g->mmc,0);
      		}
      	}
}


static void himci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	 int ret;
       unsigned long dwClkDivdata;
	 if(ios->clock==1)
	 {
			    //if(mmc->mode == MMC_MODE_SD)
			    //   dwClkDivdata = SD_FREQDIVIDER;//divide by 1/2.
			    //else
			       dwClkDivdata = MMC_FREQDIVIDER;//No divide 1.	 
		       
			if(set_clk_frequency(0,dwClkDivdata,(unsigned char *)&ret))
			{
			    printk("set high ndividerval failed!\n");
			}
	 }
	 else
	 {	 
			if(set_clk_frequency(0,(CIU_CLK/(SD_FOD_VALUE*2)),(unsigned char *)&ret))
			{
			    printk("set low ndividerval failed!\n");
			}    	
	 }

    if(ios->bus_width)
  	{
	  	 ret = 0;
	     set_ip_parameters(5,(unsigned long*)(&ret));
  	}
    else
  	{
	  	 ret = 0;
	     set_ip_parameters(6,(unsigned long*)(&ret));      	 
  	}
}

int himci_get_ro(struct mmc_host *host)
{
    return sd_mmc_check_ro(0);
    //return 0;
}

static struct mmc_host_ops himci_ops = {
	.request	= himci_request,
	.get_ro	= himci_get_ro,
	.set_ios	= himci_set_ios,
};
extern unsigned int read_dma_channelnum(void);
static int himci_probe(struct device *dev)
{
	struct mmc_host *mmc;
	struct himci_host *sd_host = NULL;
	int ret;		
      _ENTER();
	mmc = mmc_alloc_host(sizeof(struct himci_host), dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto out;
	}
	mmc->ops = &himci_ops;
	mmc->f_min = 0;
	mmc->f_max = 25000;
      mmc->caps |=1;	
      mmc->card_selected=NULL;

	sd_host = mmc_priv(mmc);
	himci_host_g=sd_host;
      sd_host->dma_num=read_dma_channelnum();
	sd_host->mmc = mmc;
	mmc->ocr_avail =MMC_VDD_32_33|MMC_VDD_33_34;
	mmc->ocr = mmc->ocr_avail;
	sd_host->sg_cpu = dma_alloc_coherent(NULL, 2*PAGE_SIZE, &sd_host->sg_dma, GFP_KERNEL);
	if (!sd_host->sg_cpu) {
		ret = -ENOMEM;
		goto out;
	}

	sd_host->card_insert = sd_mmc_check_card(0);
	spin_lock_init(&sd_host->lock);

    sd_register_detect_hook(0,himci_detect_irq_hook);
      
	dev_set_drvdata(dev, mmc);

      sd_host->himci_queue = create_singlethread_workqueue("himci_queue");
	mmc_add_host(mmc);
       _LEAVE();
	return 0;

 out:
	if (sd_host) {
		if (sd_host->sg_cpu)
			dma_free_coherent(NULL, PAGE_SIZE, sd_host->sg_cpu, sd_host->sg_dma);
	}
	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

static int himci_remove(struct device *dev)
{
	struct mmc_host *mmc = dev_get_drvdata(dev);
	dev_set_drvdata(dev, NULL);

	if (mmc) {
		struct himci_host *host = mmc_priv(mmc);

             flush_workqueue(host->himci_queue);
             destroy_workqueue(host->himci_queue);
             host->himci_queue=NULL;
		mmc_remove_host(mmc);

		dma_free_coherent(dev, 2*PAGE_SIZE, host->sg_cpu, host->sg_dma);

		mmc_free_host(mmc);
	}
	return 0;
}
static void  himmci_dev_release(struct device * dev)
{
	return ;
};
static struct platform_device himmci_device = {
	.name		= "himmci",
	.id		= 0x19810109,
	.dev ={
	         .release=himmci_dev_release,
		},
};

static struct platform_device *devices[] __initdata = {
	&himmci_device,
};

static struct device_driver himmci_driver = {
	.name		= "himmci",
	.bus		       = &platform_bus_type,
	.probe		= himci_probe,
	.remove		= himci_remove,
};

static int __init himci_init(void)
{
/*
	kcom_getby_uuid(UUID_HI_DMAC_V_1_0_0, &kcom);
	if(kcom ==NULL) 
	{	
		printk(KERN_ERR "DMAC: can't access DMAC, start DMAC service first.\n");	
		return -1;
	}
	kcom_hidmac = kcom_to_instance(kcom, struct kcom_hi_dmac, kcom);
*/
	//print the module version
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");

	if(KCOM_HI_DMAC_INIT() != 0 )
	{
		printk(KERN_ERR "himci_init kcom_getby_uuid error,please comfirm the damc mode is ok!\n");	
		return -1;
	}

   	platform_add_devices(devices, ARRAY_SIZE(devices));
	return driver_register(&himmci_driver);
}


static void __exit himci_exit(void)
{
	driver_unregister(&himmci_driver);
	platform_device_unregister(&himmci_device);
	sd_register_detect_hook(0,NULL);
	//kcom_put(kcom);
	KCOM_HI_DMAC_EXIT();
}


module_init(himci_init);
module_exit(himci_exit);

MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
MODULE_DESCRIPTION("hisilicon SD MMC Card Interface Driver");
MODULE_LICENSE("GPL");
