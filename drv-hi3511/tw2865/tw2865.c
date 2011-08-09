/* ~/source/drv/tw2865/tw2865.c
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
 */


#include <linux/config.h>
#include <linux/kernel.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <linux/string.h>
#include <linux/list.h>
#include <asm/semaphore.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>

#include <asm/hardware.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/kcom.h>
//#include <kcom/gpio_i2c.h>
#include <linux/i2c.h>
#include <asm/arch/yx_iomultplex.h>

#include "tw2865_def.h"
#include "tw2865.h"

#define SET_BIT(x,y)        ((x) |= (y))
#define CLEAR_BIT(x,y)      ((x) &= ~(y))
#define CHKBIT_SET(x,y)     (((x)&(y)) == (y))
#define CHKBIT_CLR(x,y)     (((x)&(y)) == 0)

/*制式及芯片的个数*/
static int norm = TW2865_PAL;
static int chips = 1;


/*确定外部通道和芯片及芯片内部通道的对应关系，此对应关系可能是多样化的*/
struct _chnandchip_{
	unsigned char chip;/*芯片ID*/
	unsigned char chn;/*芯片内部通道ID*/
};
struct _chnandchip_ ChnAndChipNormal[] = {
	{0, 0}, {0, 1}, {0, 2}, {0, 3},/*1-4*/
	{1, 0}, {1, 1}, {1, 2}, {1, 3},/*5-8*/
	{2, 0}, {2, 1}, {2, 2}, {2, 3},/*9-12*/
	{3, 0}, {3, 1}, {3, 2}, {3, 3}/*13-16*/
};
/*定义这个变量主要是应对多样化*/
struct _chnandchip_ *pChnAndChip = ChnAndChipNormal;



/*TW2865的I2C地址*/
#define TW2865A_I2C_ADDR 	0x50
#define TW2865B_I2C_ADDR 	0x54
#define TW2865C_I2C_ADDR 	0x52
#define TW2865D_I2C_ADDR 	0x56
unsigned char Tw2865I2cAddr[] = {/*各块TW2865的I2C地址*/
		TW2865A_I2C_ADDR, TW2865B_I2C_ADDR, TW2865C_I2C_ADDR, TW2865D_I2C_ADDR};



/*通过外部通道号获取I2C地址*/
#define I2C_ADDR(ChnNo) 	Tw2865I2cAddr[pChnAndChip[ChnNo].chip]
/*通过外部通道号获取该通道对应的芯片内部通道号*/
#define CHN_IN_CHIP(ChnNo) pChnAndChip[ChnNo].chn
/*判断外部通道号是否合法*/
#define INVALID_CHNNO(ChnNo) ((ChnNo) >= (chips * 4))
/*判断芯片ID是否合法*/
#define INVALID_CHIPNO(ChipNo) ((ChipNo) >= chips)

#define	TW2865a_DRIVER_NAME		        "tw2835a"

#define I2C_DRIVERID_TW2865a		28151
static struct i2c_client *save_client;
#define RW_RETRY_TIME   5

static DECLARE_MUTEX(tw2865_mutex);

static struct i2c_driver tw2865a_driver;
//static struct i2c_driver tw2865b_driver;



static unsigned short ignore[]      = {I2C_CLIENT_END};
static unsigned short normal_addra[] = {TW2865A_I2C_ADDR>>1, I2C_CLIENT_END};

static struct i2c_client_address_data addr_dataa = {
	.normal_i2c		= normal_addra,
	.probe			= ignore,
	.ignore			= ignore,
};
tw2865_set_audiooutput RegisterAudioOutput = {
	.PlaybackOrLoop = 0,
	.channel = 0
};
/*0:demute, 1:mute*/
unsigned char RegisterAudioMute = 0;
unsigned char channel_array[8] = {0x20,0x64,0xa8,0xec,0x31,0x75,0xb9,0xfd};
unsigned char channel_array_default[8] = {0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe};
unsigned char channel_array_our[8] = {0x10,0x32,0x98,0xba,0x54,0x76,0xdc,0xfe};
unsigned char channel_array_4chn[8] = {0x10,0x54,0x98,0xdc,0x32,0x76,0xba,0xfe};
static void Tw2865SetNormalRegister(unsigned char addr, 
						unsigned char mode, unsigned char chn);
						

unsigned char tw2865_byte_read(unsigned char chip_addr, unsigned char addr)
{
  int i;
  unsigned int have_retried = 0;
  unsigned char msgbuf0[1];
  unsigned char msgbuf1[1];
  struct i2c_msg msg[2] = {{chip_addr>>1, 0, 1, msgbuf0 }, 
	                         {chip_addr>>1, I2C_M_RD | I2C_M_NOSTART, 0, msgbuf1 }
	                        };
	                        
	down(&tw2865_mutex);
	for(i=0;i<RW_RETRY_TIME;i++)
	{
        msgbuf0[0] = addr;       
        msg[0].len = 1;
        msg[1].len = 1;
        if (i2c_transfer(save_client->adapter, msg, 2) < 0) {
            printk("i2c transfer fail in TW2815 read\n");
        } else {            
		if (have_retried) { 
				break;
		    }
		    have_retried = 1;
		}
	}
	up(&tw2865_mutex);
	if (i == RW_RETRY_TIME) {
		printk("read tw2815 fail!\n");
		return -EAGAIN;
	}
	return msgbuf1[0];
	
}

/*
 * TW2815 write register routine .
 * @param page: page number
 * @param addr: register address
 * @param value: the data write to the register
 * @return 1:error,0:succ.
 */
unsigned int tw2865_byte_write(unsigned char chip_addr, unsigned char addr, unsigned char value)
{

    int i;
    unsigned char msgbuf[2];	
    struct i2c_msg msg = {chip_addr>>1, 0,1, msgbuf};
		down(&tw2865_mutex);

		for(i=0;i<RW_RETRY_TIME;i++)
		{
   	 	msgbuf[0] = addr;
    	msgbuf[1] = value;
    	msg.len   = 2;
    	if (i2c_transfer(save_client->adapter, &msg, 1) < 0)  
    		{
       	 	printk("i2c transfer fail in TW2815 write\n");
       	 	up(&tw2865_mutex);
       	 	return 1;
    		}
    }
		up(&tw2865_mutex);
	
		return 0;	
}

/*
 * TW2815 write a group of register routine .
 * @param page: page number
 * @param addr: start register address
 * @param tbl_ptr: the first data pointer
 * @param tbl_cnt: the number of data
 * @return value:nothing.
 */
static void tw2865_write_table(unsigned char chip_addr,
		unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
    int i;
    unsigned int value;
    int  temp;

    for ( i=0;i<tbl_cnt;i++)
      {
        value = *tbl_ptr;
        temp = addr+i;
        if(tw2865_byte_write(chip_addr,temp,value))
        	{
        		printk("TW2815_ERROR: write failed!\n");
            return;        	
        	}
        tbl_ptr++;
      }
}

static void tw2865_read_table(unsigned char chip_addr,
						unsigned char addr, unsigned char reg_num)
{
	unsigned char i = 0, temp = 0;
	
	for(i = 0; i < reg_num; i++ )
	{
		temp = tw2865_byte_read(chip_addr, addr + i);
		printk("reg 0x%02x=0x%02x,", addr + i, temp);
		if(((i + 1) % 4) == 0)
		{
			printk("\n");
		}
	}
}

/*
static void set_audio_cascad(void)
{
	//unsigned int tmp1,tmp2;
	unsigned char tmp;
	//set the two audio chips cascade
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xcf);
	SET_BIT(tmp, 0x80);
	CLEAR_BIT(tmp, 0x40);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xcf,tmp);
	//tw2865_byte_write(TW2865B_I2C_ADDR,0xcf,tmp);
	//First Stage PalyBackIn audio
	//8 channel audios for adatr output
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xd2);
	CLEAR_BIT(tmp,0x30);
	CLEAR_BIT(tmp,0x01);
	SET_BIT(tmp, 0x02);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xd2,tmp);
#if 1
	//for ox52
	//4 channel audios for adatr output
	//may be someproblems
	//tmp = tw2865_byte_read(TW2865B_I2C_ADDR,0xd2);
	CLEAR_BIT(tmp,0x30);
 	SET_BIT(tmp, 0x01);
 //	tw2865_byte_write(TW2865B_I2C_ADDR,0xd2,tmp);	
#endif
	//enanble audio adc and dac
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xdb);
	SET_BIT(tmp, 0xc0);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xdb,tmp);
	//for 0x52
//	tmp = tw2865_byte_read(TW2865B_I2C_ADDR,0xdb);
	SET_BIT(tmp, 0xc0);
//	tw2865_byte_write(TW2865B_I2C_ADDR,0xdb,tmp);	
	//match the actual audio channel with virtual audio channel
	tw2865_byte_write(TW2865A_I2C_ADDR,0xd3,10);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xd4,32);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xd7,54);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xd3,76);
	//here you also can add some codes to configure 0x52's audio channels
	//
}

static void set_single_chip(void)
{
	//unsigned int tmp1,tmp2;
	unsigned char tmp;
	//set the two audio chips single
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xcf);
	CLEAR_BIT(tmp, 0xc0);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xcf,0x00);
	//First Stage PalyBackIn audio
	//4 channel audios for adatr output
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xd2);
	CLEAR_BIT(tmp,0x30);
 	SET_BIT(tmp, 0x01);
 	tw2865_byte_write(TW2865A_I2C_ADDR,0xd2,tmp);
#if 1
	//for ox52
	// channel audios for adatr output
//	tmp = tw2865_byte_read(TW2865B_I2C_ADDR,0xd2);
//	CLEAR_BIT(tmp,0x30);
 //	SET_BIT(tmp, 0x01);
 //	tw2865_byte_write(TW2865B_I2C_ADDR,0xd2,tmp);	
#endif
	//enanble audio adc and dac
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xdb);
	SET_BIT(tmp, 0xc0);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xdb,tmp);

	tw2865_byte_write(TW2865A_I2C_ADDR,0xf3,0x00);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xf4,0x01);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xf5,0x00);

}
*/
static void set_audio_output(unsigned char chip_addr, unsigned char num_path)
{
	unsigned char temp = 0;
	unsigned char help = 0;
	#if 0
	if(cascad_judge)
	{
	return;
	}
  	#endif		
    	if(0 < num_path && num_path <=2) {
	 	help = 0;
   	} else if(2 < num_path && num_path <= 4) {
       	 help = 1;
    	} else if(4 < num_path && num_path <= 8) {
       	 help = 2;
    	} else if(8 <num_path && num_path <=16) {
       	 help = 3;
    	}
    	temp = tw2865_byte_read(chip_addr,0xd2) & 0xfc;
   	temp |= (help & 0x03);
       tw2865_byte_write(chip_addr,0xd2,temp);
}
static void set_ada_bitwidth(unsigned char chip_addr, unsigned char bitwidth)
{
	unsigned int ada_bitwidth = 0;
	if(bitwidth == SET_8_BITWIDTH) {
		ada_bitwidth = tw2865_byte_read(chip_addr, 0xdb)& 0xfb;
		ada_bitwidth |= 0x04;
		tw2865_byte_write(chip_addr, 0xdb, ada_bitwidth);
		//set_audio_cascad();
		
	} else if(bitwidth == SET_16_BITWIDTH) {
		ada_bitwidth = tw2865_byte_read(chip_addr, 0xdb)& 0xfb;
		tw2865_byte_write(chip_addr, 0xdb, ada_bitwidth);
		//set_single_chip();
	}
}
static void channel_alloc(unsigned char chip_addr, unsigned char ch)
{
	unsigned char i = 0;
	if(ch == 0)
	{
		for(i =0 ;i < 8;i++)
		{
			tw2865_byte_write(chip_addr,0xd3+i,0);
			tw2865_byte_write(chip_addr,0xd3+i,channel_array_default[i]);
		}
	}
	else if(ch == 1)
	{
		for(i =0 ;i < 8;i++)
		{
			tw2865_byte_write(chip_addr,0xd3+i,0);
			tw2865_byte_write(chip_addr,0xd3+i,channel_array[i]);
		}
		
	}
	else if(ch == 2)
	{
			for(i =0 ;i < 8;i++)
		{
			tw2865_byte_write(chip_addr,0xd3+i,0);
			tw2865_byte_write(chip_addr,0xd3+i,channel_array_our[i]);
		}
	}
	else if(ch == 3)
	{
		for(i = 0; i < 8; i++)
		{
			tw2865_byte_write(chip_addr,0xd3+i,0);
			tw2865_byte_write(chip_addr,0xd3+i,channel_array_4chn[i]);
		}
	}


}
static void setd1(unsigned char chip_addr, unsigned char vdn, unsigned ch)
{
	unsigned char tmp=0;
	vdn-=1;
	tmp = tw2865_byte_read(chip_addr, 0xcb);
	tmp = tmp& (~(0x01<<vdn)); 
	tw2865_byte_write(chip_addr, 0xcb, tmp);
	
	tmp = tw2865_byte_read(chip_addr, 0xca);
	tmp = tmp& (~(0x03<<2*vdn)); 
	tw2865_byte_write(chip_addr, 0xca, 0x00);
	if(vdn == 0) {
		tmp = tw2865_byte_read(chip_addr, 0xfa);
		tmp &= 0xf0; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);
	} else if(vdn == 1) {
		tmp = tw2865_byte_read(chip_addr, 0x6a);
		tmp &= 0xf0; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);
	} else if(vdn == 2) {
		tmp = tw2865_byte_read(chip_addr, 0x6b);
		tmp &= 0xf0; 
		tw2865_byte_write(chip_addr, 0x6b, tmp);
	} else if(vdn == 3) {
		tmp = tw2865_byte_read(chip_addr, 0x6c);
		tmp &= 0xf0; 
		tw2865_byte_write(chip_addr, 0x6c, tmp);
	}
	tmp = tw2865_byte_read(chip_addr, 0xcd);
	tmp = (tmp & (~(0x03<<2*vdn))) | (ch<<2*vdn); 
	tw2865_byte_write(chip_addr, 0xcd, tmp);

}

static void set_2_d1(unsigned char chip_addr, unsigned char vdn, unsigned char ch1, unsigned char ch2)
{
	unsigned char tmp=0;
//	tw2865_byte_write(chip_addr, 0xcb, 0x88);
	vdn -= 1;
	if(vdn == 0) {
		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0x40;
		tmp |= 0x05; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);
	} else if(vdn == 1) {
		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0x00;
		tmp |= 0x05; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);
	} else if(vdn == 2) {
		tmp = tw2865_byte_read(chip_addr, 0x6b);
		tmp |= 0x05; 
		tw2865_byte_write(chip_addr, 0x6b, tmp);
	} else if(vdn == 3) {
		tmp = tw2865_byte_read(chip_addr, 0x6c);
		tmp |= 0x05; 
		tw2865_byte_write(chip_addr, 0x6c, tmp);
	} 
	tw2865_byte_write(chip_addr, 0xcb, 0);

	tmp = tw2865_byte_read(chip_addr, 0xca);
	tmp = tmp | (0x01<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xca, tmp);

	tmp = tw2865_byte_read(chip_addr, 0xcd);
	tmp = (tmp & (~(0x03<<vdn*2))) |(ch1<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xcd, tmp);
	tmp = tw2865_byte_read(chip_addr, 0xcc);
	tmp = (tmp & (~(0x03<<vdn*2))) |(ch2<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xcc, tmp);


}

static void set_4_cif(unsigned char chip_addr, unsigned char vdn)
{
	unsigned char tmp=0;
	vdn -= 1;
	if(vdn == 0) {
		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0x40;
		tmp |= 0x05; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);
	} else if(vdn == 1) {
		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0x40;
		tmp |= 0x05; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);
	}
	tmp = tw2865_byte_read(chip_addr, 0xca);
	tmp = tmp | (0x02<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xca, 0xaa);

	tmp = tw2865_byte_read(chip_addr, 0xcb);
	tmp = tmp | (0x01<<vdn); 
	tw2865_byte_write(chip_addr, 0xcb, 0x00);
	
	tmp = tw2865_byte_read(chip_addr, 0xf9);
	tmp = tmp & 0xfd; 
	tw2865_byte_write(chip_addr, 0xf9, tmp);
}

static void set_4_d1(unsigned char chip_addr, unsigned char vdn)
{
	unsigned char tmp=0;
	vdn -=1;
	if(vdn == 0) {
		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0xf0;
		tmp |= 0x0a; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);

		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0x8f;
		tmp |= 0x70; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);
	} else if(vdn == 1) {
		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0xf0;
		tmp |= 0x0a; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);

		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0x8f;
		tmp |= 0x70; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);
	}
	
	tmp = tw2865_byte_read(chip_addr, 0xca);
	tmp = tmp | (0x02<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xca, tmp);

	tmp = tw2865_byte_read(chip_addr, 0xcb);
	tmp = tmp & (~(0x01<<vdn)); 
	tw2865_byte_write(chip_addr, 0xcb, tmp);

}

void Tw2865_SetAudioOutput(tw2865_set_audiooutput *pAudioOutput)
{
	
	unsigned char RegValue;
	
	if(pAudioOutput == NULL)
	{
		return;
	}	
	tw2865_byte_write(Tw2865I2cAddr[0], 0xcf, 0x80);
	//tw2865_byte_write(Tw2865I2cAddr[1], 0xcf, 0x80);
	RegValue = tw2865_byte_read(Tw2865I2cAddr[0], 0xe0) & 0xe0;
	switch(pAudioOutput->PlaybackOrLoop)
	{
		case 0://playback
			RegValue |= 16;
			break;
		case 1://loop
		default:
			RegValue |= pAudioOutput->channel;
			break;
	}
	tw2865_byte_write(Tw2865I2cAddr[0], 0xe0, RegValue);
}

int tw2865a_open(struct inode * inode, struct file * file)
{
	return 0;
} 

int tw2865a_close(struct inode * inode, struct file * file)
{
	return 0;
}
static void tw2865_reg_dump(unsigned char chip_addr)
{
	tw2865_read_table(chip_addr,0x0,0xff);
	printk("tw2815_reg_dump ok\n");
}
int tw2865a_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int __user *argp = (unsigned int __user *)arg;
	unsigned int uiTmp1;
	unsigned int tmp;
	int j;
	unsigned char *ucpTmp1;
	unsigned long ulTmp1;
	unsigned char ucTmp1;
	unsigned char RegVal;
	tw2865_set_audiooutput AudioOutput;
	tw2865_set_videooutput VideoOutput;

    	tw2865_w_reg tw2865reg; 
    	tw2865_set_2d1 tw2865_2d1;
   	 tw2865_set_videomode tw2865_videomode;
	tw2865_set_analogsetting AnalogSetting;
 	tw2865_set_1d1 tw2865_d1;
//printk(KERN_ERR "the CMD from user is %d==\n",cmd);
	switch(cmd)
	{
		case TW2865CMD_REG_DUMP:
			tw2865_reg_dump(TW2865A_I2C_ADDR);
			break;
		case TW2865CMD_READ_REG:/*read register value*/
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))/*get the register address*/
			{
				return -1;
			}			

			tmp = tw2865_byte_read(TW2865A_I2C_ADDR, (unsigned char)uiTmp1);/*read register*/
			copy_to_user(argp, &tmp, sizeof(tmp));/*return the register value*/
			break;		
			
		case TW2865CMD_READ_REGT:/*read register table*/
			if(copy_from_user(&tw2865reg, argp, sizeof(tw2865reg)))/*get the register address*/
			{
				return -1;
			}			
			tw2865_read_table(TW2865A_I2C_ADDR, tw2865reg.addr, tw2865reg.value);/*read register*/
			break;
			
		case TW2865CMD_WRITE_REG:/*write */
			if(copy_from_user(&tw2865reg, argp, sizeof(tw2865reg)))/*get the register address*/
			{
				return -1;
			}
			tw2865_byte_write(TW2865A_I2C_ADDR, tw2865reg.addr, tw2865reg.value);/*write register*/			
			break;

		case TW2865CMD_SET_1_D1:
			if(copy_from_user(&tw2865_d1, argp, sizeof(tw2865_d1))) {
				return -1;
			}
			setd1(TW2865A_I2C_ADDR, tw2865_d1.vdn, tw2865_d1.ch);
			break;
		case TW2865CMD_SET_2_D1:
			if(copy_from_user(&tw2865_2d1, argp, sizeof(tw2865_2d1))) {
				return -1;
			}
			set_2_d1(TW2865A_I2C_ADDR, tw2865_2d1.vdn, tw2865_2d1.ch1, tw2865_2d1.ch2);
			break;
		case TW2865CMD_SET_4_CIF:
			if(copy_from_user(&tmp, argp, sizeof(tmp))) {
				return -1;
			}
			set_4_cif(TW2865A_I2C_ADDR, tmp);
			break;
		case TW2865CMD_SET_4_D1:
			if(copy_from_user(&tmp, argp, sizeof(tmp))) {
				return -1;
			}
			set_4_d1(TW2865A_I2C_ADDR, tmp);
			break;
		case TW2865CMD_SET_VIDEO_MODE:
			if(copy_from_user(&tw2865_videomode, argp, sizeof(tw2865_videomode))) {
				return -1;
			}
			Tw2865SetNormalRegister(TW2865A_I2C_ADDR, tw2865_videomode.mode, tw2865_videomode.ch);
			break;
		case TW2865CMD_GET_VL:/*get video loss state for all channels*/
			ulTmp1 = 0;
			for(j = 0; j < 4; j ++)/*every channel in a chip*/
			{
				ucTmp1 = tw2865_byte_read(TW2865A_I2C_ADDR,  j * 0x10);
				ulTmp1 |= ((((unsigned long)(ucTmp1 >> 7)) & 0x01) << (j));
			}				
		
			copy_to_user(argp, &ulTmp1, sizeof(ulTmp1));
			break;

		case TW2865CMD_SET_ANALOG:
			if(copy_from_user(&AnalogSetting, argp, sizeof(AnalogSetting)))
			{
				return -1;
			}
			if(AnalogSetting.SettingBitmap & 0x01)//hue
			{
				tw2865_byte_write(TW2865A_I2C_ADDR, \
						0x06 + AnalogSetting.ch * 0x10, AnalogSetting.hue);
			}
			if(AnalogSetting.SettingBitmap & 0x02)//contrast
			{
				tw2865_byte_write(TW2865A_I2C_ADDR, \
						0x02 + AnalogSetting.ch * 0x10, AnalogSetting.contrast);
			}
			if(AnalogSetting.SettingBitmap & 0x04)//brightness
			{
				tw2865_byte_write(TW2865A_I2C_ADDR, \
						0x01 + AnalogSetting.ch * 0x10, AnalogSetting.brightness);
			}
			if(AnalogSetting.SettingBitmap & 0x08)//saturation
			{
				tw2865_byte_write(TW2865A_I2C_ADDR, \
						0x04 + AnalogSetting.ch * 0x10, AnalogSetting.saturation);
				tw2865_byte_write(TW2865A_I2C_ADDR, \
						0x05 + AnalogSetting.ch * 0x10, AnalogSetting.saturation);
			}
			break;

		case TW2865CMD_SET_HUE :
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x06 + AnalogSetting.ch * 0x10, uiTmp1);
			break;

		case TW2865CMD_SET_CONTRAST:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x02 + AnalogSetting.ch * 0x10, uiTmp1);
			break;

		case TW2865CMD_SET_BRIGHTNESS:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x01 + AnalogSetting.ch * 0x10, uiTmp1);
			break;

		case TW2865CMD_SET_SATURATION:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
					0x04 + AnalogSetting.ch * 0x10, uiTmp1);
			tw2865_byte_write(TW2865B_I2C_ADDR, \
					0x05 + AnalogSetting.ch * 0x10, uiTmp1);
			break;
			
		case TW2865CMD_GET_ANALOG:
			if(copy_from_user(&AnalogSetting, argp, sizeof(AnalogSetting)))
			{
				return -1;
			}
			if(AnalogSetting.SettingBitmap & 0x01)//hue
			{
				AnalogSetting.hue = tw2865_byte_read(TW2865A_I2C_ADDR, \
								0x06 + AnalogSetting.ch * 0x10);
			}
			if(AnalogSetting.SettingBitmap & 0x02)//contrast
			{
				AnalogSetting.contrast = tw2865_byte_read(TW2865A_I2C_ADDR, \
								0x02 + AnalogSetting.ch * 0x10);
			}
			if(AnalogSetting.SettingBitmap & 0x04)//brightness
			{
				AnalogSetting.brightness = tw2865_byte_read(TW2865A_I2C_ADDR, \
								0x01 + AnalogSetting.ch * 0x10);
			}
			if(AnalogSetting.SettingBitmap & 0x08)//saturation
			{
				AnalogSetting.saturation = tw2865_byte_read(TW2865A_I2C_ADDR, \
								0x04 + AnalogSetting.ch * 0x10);
			}
			copy_to_user(argp, &AnalogSetting, sizeof(AnalogSetting));
			break;

		case TW2865CMD_GET_HUE:
			uiTmp1= tw2865_byte_read(TW2865B_I2C_ADDR, \
							0x06 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));			
			break;

		case TW2865CMD_GET_CONTRAST:
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
							0x02 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));	
			break;

		case TW2865CMD_GET_BRIGHTNESS:
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
							0x01 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));	
			break;

		case TW2865CMD_GET_SATURATION:
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
						0x04 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));	
			break;

		case TW2865CMD_AUDIO_OUTPUT:
			if(copy_from_user(&AudioOutput, argp, sizeof(AudioOutput)))
			{
				return -1;
			}
			/*check the channel number when output loop audio*/

			RegisterAudioOutput.PlaybackOrLoop = AudioOutput.PlaybackOrLoop;
			RegisterAudioOutput.channel = AudioOutput.channel;
	
			//if(RegisterAudioMute == 0)//demute
			//{
				Tw2865_SetAudioOutput(&AudioOutput);
		//	}			
			break;

		case TW2865CMD_AUDIO_MUTE:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			RegVal = tw2865_byte_read(Tw2865I2cAddr[0], 0xdc) & (~(0x01<<uiTmp1));			
			RegVal |= uiTmp1;
			tw2865_byte_write(Tw2865I2cAddr[0], 0xe0, 20);
			tw2865_byte_write(Tw2865I2cAddr[0], 0xdc, RegVal);			
			RegisterAudioMute = 1;
			break;
			
		case TW2865CMD_AUDIO_DEMUTE:
			RegVal = tw2865_byte_read(Tw2865I2cAddr[0], 0xdc) & 0xe0;
			tw2865_byte_write(Tw2865I2cAddr[0], 0xdc, RegVal);
			Tw2865_SetAudioOutput(&RegisterAudioOutput);
			RegisterAudioMute = 0;
			break;

		case TW2865CMD_SET_AUDIO_OUTPUT:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			if(uiTmp1<=0 || uiTmp1>16) {
				return -1;
			}
			set_audio_output(TW2865A_I2C_ADDR, uiTmp1);
			break;
		case TW2865CMD_SET_CHANNEL_SEQUENCE:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			if(uiTmp1>3) {
				return -1;
			}
			channel_alloc(TW2865A_I2C_ADDR, uiTmp1);
			break;
		case TW2865CMD_SET_ADA_BITWIDTH:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			set_ada_bitwidth(TW2865A_I2C_ADDR, uiTmp1);
			break;
		case TW2865CMD_SET_ADA_SAMPLERATE:/*设置音频输出的采样频率，通过设置0x70寄存器完成*/			
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))
			{
				return -1;
			}
			switch(uiTmp1)
			{
				case ASR_8K:
				default:
					ucTmp1 = 0x08;
					//ucTmp1 = 0x18;//dingyjun
					ucpTmp1 = AudioSetup8K;
					break;
				case ASR_16K:
					ucTmp1 = 0x08 | 0x01;
					//ucTmp1 = 0x18 | 0x01;
					ucpTmp1 = AudioSetup16K;
					break;
				case ASR_32K:
					ucTmp1 = 0x08 | 0x02;
					//ucTmp1 = 0x18 | 0x02;
					ucpTmp1 = AudioSetup32K;
					break;
				case ASR_44DOT1K:
					ucTmp1 = 0x08 | 0x03;
					//ucTmp1 = 0x18 | 0x03;
					ucpTmp1 = AudioSetup44_1K;
					break;
				case ASR_48K:
					ucTmp1 = 0x08 | 0x04;
					//ucTmp1 = 0x18 | 0x04;
					ucpTmp1 = AudioSetup48K;
					break;
			}
			tw2865_byte_write(TW2865A_I2C_ADDR, 0x70, ucTmp1);
			tw2865_write_table(TW2865A_I2C_ADDR, 0xf0, ucpTmp1, 6);
			break;
		case TW2865CMD_AUDIO_VOLUME:
		//	printk("TW2865CMD_AUDIO_VOLUME in driver\n");
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))
		    {
			    printk("tw2865_ERROR: WRONG cpy tmp is\n");
			    return -1;
		    }
			//printk("uiTmp1======= ====%d===\n", uiTmp1);
		    if(uiTmp1 > 15)
		    {
			    printk("ao output gain out of gain!\n");
			    return -1;
		    }
			//printk("before write\n");
		    RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xdf)&0x0f;
		    uiTmp1 = RegVal | ((uiTmp1 << 4) & 0xf0);
		    printk("write to 0xdf\n ");
		    tw2865_byte_write(TW2865A_I2C_ADDR,0xdf,uiTmp1);
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xdf);
			//printk("read from 0xdf=\n");

			return RegVal;
			break;	
		case TW2865CMD_VIDEO_IN_ON:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
			    printk("\ttw2865_ERROR: WRONG cpy tmp is %x\n",uiTmp1);
			    return -1;
		    }
		    if(uiTmp1<0 || uiTmp1>3) {
				return -1;
			}
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
			uiTmp1 = RegVal & (~(0x01<<uiTmp1));
			tw2865_byte_write(TW2865A_I2C_ADDR,0xce,uiTmp1);
			break;
		case TW2865CMD_VIDEO_IN_OFF:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
			    printk("\ttw2865_ERROR: WRONG cpy tmp is %x\n",uiTmp1);
			    return -1;
		    }
		    if(uiTmp1<0 || uiTmp1>3) {
				return -1;
			}
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
			uiTmp1 = RegVal | (0x01<<uiTmp1);
			tw2865_byte_write(TW2865A_I2C_ADDR,0xce,uiTmp1);
			break;
		case TW2865CMD_VIDEO_OUT_ON:
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0x4b);
			uiTmp1 = RegVal & (~(0x03<<4));
			tw2865_byte_write(TW2865A_I2C_ADDR,0x4b,uiTmp1);
			break;
		case TW2865CMD_VIDEO_OUT_OFF:
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0x4b);
			uiTmp1 = RegVal | (0x03<<4);
			tw2865_byte_write(TW2865A_I2C_ADDR,0x4b,uiTmp1);
			break;	
		
		case TW2865CMD_AUDIO_IN_ON:
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
			uiTmp1 = RegVal & (~(0x01<<4));
			tw2865_byte_write(TW2865A_I2C_ADDR,0xce,uiTmp1);
			break;
		case TW2865CMD_AUDIO_IN_OFF:
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
			uiTmp1 = RegVal | (0x01<<4);
			tw2865_byte_write(TW2865A_I2C_ADDR,0xce,uiTmp1);
			break;
		case TW2865CMD_AUDIO_OUT_ON:
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
			uiTmp1 = RegVal & (~(0x01<<5));
			tw2865_byte_write(TW2865A_I2C_ADDR,0xce,uiTmp1);
			break;
		case TW2865CMD_AUDIO_OUT_OFF:
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
			uiTmp1 = RegVal | (0x01<<5);
			tw2865_byte_write(TW2865A_I2C_ADDR,0xce,uiTmp1);
			break;
		case TW2865CMD_VIDEO_OUT:
			if(copy_from_user(&VideoOutput, argp, sizeof(VideoOutput)))
			{
				return -1;
			}
			if(VideoOutput.PlaybackOrLoop == 0) {
				RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0x4b) & (~(0x01<<2));
				tw2865_byte_write(TW2865A_I2C_ADDR,0x4b,RegVal);
			} else if(VideoOutput.PlaybackOrLoop == 1) {
				RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0x4b) | (0x01<<2);
				RegVal |= VideoOutput.channel;
				tw2865_byte_write(TW2865A_I2C_ADDR,0x4b,RegVal);
			}
			break;
		case TW2865CMD_SET_AIGAIN:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				    printk("\ttw2865_ERROR: WRONG cpy tmp is %x\n",uiTmp1);
				    return -1;
			    }
			    if(uiTmp1<0 || uiTmp1>15) {
				return -1;
			}
			uiTmp1 = uiTmp1 | ((uiTmp1 & 0x0f) <<4);
			tw2865_byte_write(TW2865A_I2C_ADDR,0xd0,uiTmp1);
			tw2865_byte_write(TW2865A_I2C_ADDR,0xd1,uiTmp1);
			tw2865_byte_write(TW2865A_I2C_ADDR,0x7f,uiTmp1);
			break;
		default:
			break;
	}
	
	return 0;
}
/*
int tw2865b_open(struct inode * inode, struct file * file)
{
	return 0;
} 

int tw2865b_close(struct inode * inode, struct file * file)
{
	return 0;
}

int tw2865b_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int __user *argp = (unsigned int __user *)arg;
	unsigned int uiTmp1;
	unsigned int tmp;
	int j;
	unsigned char *ucpTmp1;
	unsigned long ulTmp1;
	unsigned char ucTmp1;
	unsigned char RegVal;
	tw2865_set_audiooutput AudioOutput;

    	tw2865_w_reg tw2865reg; 
    	tw2865_set_2d1 tw2865_2d1;
   	 tw2865_set_videomode tw2865_videomode;
	tw2865_set_analogsetting AnalogSetting;
 	tw2865_set_1d1 tw2865_d1;

	switch(cmd)
	{
		case TW2865CMD_READ_REG:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))
			{
				return -1;
			}			

			tmp = tw2865_byte_read(TW2865B_I2C_ADDR, (unsigned char)uiTmp1);
			copy_to_user(argp, &tmp, sizeof(tmp));
			break;		
			
		case TW2865CMD_READ_REGT:
			if(copy_from_user(&tw2865reg, argp, sizeof(tw2865reg)))
			{
				return -1;
			}			
			tw2865_read_table(TW2865B_I2C_ADDR, tw2865reg.addr, tw2865reg.value);
			break;
			
		case TW2865CMD_WRITE_REG:
			if(copy_from_user(&tw2865reg, argp, sizeof(tw2865reg)))
			{
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, tw2865reg.addr, tw2865reg.value);		
			break;

		case TW2865CMD_SET_1_D1:
			if(copy_from_user(&tw2865_d1, argp, sizeof(tw2865_d1))) {
				return -1;
			}
			setd1(TW2865A_I2C_ADDR, tw2865_d1.vdn, tw2865_d1.ch);
			break;
			break;
		case TW2865CMD_SET_2_D1:
			if(copy_from_user(&tw2865_2d1, argp, sizeof(tw2865_2d1))) {
				return -1;
			}
			set_2_d1(TW2865B_I2C_ADDR, tw2865_2d1.vdn,tw2865_2d1.ch1, tw2865_2d1.ch2);
			break;
		case TW2865CMD_SET_4_CIF:
			if(copy_from_user(&tmp, argp, sizeof(tmp))) {
				return -1;
			}
			set_4_cif(TW2865B_I2C_ADDR, tmp);
			break;
		case TW2865CMD_SET_4_D1:
			if(copy_from_user(&tmp, argp, sizeof(tmp))) {
				return -1;
			}
			set_4_d1(TW2865B_I2C_ADDR, tmp);
			break;
		case TW2865CMD_SET_VIDEO_MODE:
			if(copy_from_user(&tw2865_videomode, argp, sizeof(tw2865_videomode))) {
				return -1;
			}
			Tw2865SetNormalRegister(TW2865B_I2C_ADDR, tw2865_videomode.mode, tw2865_videomode.ch);
			break;
		case TW2865CMD_GET_VL:
			ulTmp1 = 0;
			for(j = 0; j < 4; j ++)
			{
				ucTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR,  j * 0x10);
				ulTmp1 |= ((((unsigned long)(ucTmp1 >> 7)) & 0x01) << (j));
			}				
		
			copy_to_user(argp, &ulTmp1, sizeof(ulTmp1));
			break;

		case TW2865CMD_SET_ANALOG:
			if(copy_from_user(&AnalogSetting, argp, sizeof(AnalogSetting)))
			{
				return -1;
			}
			if(AnalogSetting.SettingBitmap & 0x01)//hue
			{
				tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x06 + AnalogSetting.ch * 0x10, AnalogSetting.hue);
			}
			if(AnalogSetting.SettingBitmap & 0x02)//contrast
			{
				tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x02 + AnalogSetting.ch * 0x10, AnalogSetting.contrast);
			}
			if(AnalogSetting.SettingBitmap & 0x04)//brightness
			{
				tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x01 + AnalogSetting.ch * 0x10, AnalogSetting.brightness);
			}
			if(AnalogSetting.SettingBitmap & 0x08)//saturation
			{
				tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x04 + AnalogSetting.ch * 0x10, AnalogSetting.saturation);
				tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x05 + AnalogSetting.ch * 0x10, AnalogSetting.saturation);
			}
			break;

		case TW2865CMD_SET_HUE :
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x06 + AnalogSetting.ch * 0x10, uiTmp1);
			break;

		case TW2865CMD_SET_CONTRAST:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x02 + AnalogSetting.ch * 0x10, uiTmp1);
			break;

		case TW2865CMD_SET_BRIGHTNESS:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
						0x01 + AnalogSetting.ch * 0x10, uiTmp1);
			break;

		case TW2865CMD_SET_SATURATION:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, \
					0x04 + AnalogSetting.ch * 0x10, uiTmp1);
			tw2865_byte_write(TW2865B_I2C_ADDR, \
					0x05 + AnalogSetting.ch * 0x10, uiTmp1);
			break;
		case TW2865CMD_GET_ANALOG:
			if(copy_from_user(&AnalogSetting, argp, sizeof(AnalogSetting)))
			{
				return -1;
			}
			if(AnalogSetting.SettingBitmap & 0x01)//hue
			{
				AnalogSetting.hue = tw2865_byte_read(TW2865B_I2C_ADDR, \
								0x06 + AnalogSetting.ch * 0x10);
			}
			if(AnalogSetting.SettingBitmap & 0x02)//contrast
			{
				AnalogSetting.contrast = tw2865_byte_read(TW2865B_I2C_ADDR, \
								0x02 + AnalogSetting.ch * 0x10);
			}
			if(AnalogSetting.SettingBitmap & 0x04)//brightness
			{
				AnalogSetting.brightness = tw2865_byte_read(TW2865B_I2C_ADDR, \
								0x01 + AnalogSetting.ch * 0x10);
			}
			if(AnalogSetting.SettingBitmap & 0x08)//saturation
			{
				AnalogSetting.saturation = tw2865_byte_read(TW2865B_I2C_ADDR, \
								0x04 + AnalogSetting.ch * 0x10);
			}
			copy_to_user(argp, &AnalogSetting, sizeof(AnalogSetting));
			break;

		case TW2865CMD_GET_HUE :
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
							0x06 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));			
			break;

		case TW2865CMD_GET_CONTRAST:
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
							0x02 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));	
			break;

		case TW2865CMD_GET_BRIGHTNESS:
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
							0x01 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));	
			break;

		case TW2865CMD_GET_SATURATION:
			uiTmp1 = tw2865_byte_read(TW2865B_I2C_ADDR, \
						0x04 + AnalogSetting.ch * 0x10);
			copy_to_user(argp, &uiTmp1, sizeof(uiTmp1));	
			break;
			
		case TW2865CMD_AUDIO_OUTPUT:
			if(copy_from_user(&AudioOutput, argp, sizeof(AudioOutput)))
			{
				return -1;
			}

			RegisterAudioOutput.PlaybackOrLoop = AudioOutput.PlaybackOrLoop;
			RegisterAudioOutput.channel = AudioOutput.channel;
	
			//if(RegisterAudioMute == 0)//demute
		//	{
				Tw2865_SetAudioOutput(&AudioOutput);
		//	}			
			break;

		case TW2865CMD_AUDIO_MUTE:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			RegVal = tw2865_byte_read(Tw2865I2cAddr[0], 0xdc) & (~(0x01<<uiTmp1));			
			RegVal |= uiTmp1;	
			tw2865_byte_write(Tw2865I2cAddr[1], 0xe0, 20);
			tw2865_byte_write(Tw2865I2cAddr[1], 0xdc, RegVal);			
			RegisterAudioMute = 1;
			break;
			
		case TW2865CMD_AUDIO_DEMUTE:
			RegVal = tw2865_byte_read(Tw2865I2cAddr[1], 0xdc) & 0xe0;
			tw2865_byte_write(Tw2865I2cAddr[1], 0xdc, RegVal);
			Tw2865_SetAudioOutput(&RegisterAudioOutput);
			RegisterAudioMute = 0;
			break;

		case TW2865CMD_SET_AUDIO_OUTPUT:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			if(uiTmp1<=0 || uiTmp1>16) {
				return -1;
			}
			set_audio_output(TW2865B_I2C_ADDR, uiTmp1);
			break;
		case TW2865CMD_SET_CHANNEL_SEQUENCE:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			if(uiTmp1>3) {
				return -1;
			}
			channel_alloc(TW2865B_I2C_ADDR, uiTmp1);
			break;
		case TW2865CMD_SET_ADA_BITWIDTH:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			set_ada_bitwidth(TW2865B_I2C_ADDR, uiTmp1);
			break;
		case TW2865CMD_SET_ADA_SAMPLERATE:		
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))
			{
				return -1;
			}
			switch(uiTmp1)
			{
				case ASR_8K:
				default:
					//ucTmp1 = 0x08;
					ucTmp1 = 0x18;//dingyjun
					ucpTmp1 = AudioSetup8K;
					break;
				case ASR_16K:
					//ucTmp1 = 0x08 | 0x01;
					ucTmp1 = 0x18 | 0x01;
					ucpTmp1 = AudioSetup16K;
					break;
				case ASR_32K:
					//ucTmp1 = 0x08 | 0x02;
					ucTmp1 = 0x18 | 0x02;
					ucpTmp1 = AudioSetup32K;
					break;
				case ASR_44DOT1K:
					//ucTmp1 = 0x08 | 0x03;
					ucTmp1 = 0x18 | 0x03;
					ucpTmp1 = AudioSetup44_1K;
					break;
				case ASR_48K:
					//ucTmp1 = 0x08 | 0x04;
					ucTmp1 = 0x18 | 0x04;
					ucpTmp1 = AudioSetup48K;
					break;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, 0x70, ucTmp1);
			tw2865_write_table(TW2865B_I2C_ADDR, 0xf0, ucpTmp1, 6);
			break;
		case TW2865CMD_AUDIO_VOLUME:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))
		    {
			    printk("\ttw2865_ERROR: WRONG cpy tmp is %x\n",uiTmp1);
			    return -1;
		    }

		    if(uiTmp1 > 15)
		    {
			    printk("ao output gain out of gain!\n");
			    return -1;
		    }
		    RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xdf)&0x0f;
		    uiTmp1 = RegVal | ((uiTmp1 << 4) & 0xf0);
		 //  printk("0xdf=%x\n", uiTmp1);
		    tw2865_byte_write(TW2865A_I2C_ADDR,0xdf,uiTmp1);
			RegVal = tw2865_byte_read(TW2865A_I2C_ADDR,0xdf);
		//	printk("0xdf=%x\n", RegVal);

			break;	
		default:
			break;
	}
	
	return 0;
}
*/

static void Tw2865SetNormalRegister(unsigned char addr, 
						unsigned char mode, unsigned char chn)
{
	if(mode == TW2865_NTSC)
	{
		tw2865_write_table(addr, 0x10 * chn, tbl_ntsc_tw2865_common, 16);
	}
	else
	{
		tw2865_write_table(addr, 0x10 * chn, tbl_pal_tw2865_common, 16);
	}
}
static int  Tw2865Init(unsigned char addr, unsigned char mode)
{
	unsigned char RegValue,i;

	tw2865_byte_write(addr,0x80,0x3f);
    	udelay(50);

	Tw2865SetNormalRegister(addr, mode, 0);
	Tw2865SetNormalRegister(addr, mode, 1);
	Tw2865SetNormalRegister(addr, mode, 2);
	Tw2865SetNormalRegister(addr, mode, 3);

	if(mode == TW2865_PAL)
	{
		tw2865_byte_write(addr, 0x41, 0xd4);
	}
	else
	{
		tw2865_byte_write(addr, 0x41, 0x40);
	}

	tw2865_byte_write(addr,0x43,0x08);
	tw2865_byte_write(addr,0x44,0x00);
	tw2865_byte_write(addr,0x9c,0x20);
	tw2865_byte_write(addr,0x60,0x10);
	tw2865_byte_write(addr,0x61,0x03);
	tw2865_byte_write(addr,0x96,0xe0);
	tw2865_byte_write(addr,0x97,0x05);

	/*CLKPO,CLKPN, 4*/

	tw2865_byte_write(addr, 0xfa, 0x45);//54mhz
	tw2865_byte_write(addr, 0x6a, 0x05);//54mhz
	tw2865_byte_write(addr, 0x6b, 0x05);//54mhz
	tw2865_byte_write(addr, 0x6c, 0x05);//54mhz

	
	/*CHID*/
	tw2865_byte_write(addr, 0x9e, 0x52);
	/*2nd Channel Selection */
	tw2865_byte_write(addr, 0xcc, 0x07);
	/*1st Channel Selection*/
	tw2865_byte_write(addr, 0xcd, 0x02);
	/*Video Channel Output Control 2D1*/
	tw2865_byte_write(addr, 0xca, 0x55);
	
	tw2865_write_table(addr, 0xd0, InitAudioSetup, 20);
	tw2865_byte_write(addr, 0xdb, 0xd1);//
	tw2865_byte_write(addr, 0x73, 0x01);
	tw2865_byte_write(addr, 0xd2, 0x01);//4 audios
	tw2865_byte_write(addr, 0xdd, 0x00);
	tw2865_byte_write(addr, 0xde, 0x00);
	tw2865_byte_write(addr, 0xf8, 0xc4);
	tw2865_byte_write(addr, 0xf9, 0x51);
	tw2865_byte_write(addr, 0x70, 0x08);
	tw2865_byte_write(addr, 0x7f, 0x80);//ain5gain=1.0
	tw2865_byte_write(addr, 0xcf, 0x00);//not cascade
	tw2865_byte_write(addr, 0xd0,0x88 );//input gain(2.1)=8(1.0 default)
	tw2865_byte_write(addr, 0xd1,0x88 );//input gain(4.3)=8(1.0 default)
	RegValue = tw2865_byte_read(Tw2865I2cAddr[0], 0xe0) & 0xe0;
	RegValue |= 16;
	tw2865_byte_write(Tw2865I2cAddr[0], 0xe0, RegValue);
	tw2865_byte_write(addr, 0xdf,0xf0 );
	if(mode == TW2865_PAL)
	{
		tw2865_write_table(addr, 0xf0, tbl_pal_tw2865_8KHz, 6);

	}
	else
	{
		tw2865_write_table(addr, 0xf0, tbl_ntsc_tw2865_8KHz, 6);		
	}

	//16bitwiath
	tw2865_byte_write(addr, 0xd3, 0x10);
	tw2865_byte_write(addr, 0xd4, 0x54);
	tw2865_byte_write(addr, 0xd5, 0x98);
	tw2865_byte_write(addr, 0xd6, 0xdc);
	tw2865_byte_write(addr, 0xd7, 0x32);
	tw2865_byte_write(addr, 0xd8, 0x76);
	tw2865_byte_write(addr, 0xd9, 0xba);
	tw2865_byte_write(addr, 0xda, 0xfe);
	
	for(i = 0; i < 8; i++) {
			tw2865_byte_write(addr,0xd3+i,0);
			tw2865_byte_write(addr,0xd3+i,channel_array_4chn[i]);
		}
	//tw2865_reg_dump(addr);
	//video dac off
	RegValue = tw2865_byte_read(TW2865A_I2C_ADDR,0x4b);
	RegValue  = RegValue | (0x03<<4);
	tw2865_byte_write(TW2865A_I2C_ADDR,0x4b,RegValue );
	//audio dac off
	RegValue = tw2865_byte_read(TW2865A_I2C_ADDR,0xce);
	RegValue = RegValue | (0x01<<5);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xce,RegValue);
	return 0;
}


static struct file_operations tw2865a_fops = {
	.owner      = THIS_MODULE,
	.ioctl      	  = tw2865a_ioctl,
	.open        = tw2865a_open,
	.release     = tw2865a_close
};
static struct miscdevice tw2865a_dev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "tw2865adev",
	.fops  		= &tw2865a_fops,
};
/*
static struct file_operations tw2865b_fops = {
	.owner      = THIS_MODULE,
	.ioctl      	  = tw2865b_ioctl,
	.open        = tw2865b_open,
	.release     = tw2865b_close
};

static struct miscdevice tw2865b_dev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "tw2865bdev",
	.fops  		= &tw2865b_fops,
};*/	
static int TW2865a_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *client;
	int ret;
//PRINTK_TW2815("inter tw2815 i2c probe\n");
	client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client) {
		return -ENOMEM;
    }
	memset(client, 0, sizeof(struct i2c_client));
	strncpy(client->name, TW2865a_DRIVER_NAME, I2C_NAME_SIZE);
	client->flags   = I2C_DF_NOTIFY;
	client->addr    = addr;
	client->adapter = adap;
	client->driver  = &tw2865a_driver;

	if ((ret = i2c_attach_client(client)) != 0) 
		{
	    kfree(client);
		} 
	else 
		{
	    save_client = client;
//      misc_register(&TW2815_dev);
    }
    return ret;
}

static int TW2865a_attach(struct i2c_adapter *adap)
{
//	PRINTK_TW2815("inter tw2815 i2c attach\n");
	return i2c_probe(adap, &addr_dataa, TW2865a_probe);
}

static int TW2865a_detach(struct i2c_client *client)
{
	int	ret = -ENODEV;

	if (client->adapter) 
		{
	    ret = i2c_detach_client(client);
	    if (!ret) 
	    	{
	        kfree(client);
	        client->adapter = NULL;
	    	}
		}
	return ret;
}

static struct i2c_driver tw2865a_driver = {
	.owner		    	= THIS_MODULE,
	.name		    	= TW2865a_DRIVER_NAME,
	.id		        	= I2C_DRIVERID_TW2865a,
	.flags		    	= I2C_DF_NOTIFY,
	.attach_adapter		= TW2865a_attach,
	.detach_client		= TW2865a_detach,
};

module_param(norm, uint, S_IRUGO);
module_param(chips, uint, S_IRUGO);


//DECLARE_KCOM_GPIO_I2C();
static int __init tw2865_init(void)
{
	int ret = 0;

    i2c_add_driver(&tw2865a_driver);

	/*first check the parameters*/
/*	if((norm != TW2865_PAL) && (norm != TW2865_NTSC))
	{
		printk("module param norm must be PAL(%d) or NTSC(%d)\n", TW2865_PAL ,TW2865_NTSC);
		return -1;
	}
	if(chips > 4)
	{
		printk("tw2865_init invalid chip number(%d)\n", chips);
	}*/

	/*get the i2c module instance*/
	//ret = KCOM_GPIO_I2C_INIT();
	if(ret)
	{
		printk("GPIO I2C module is not load.\n");
		return -1;
	}
	/*register misc device*/
	ret = misc_register(&tw2865a_dev);
	if (ret)
	{
		printk("ERROR: could not register tw2865 devices\n");
		goto FAILED;
	}
	/*initize each tw2865*/		
	Tw2865Init(Tw2865I2cAddr[0], norm);
	Tw2865_SetAudioOutput(&RegisterAudioOutput);
	return 0;

FAILED:
	misc_deregister(&tw2865a_dev);
	i2c_del_driver(&tw2865a_driver);
	//KCOM_GPIO_I2C_EXIT();
	return ret;
}



static void __exit tw2865_exit(void)
{
	   i2c_del_driver(&tw2865a_driver);

	misc_deregister(&tw2865a_dev);
	//KCOM_GPIO_I2C_EXIT();
}

module_init(tw2865_init);
module_exit(tw2865_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");

