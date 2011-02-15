 /*     extdrv/include/gpio_i2c.h for Linux .
 *
 * 
 * This file declares functions for user.
 *
 * History:
 *     03-Apr-2006 create this file
 *      
 */
 

#ifndef _GPIO_I2C_H
#define _GPIO_I2C_H

#define __KCOM_GPIO_I2C_INTER__
#include "kcom-gpio_i2c.h"

#define OSDRV_MODULE_VERSION_STRING "GPIO_I2C-MDC030001 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:53:17"

unsigned char  gpio_vs6724_read(unsigned char devaddress,unsigned int address);
void  gpio_vs6724_write(unsigned char devaddress,unsigned int address,unsigned char value);
unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address);
unsigned char gpio_i2c_read_tw2815(unsigned char devaddress, unsigned char address);
unsigned char gpio_sccb_read(unsigned char devaddress, unsigned char address);
void gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char value);
int kcom_gpio_i2c_register(void);
void kcom_gpio_i2c_unregister(void);


#endif
