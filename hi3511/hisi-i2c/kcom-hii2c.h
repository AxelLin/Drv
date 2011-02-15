#ifndef __KCOM_HI_I2C_H__
#define __KCOM_HI_I2C_H__

#define UUID_KCOM_HI_I2C_V1_0_0_0	"hi-i2c-1.0.0.0"


#define I2C_CMD_WRITE 0x1
#define I2C_CMD_READ 0x2

struct i2c_ioctl_data
{
	unsigned char devaddr;	/*dev addr*/
	unsigned int  regaddr;	/*reg addr*/
	int reg_count;	/* re*/	
	unsigned char* data;	/*return data*/
	unsigned long data_count; /*read or write data count*/

};
struct kcom_hi_i2c {
	struct kcom_object kcom;
	unsigned char  (*hi_i2c_read)(unsigned char devaddress,unsigned char address);
	int (*hi_i2c_write)(unsigned char devaddress,unsigned char address,unsigned char value);	
	int  (*hi_i2c_muti_read)(unsigned char devaddress, unsigned int regaddress, int reg_addr_count,
	                           unsigned char* data, unsigned long  count);
    int  (*hi_i2c_muti_write)(unsigned char devaddress,unsigned int regaddress,int reg_addr_count, 
	                           unsigned char* data,unsigned long  count);	
	unsigned char (*hi_i2c_readspecial)(unsigned char devaddress, unsigned char pageindex,
	                                     unsigned char regaddress);                        
	int (*hi_i2c_writespecial)(unsigned char devaddress,unsigned char pageindex,
	                            unsigned char regaddress,unsigned char data);
};

#ifndef __KCOM_HI_I2C_INTER__

extern struct kcom_hi_i2c *p_kcom_hi_i2c;

#define DECLARE_KCOM_HI_I2C() struct kcom_hi_i2c *p_kcom_hi_i2c
#define KCOM_HI_I2C_INIT()	KCOM_GET_INSTANCE(UUID_KCOM_HI_I2C_V1_0_0_0, p_kcom_hi_i2c)
#define KCOM_HI_I2C_EXIT()	KCOM_PUT_INSTANCE(p_kcom_hi_i2c)

#define hi_i2c_read	p_kcom_hi_i2c->hi_i2c_read
#define hi_i2c_write	p_kcom_hi_i2c->hi_i2c_write
#define hi_i2c_muti_read	p_kcom_hi_i2c->hi_i2c_muti_read
#define hi_i2c_muti_write		p_kcom_hi_i2c->hi_i2c_muti_write
#define hi_i2c_readspecial	p_kcom_hi_i2c->hi_i2c_readspecial	
#define hi_i2c_writespecial	p_kcom_hi_i2c->hi_i2c_writespecial
	

#endif

#endif

