
#ifndef __KCOM_GPIO_I2C_H__
#define __KCOM_GPIO_I2C_H__

#define UUID_KCOM_GPIO_I2C_V1_0_0_0	"gpio-i2c-1.0.0.0"

struct kcom_gpio_i2c {
	struct kcom_object kcom;
	unsigned char  (*gpio_vs6724_read)(unsigned char devaddress,unsigned int address);
	void  (*gpio_vs6724_write)(unsigned char devaddress,unsigned int address,unsigned char value);
	unsigned char (*gpio_i2c_read)(unsigned char devaddress, unsigned char address);
	unsigned char (*gpio_i2c_read_tw2815)(unsigned char devaddress, unsigned char address);
	unsigned char (*gpio_sccb_read)(unsigned char devaddress, unsigned char address);
	void (*gpio_i2c_write)(unsigned char devaddress, unsigned char address, unsigned char value);
};

#ifndef __KCOM_GPIO_I2C_INTER__

extern struct kcom_gpio_i2c *g_kcom_gpio_i2c;

#define DECLARE_KCOM_GPIO_I2C() struct kcom_gpio_i2c *g_kcom_gpio_i2c
#define KCOM_GPIO_I2C_INIT()	KCOM_GET_INSTANCE(UUID_KCOM_GPIO_I2C_V1_0_0_0, g_kcom_gpio_i2c)
#define KCOM_GPIO_I2C_EXIT()	KCOM_PUT_INSTANCE(g_kcom_gpio_i2c)

#define gpio_vs6724_read	g_kcom_gpio_i2c->gpio_vs6724_read
#define gpio_vs6724_write	g_kcom_gpio_i2c->gpio_vs6724_write
#define gpio_i2c_read		g_kcom_gpio_i2c->gpio_i2c_read	
#define gpio_i2c_read_tw2815	g_kcom_gpio_i2c->gpio_i2c_read_tw2815
#define gpio_sccb_read		g_kcom_gpio_i2c->gpio_sccb_read	
#define gpio_i2c_write		g_kcom_gpio_i2c->gpio_i2c_write	

#endif

#endif

