/* ./arch/arm/mach-hi3511_v100_f01/dmac.h
 * 
 *
 * History: 
 *      17-August-2006 create this file
 */
#ifndef __HI_INC_ECSDMAC_H__
#define __HI_INC_ECSDMAC_H__

	#include <linux/config.h>

#define OSDRV_MODULE_VERSION_STRING "HIDMAC-MDC030003 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:52:37"

	#define  dmac_writew(addr,value)      ((*(volatile unsigned int *)(addr)) = (value))
	#define  dmac_readw(addr,v)           (v =(*(volatile unsigned int *)(addr)))

    
	#define DDRAM_ADRS              0xE0000000              /* fixed */
	#define DDRAM_SIZE              0x10000000              /* 256MB DDR. */


	#define FLASH_BASE              0x34000000
	#define FLASH_SIZE              0x02000000              /* (32MB) */

	#define ITCM_BASE               0x0
	#define ITCM_SIZE               0x800
   

	#define DMAC_CONFIGURATIONx_HALT_DMA_ENABLE     (0x01L<<18)
	#define DMAC_CONFIGURATIONx_ACTIVE              (0x01L<<17)
	#define DMAC_CONFIGURATIONx_CHANNEL_ENABLE       1
	#define DMAC_CONFIGURATIONx_CHANNEL_DISABLE      0
    

	#define DMAC_BASE_REG                           0x10130000


	#define DMAC_INTSTATUS 	                        IO_ADDRESS(DMAC_BASE_REG + 0X00)
	#define DMAC_INTTCSTATUS                        IO_ADDRESS(DMAC_BASE_REG + 0X04)
	#define DMAC_INTTCCLEAR                         IO_ADDRESS(DMAC_BASE_REG + 0X08)
	#define DMAC_INTERRORSTATUS                     IO_ADDRESS(DMAC_BASE_REG + 0X0C)
	#define DMAC_INTERRCLR                          IO_ADDRESS(DMAC_BASE_REG + 0X10)
	#define DMAC_RAWINTTCSTATUS                     IO_ADDRESS(DMAC_BASE_REG + 0X14)
	#define DMAC_RAWINTERRORSTATUS                  IO_ADDRESS(DMAC_BASE_REG + 0X18)
	#define DMAC_ENBLDCHNS                          IO_ADDRESS(DMAC_BASE_REG + 0X1C)
	#define DMAC_CONFIG                             IO_ADDRESS(DMAC_BASE_REG + 0X30)
	#define DMAC_SYNC                               IO_ADDRESS(DMAC_BASE_REG + 0X34)

	#define DMAC_MAXTRANSFERSIZE                    0x0fff /*the max length is denoted by 0-11bit*/
	#define MAXTRANSFERSIZE                         DMAC_MAXTRANSFERSIZE
	#define DMAC_CxDISABLE                          0x00
	#define DMAC_CxENABLE                           0x01

	/*the definition for DMAC channel register*/

	#define DMAC_CxBASE(i)                          IO_ADDRESS(DMAC_BASE_REG + 0x100+i*0x20)
	#define DMAC_CxSRCADDR(i)                       DMAC_CxBASE(i)
	#define DMAC_CxDESTADDR(i)                      (DMAC_CxBASE(i)+0x04)
	#define DMAC_CxLLI(i)                           (DMAC_CxBASE(i)+0x08)
	#define DMAC_CxCONTROL(i)                       (DMAC_CxBASE(i)+0x0C)
	#define DMAC_CxCONFIG(i)                        (DMAC_CxBASE(i)+0x10)

	/*the means the bit in the channel control register*/ 
	
	#define DMAC_CxCONTROL_M2M                      0x8f489000  /* Dwidth=32,burst size=4 */
	#define DMAC_CxCONTROL_LLIM2M                   0x0f480000  /* Dwidth=32,burst size=1 */
	#define DMAC_CxLLI_LM                           0x01	
	
	#define DMAC_CxCONFIG_M2M                       0xc000
	#define DMAC_CxCONFIG_LLIM2M                    0xc000
	
	/*#define DMAC_CxCONFIG_M2M  0x4001*/
	
	#define DMAC_CHANNEL_ENABLE                     1
	#define DMAC_CHANNEL_DISABLE                    0xfffffffe
	
	#define DMAC_CxCONTROL_P2M                      0x89409000
	#define DMAC_CxCONFIG_P2M                       0xd000
	
	#define DMAC_CxCONTROL_M2P                      0x86089000
	#define DMAC_CxCONFIG_M2P                       0xc800

	/*default the config and sync regsiter for DMAC controller*/
	#define DMAC_CONFIG_VAL                         0x01    /*M1,M2 little endian, enable DMAC*/
	#define DMAC_SYNC_VAL                           0x0     /*enable the sync logic for the 16 peripheral*/

	/*definition for the return value*/

	    

	#define DMAC_ERROR_BASE                         100
	#define DMAC_CHANNEL_INVALID                    (DMAC_ERROR_BASE+1)
	
	#define DMAC_TRXFERSIZE_INVALID                 (DMAC_ERROR_BASE+2)
	#define DMAC_SOURCE_ADDRESS_INVALID             (DMAC_ERROR_BASE+3)
	#define DMAC_DESTINATION_ADDRESS_INVALID        (DMAC_ERROR_BASE+4)
	#define DMAC_MEMORY_ADDRESS_INVALID             (DMAC_ERROR_BASE+5)
	#define DMAC_PERIPHERAL_ID_INVALID              (DMAC_ERROR_BASE+6)
	#define DMAC_DIRECTION_ERROR                    (DMAC_ERROR_BASE+7)
	#define DMAC_TRXFER_ERROR                       (DMAC_ERROR_BASE+8)
	#define DMAC_LLIHEAD_ERROR                      (DMAC_ERROR_BASE+9)
	#define DMAC_SWIDTH_ERROR                       (DMAC_ERROR_BASE+0xa)
	#define DMAC_LLI_ADDRESS_INVALID                (DMAC_ERROR_BASE+0xb)
	#define DMAC_TRANS_CONTROL_INVALID              (DMAC_ERROR_BASE+0xc)
	#define DMAC_MEMORY_ALLOCATE_ERROR              (DMAC_ERROR_BASE+0xd)
	#define DMAC_NOT_FINISHED                       (DMAC_ERROR_BASE+0xe)
	
	#define DMAC_TIMEOUT	                        (DMAC_ERROR_BASE+0xf)
	#define DMAC_CHN_SUCCESS                        (DMAC_ERROR_BASE+0x10)
	#define DMAC_CHN_ERROR                          (DMAC_ERROR_BASE+0x11)
	#define DMAC_CHN_TIMEOUT                        (DMAC_ERROR_BASE+0x12)
	#define DMAC_CHN_ALLOCAT                        (DMAC_ERROR_BASE+0x13)
	#define DMAC_CHN_VACANCY                        (DMAC_ERROR_BASE+0x14)
	
	#define DMAC_CONFIGURATIONx_ACTIVE_NOT           0
	


	#define DMAC_MAX_PERIPHERALS	                 16
	#define DMAC_MAX_CHANNELS 	                     8
	#define MEM_MAX_NUM                              3

	/*define the Data register for the pepherial */
	#define OTG20_DMA4_DATA_REG                     0x0   //need modify
	#define OTG20_DMA1_DATA_REG                     0x0   //need modify
	
	#define I2C_BASE_REG                             0x101f6000
	#define I2C_DATA_REG                             (I2C_BASE_REG+0x0)
	
	#define SIO0_BASE_REG                            0x80080000
	#define SIO0_TX_LEFT_FIFO                        (SIO0_BASE_REG+0x4c)
	#define SIO0_TX_RIGHT_FIFO                       (SIO0_BASE_REG+0x50)
	#define SIO0_RX_LEFT_FIFO                        (SIO0_BASE_REG+0x54)
	#define SIO0_RX_RIGHT_FIFO                       (SIO0_BASE_REG+0x58)
	#define SIO0_TX_FIFO                             (SIO0_BASE_REG+0xc0)
	#define SIO0_RX_FIFO                             (SIO0_BASE_REG+0xa0)

	#define SIO1_BASE_REG                            0x900a0000
	#define SIO1_RX_LEFT_FIFO                        (SIO1_BASE_REG+0x54)
	#define SIO1_RX_RIGHT_FIFO                       (SIO1_BASE_REG+0x58)
	#define SIO1_RX_FIFO                             (SIO1_BASE_REG+0xa0)
		
	#define SPDIF_BASE_REG                           0x101fd000
	#define SPDIF_DATA_REG                           (SPDIF_BASE_REG+0x10)

	#define ATAH0_BASE_REG                             0x0  //need modify
	#define ATAH0_DATA_REG                             (ATAH0_BASE_REG+0x80)

	#define ATAH1_BASE_REG                             0x0  //need modify
	#define ATAH1_DATA_REG                             (ATAH1_BASE_REG+0x80)
	
	#define SSP_BASE_REG                             0x101f4000
	#define SSP_DATA_REG                             (SSP_BASE_REG+0x08)
	
	#define OTG20_DMA3_DATA_REG                     0x0   //need modify
	#define OTG20_DMA0_DATA_REG                     0x0   //need modify	

	#define USB11_RX_REG                             (0x0)  //need modify
	#define USB11_TX_REG                             (0x0)  //need modify
	
	#define MMC_BASE_REG                             0x90020000
	#define MMC_RX_REG                               (MMC_BASE_REG+0x100)  //need modify
	#define MMC_TX_REG                               (MMC_BASE_REG+0x100)  //need modify

	#define UART0_BASE_REG                           0x101f1000
	#define UART0_DATA_REG                           (UART0_BASE_REG+0x0)
		
	#define PERIPHERIAL1_DATA_REG                     0x0   //need modify
	#define PERIPHERIAL2_DATA_REG                     0x0   //need modify		

	/*the transfer control and configuration value for different peripheral*/
	
	#define OTG20_DMA4_CONTROL                       0x0  //need modify
	#define OTG20_DMA4_CONFIG                        (DMAC_CxCONFIG_P2M|(0x0<<1))
	
	#define OTG20_DMA1_CONTROL                       0x0  //need modify
	#define OTG20_DMA1_CONFIG                        (DMAC_CxCONFIG_M2P|(0x01<<6))	

	#define I2C_RX_CONTROL                           DMAC_CxCONTROL_P2M
	#define I2C_RX_CONFIG                            (DMAC_CxCONFIG_P2M|(0x0<<1))
	
	#define I2C_TX_CONTROL                           DMAC_CxCONTROL_M2P
	#define I2C_TX_CONFIG                            (DMAC_CxCONFIG_M2P|(0x01<<6))
	
	#define SIO0_RX_CONTROL                          0x8a489000  
	#define SIO0_RX_CONFIG                           (DMAC_CxCONFIG_P2M|(0x02<<1))
	

	#define SIO0_TX_CONTROL                          0x85489000 
	#define SIO0_TX_CONFIG                           (DMAC_CxCONFIG_M2P|(0x03<<6))
	
	#define SIO1_RX_CONTROL                          0x8a489000    //need modify
	#define SIO1_RX_CONFIG                           (DMAC_CxCONFIG_P2M|(0x04<<1))	
	
    #define SPDIF_TX_CONTROL                         0x85492000             
    #define SPDIF_TX_CONFIG                          (DMAC_CxCONFIG_M2P|(0x05<<6))                      
	
    #define ATAH0_CONTROL                            0x0   //need modify
    #define ATAH0_CONFIG                             (DMAC_CxCONFIG_P2M|(0x06<<1))
    
    #define ATAH1_CONTROL                            0x0   //need modify
    #define ATAH1_CONFIG                             (DMAC_CxCONFIG_M2P|(0x07<<6))
	
	#define SSP_RX_CONTROL                           0x89449000   
	#define SSP_RX_CONFIG                            (DMAC_CxCONFIG_P2M|(0x08<<1))
	  
	#define SSP_TX_CONTROL                           0x86289000  
	#define SSP_TX_CONFIG                            (DMAC_CxCONFIG_M2P|(0x09<<6))
	
	#define OTG20_DMA3_CONTROL                         0x0   //need modify
	#define OTG20_DMA3_CONFIG                          (DMAC_CxCONFIG_P2M|(0x0a<<1))
	
	#define OTG20_DMA0_CONTROL                         0x0   //need modify
	#define OTG20_DMA0_CONFIG                          (DMAC_CxCONFIG_M2P|(0x0b<<6))	
	
	#define USB11_RX_CONTROL                         0x0   //need modify
	#define USB11_RX_CONFIG                          (DMAC_CxCONFIG_P2M|(0x0a<<1))
	
	#define USB11_TX_CONTROL                         0x0   //need modify
	#define USB11_TX_CONFIG                          (DMAC_CxCONFIG_M2P|(0x0b<<6))
	
	#define MMC_RX_CONTROL                           0x8a492000   
	#define MMC_RX_CONFIG                            (DMAC_CxCONFIG_P2M|(0x0c<<1))
	
	#define MMC_TX_CONTROL                           0x87492000   
	#define MMC_TX_CONFIG                            (DMAC_CxCONFIG_M2P|(0x0d<<6))

	#define UART0_RX_CONTROL                         DMAC_CxCONTROL_P2M
	#define UART0_RX_CONFIG                          (DMAC_CxCONFIG_P2M|(0x0e<<1))
	
	#define UART0_TX_CONTROL                         DMAC_CxCONTROL_M2P
	#define UART0_TX_CONFIG                          (DMAC_CxCONFIG_M2P|(0x0f<<6))
	
	#define PERIPHERIAL1_CONTROL                     0x0   //need modify
	#define PERIPHERIAL1_CONFIG                      (DMAC_CxCONFIG_P2M|(0x0e<<1))
	
	#define PERIPHERIAL2_CONTROL                     0x0   //need modify
	#define PERIPHERIAL2_CONFIG                      (DMAC_CxCONFIG_M2P|(0x0f<<6))
	
	/*DMAC peripheral structure*/
	typedef struct dmac_peripheral
	{
	   	unsigned int         peri_id;          /* peripherial ID*/
	   	void                 *pperi_addr;      /*peripheral data register address*/
	   	unsigned int         transfer_ctrl;    /*default channel control word*/
	   	unsigned int         transfer_cfg;     /*default channel configuration word*/ 
	}dmac_peripheral; 

	typedef struct mem_addr
	{
	    	unsigned int addr_base;
	    	unsigned int size;
	}mem_addr;

#endif /* End of #ifndef __HI_INC_ECSDMACC_H__ */

