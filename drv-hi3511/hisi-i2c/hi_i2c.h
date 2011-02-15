/*
 * extdrv/include/hi_i2c.h for Linux .
 *
 * History: 
 *      10-April-2006 create this file
 *
 *
 */

#ifndef __HI_I2C_H__
#define __HI_I2C_H__

#define __KCOM_HI_I2C_INTER__
#include "kcom-hii2c.h"

#define OSDRV_MODULE_VERSION_STRING "HISI_I2C-MDC030002 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:53:09"

unsigned char hi_i2c_read(unsigned char devaddress, unsigned char regaddress);
int hi_i2c_write(unsigned char devaddress,unsigned char regaddress,unsigned char data);
int hi_i2c_muti_read(unsigned char devaddress, unsigned int regaddress, int reg_addr_count,
	                   unsigned char* data, unsigned long  count);
int hi_i2c_muti_write(unsigned char devaddress,unsigned int regaddress,int reg_addr_count, 
	                    unsigned char* data, unsigned long  data_count);
unsigned char hi_i2c_readspecial(unsigned char devaddress, unsigned char pageindex,unsigned char regaddress);
int hi_i2c_writespecial(unsigned char devaddress,unsigned char pageindex,unsigned char regaddress,unsigned char data);


int kcom_hi_i2c_register(void);
void kcom_hi_i2c_unregister(void);

#endif 
