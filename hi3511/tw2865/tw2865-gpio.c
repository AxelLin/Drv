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
#include <kcom/gpio_i2c.h>
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


tw2865_set_audiooutput RegisterAudioOutput = {
	.PlaybackOrLoop = 1,
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
static unsigned char tw2865_byte_write(unsigned char chip_addr,
										unsigned char addr, unsigned char data) 
{
	gpio_i2c_write(chip_addr, addr, data); 
	
	return 0;
}

static unsigned char tw2865_byte_read(unsigned char chip_addr, unsigned char addr)
{
	return gpio_i2c_read(chip_addr, addr);   
}


static void tw2865_write_table(unsigned char chip_addr,
		unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
	unsigned char i = 0;
	
	for(i = 0; i < tbl_cnt; i ++)
	{
		gpio_i2c_write(chip_addr, (addr + i), *(tbl_ptr + i));
	}
}


static void tw2865_read_table(unsigned char chip_addr,
						unsigned char addr, unsigned char reg_num)
{
	unsigned char i = 0,temp = 0;
	for(i =0; i < reg_num;i++ )
	{
		temp = tw2865_byte_read(chip_addr,addr+i);
		printk("reg 0x%x=0x%x,",addr+i,temp);
		if(((i+1)%4)==0)
		printk("\n");
		
	}
}
static void set_audio_cascad(void)
{
	//unsigned int tmp1,tmp2;
	unsigned char tmp;
	//set the two audio chips cascade
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xcf);
	SET_BIT(tmp, 0x80);
	CLEAR_BIT(tmp, 0x40);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xcf,tmp);
	tw2865_byte_write(TW2865B_I2C_ADDR,0xcf,tmp);
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
	tmp = tw2865_byte_read(TW2865B_I2C_ADDR,0xd2);
	CLEAR_BIT(tmp,0x30);
 	SET_BIT(tmp, 0x01);
 	tw2865_byte_write(TW2865B_I2C_ADDR,0xd2,tmp);	
#endif
	//enanble audio adc and dac
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xdb);
	SET_BIT(tmp, 0xc0);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xdb,tmp);
	//for 0x52
	tmp = tw2865_byte_read(TW2865B_I2C_ADDR,0xdb);
	SET_BIT(tmp, 0xc0);
	tw2865_byte_write(TW2865B_I2C_ADDR,0xdb,tmp);	
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
	printk("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=%d\n",tmp);
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
	tmp = tw2865_byte_read(TW2865B_I2C_ADDR,0xd2);
	CLEAR_BIT(tmp,0x30);
 	SET_BIT(tmp, 0x01);
 	tw2865_byte_write(TW2865B_I2C_ADDR,0xd2,tmp);	
#endif
	//enanble audio adc and dac
	tmp = tw2865_byte_read(TW2865A_I2C_ADDR,0xdb);
	SET_BIT(tmp, 0xc0);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xdb,tmp);

	tw2865_byte_write(TW2865A_I2C_ADDR,0xf3,0x00);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xf4,0x01);
	tw2865_byte_write(TW2865A_I2C_ADDR,0xf5,0x00);

}

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
    	printk("tw2865 set output path num=%d  ok\n",num_path);	

}
static void set_ada_bitwidth(unsigned char chip_addr, unsigned char bitwidth)
{
	unsigned int ada_bitwidth = 0;
	if(bitwidth == SET_8_BITWIDTH) {
		ada_bitwidth = tw2865_byte_read(chip_addr, 0xdb)& 0xfb;
		ada_bitwidth |= 0x04;
		tw2865_byte_write(chip_addr, 0xdb, ada_bitwidth);
		set_audio_cascad();
		
	} else if(bitwidth == SET_16_BITWIDTH) {
		ada_bitwidth = tw2865_byte_read(chip_addr, 0xdb)& 0xfb;
		tw2865_byte_write(chip_addr, 0xdb, ada_bitwidth);
		set_single_chip();
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
static void setd1(unsigned char chip_addr)
{
	unsigned char tmp=0;
	tmp = tw2865_byte_read(chip_addr, 0xcb);
	tmp &= 0xf0; 
	tw2865_byte_write(chip_addr, 0xcb, tmp);
	tw2865_byte_write(chip_addr, 0xca, 0x00);
	tmp = tw2865_byte_read(chip_addr, 0xfa);
	tmp &= 0xf0; 
	tw2865_byte_write(chip_addr, 0xfa, tmp);
	tmp = tw2865_byte_read(chip_addr, 0x6a);
	tmp &= 0xf0; 
	tw2865_byte_write(chip_addr, 0x6a, tmp);
	tmp = tw2865_byte_read(chip_addr, 0x6b);
	tmp &= 0xf0; 
	tw2865_byte_write(chip_addr, 0x6b, tmp);
	tmp = tw2865_byte_read(chip_addr, 0x6c);
	tmp &= 0xf0; 
	tw2865_byte_write(chip_addr, 0x6c, tmp);
}

static void set_2_d1(unsigned char chip_addr, unsigned char vdn, unsigned char ch1, unsigned char ch2)
{
	unsigned char tmp=0,temp;
//	tw2865_byte_write(chip_addr, 0xcb, 0x88);
/*	vdn -= 1;
	printk("vdn=%d\n", vdn);
	if(vdn == 0) {
	//	tmp = tw2865_byte_read(chip_addr, 0xfa) & 0xf0;
		tmp |= 0xf5; 
	//	tw2865_byte_write(chip_addr, 0xfa, tmp);
	} else if(vdn == 1) {
	//	tmp = tw2865_byte_read(chip_addr, 0x6a) & 0xf0;
		tmp |= 0xf5; 
	//	tw2865_byte_write(chip_addr, 0x6a, tmp);
	} else if(vdn == 2) {
		tmp = tw2865_byte_read(chip_addr, 0x6b);
		tmp |= 0xf5; 
		tw2865_byte_write(chip_addr, 0x6b, tmp);
	} else if(vdn == 3) {
		tmp = tw2865_byte_read(chip_addr, 0x6c);
		tmp |= 0xf5; 
		tw2865_byte_write(chip_addr, 0x6c, tmp);
	} 
	tw2865_byte_write(chip_addr, 0xcb, 0);

//	tmp = tw2865_byte_read(chip_addr, 0xca);
	tmp = tmp | (0x01<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xca, tmp);

//	tmp = tw2865_byte_read(chip_addr, 0xcd);
//	tmp = (tmp & (~(0x03<<vdn*2))) |(ch2<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xcd, 0x00);
	printk("0xcd=%x\n", tmp);
//	tmp = tw2865_byte_read(chip_addr, 0xcc);
//	tmp = (tmp & (~(0x03<<vdn*2))) |(ch1<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xcc, 0x00);
	printk("0xcc=%x\n", tmp);*/
	tw2865_byte_write(chip_addr, 0xcc, 0x0e);
	/*1st Channel Selection*/
	tw2865_byte_write(chip_addr, 0xcd, 0x04);

}

static void set_4_cif(unsigned char chip_addr, unsigned char vdn)
{
	unsigned char tmp=0;
	if(vdn == 1) {
		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0xf0;
		tmp |= 0xf5; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);

		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0x8f;
		tmp |= 0x70; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);
	} else if(vdn == 2) {
		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0xf0;
		tmp |= 0xf5; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);

		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0x8f;
		tmp |= 0x70; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);
	}
	tmp = tw2865_byte_read(chip_addr, 0xca);
	tmp = tmp | (0x02<<vdn*2); 
	tw2865_byte_write(chip_addr, 0xca, tmp);

	tmp = tw2865_byte_read(chip_addr, 0xcb);
	tmp = tmp | (0x01<<vdn); 
	tw2865_byte_write(chip_addr, 0xcb, tmp);
}

static void set_4_d1(unsigned char chip_addr, unsigned char vdn)
{
	unsigned char tmp=0;
	if(vdn == 1) {
		tmp = tw2865_byte_read(chip_addr, 0xfa) & 0xf0;
		tmp |= 0x0a; 
		tw2865_byte_write(chip_addr, 0xfa, tmp);

		tmp = tw2865_byte_read(chip_addr, 0x6a) & 0x8f;
		tmp |= 0x70; 
		tw2865_byte_write(chip_addr, 0x6a, tmp);
	} else if(vdn == 2) {
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
	tw2865_byte_write(Tw2865I2cAddr[1], 0xcf, 0x80);
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

    	tw2865_w_reg tw2865reg; 
    	tw2865_set_2d1 tw2865_2d1;
   	 tw2865_set_videomode tw2865_videomode;
	tw2865_set_analogsetting AnalogSetting;
 

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

		case TW2865CMD_SET_D1:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			setd1(TW2865A_I2C_ADDR);
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
	
			if(RegisterAudioMute == 0)//demute
			{
				Tw2865_SetAudioOutput(&AudioOutput);
			}			
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
			tw2865_byte_write(TW2865A_I2C_ADDR, 0x70, ucTmp1);
			tw2865_write_table(TW2865A_I2C_ADDR, 0xf0, ucpTmp1, 6);
			break;
		case TW2865CMD_AUDIO_VOLUME:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))
		    {
			    printk("\ttw2865_ERROR: WRONG cpy tmp is %x\n",uiTmp1);
			    return -1;
		    }

		   // if(uiTmp1 > 15)
		    //{
			  //  printk("ao output gain out of gain!\n");
			    //return -1;
		   // }
			//printk("ao111111111111 output gain out of gain = %x!\n",uiTmp1);
		    //uiTmp1 = tw2865_byte_read(TW2865A_I2C_ADDR,0xdf)&0x0f;
		    //uiTmp1 += uiTmp1 << 4;
			//printk("\n ao2222222222222 output gain out of gain = %x!\n",uiTmp1);
		    tw2865_byte_write(TW2865A_I2C_ADDR,0xdf,uiTmp1);

			break;	
		default:
			break;
	}
	
	return 0;
}

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
 

	switch(cmd)
	{
		case TW2865CMD_READ_REG:/*read register value*/
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1)))/*get the register address*/
			{
				return -1;
			}			

			tmp = tw2865_byte_read(TW2865B_I2C_ADDR, (unsigned char)uiTmp1);/*read register*/
			copy_to_user(argp, &tmp, sizeof(tmp));/*return the register value*/
			break;		
			
		case TW2865CMD_READ_REGT:/*read register table*/
			if(copy_from_user(&tw2865reg, argp, sizeof(tw2865reg)))/*get the register address*/
			{
				return -1;
			}			
			tw2865_read_table(TW2865B_I2C_ADDR, tw2865reg.addr, tw2865reg.value);/*read register*/
			break;
			
		case TW2865CMD_WRITE_REG:/*write */
			if(copy_from_user(&tw2865reg, argp, sizeof(tw2865reg)))/*get the register address*/
			{
				return -1;
			}
			tw2865_byte_write(TW2865B_I2C_ADDR, tw2865reg.addr, tw2865reg.value);/*write register*/			
			break;

		case TW2865CMD_SET_D1:
			if(copy_from_user(&uiTmp1, argp, sizeof(uiTmp1))) {
				return -1;
			}
			setd1(TW2865B_I2C_ADDR);
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
		case TW2865CMD_GET_VL:/*get video loss state for all channels*/
			ulTmp1 = 0;
			for(j = 0; j < 4; j ++)/*every channel in a chip*/
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
			/*check the channel number when output loop audio*/

			RegisterAudioOutput.PlaybackOrLoop = AudioOutput.PlaybackOrLoop;
			RegisterAudioOutput.channel = AudioOutput.channel;
	
			if(RegisterAudioMute == 0)//demute
			{
				Tw2865_SetAudioOutput(&AudioOutput);
			}			
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
		case TW2865CMD_SET_ADA_SAMPLERATE:/*设置音频输出的采样频率，通过设置0x70寄存器完成*/			
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

		   // if(uiTmp1 > 15)
		    //{
			  //  printk("ao output gain out of gain!\n");
			    //return -1;
		   // }
			//printk("ao111111111111 output gain out of gain = %x!\n",uiTmp1);
		    //uiTmp1 = tw2865_byte_read(TW2865A_I2C_ADDR,0xdf)&0x0f;
		    //uiTmp1 += uiTmp1 << 4;
			//printk("\n ao2222222222222 output gain out of gain = %x!\n",uiTmp1);
		    tw2865_byte_write(TW2865B_I2C_ADDR,0xdf,uiTmp1);

			break;	
		default:
			break;
	}
	
	return 0;
}


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
	unsigned char temp = 0;
	unsigned char temp1 = 0;
	unsigned char RegValue;
//******************************************
//0x80:software reset control register
	tw2865_byte_write(addr,0x80,0x3f);
    	udelay(50);
//******************************************
//******************************************
//0xfe,0xff;device ID and revision ID flag
//the following code check whether the 2865_device and the status of i2c are valid or not.
/*	temp = tw2865_byte_read(addr,0xfe); 		
	CLEAR_BIT(temp,0x3f);
	temp = temp >> 1;
	printk("tw2865 init failed .temp is :%0x\n",temp);

	temp1 = tw2865_byte_read(addr,0xff);
	temp1 = temp1 >> 3;
	printk("tw2865 init failed .tmp1 is :%0x\n",temp1);
	temp = temp + temp1;

	if(temp != 0x18){
		printk("tw2865 init failed .temp is :%0x\n",temp);
		return -1;
	}*/
//******************************************
	Tw2865SetNormalRegister(addr, mode, 0);
	Tw2865SetNormalRegister(addr, mode, 1);
	Tw2865SetNormalRegister(addr, mode, 2);
	Tw2865SetNormalRegister(addr, mode, 3);
//***********************************************
//this part is for video encoder
	/*制式*/
	//encoder,
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
//***********************************************
/*
	temp = tw2865_byte_read(addr,0x9f);
   	CLEAR_BIT(temp,0x33);
    	SET_BIT(temp,0x88);
    	tw2865_byte_write(addr,0x9f,temp);
*/
	tw2865_byte_write(addr,0x9c,0x20);

	tw2865_byte_write(addr,0x60,0x10);
	tw2865_byte_write(addr,0x61,0x03);
	
	tw2865_byte_write(addr,0x96,0xe0);
	tw2865_byte_write(addr,0x97,0x05);

	/*CLKPO,CLKPN, 4*/

	tw2865_byte_write(addr, 0xfa, 0x45);//27mhz
/*	temp = 0;
	temp = tw2865_byte_read(addr, 0xfa);
	printk("tw2865 init failed .0xfatemp-- is :%0x\n",temp);*/

	//temp = tw2865_byte_read(addr,0xfb);
	//SET_BIT(temp,0x6f);
	//tw2865_byte_write(addr,0xfb,0x6f);

	tw2865_byte_write(addr, 0x6a, 0x05);//27mhz
/*	temp = 0;
	temp = tw2865_byte_read(addr, 0x6a);
	printk("tw2865 init failed .0x6atemp-- is :%0x\n",temp);*/

	tw2865_byte_write(addr, 0x6b, 0x05);//27mhz
/*	temp = 0;
	temp = tw2865_byte_read(addr, 0x6b);
	printk("tw2865 init failed .0x6btemp-- is :%0x\n",temp);*/

	tw2865_byte_write(addr, 0x6c, 0x05);//27mhz
	temp = 0;
	temp = tw2865_byte_read(addr, 0x07);
	printk("tw2865 init failed .0x6ctemp-- is :%0x\n",temp);
	mdelay(1000);
//	tw2865_byte_write(addr, 0x6c, 0x05);//27mhz
	temp = 0;
	temp = tw2865_byte_read(addr, 0x6c);
	printk("tw2865 init failed .0x6ctemp-- is :%0x\n",temp);
	mdelay(1000);
	temp = 0;
	temp = tw2865_byte_read(addr, 0x07);
	printk("tw2865 init failed .0x07temp-- is :%0x\n",temp);
	mdelay(1000);
	temp = 0;
	temp = tw2865_byte_read(addr, 0x07);
	printk("tw2865 init failed .0x6ctemp-- is :%0x\n",temp);
	mdelay(1000);
	temp = 0;
	temp = tw2865_byte_read(addr, 0x07);
	printk("tw2865 init failed .0x6ctemp-- is :%0x\n",temp);
	
	//vd1:ch1 and ch2
	//vd2:ch3 and ch4
	/*CHID*/
	tw2865_byte_write(addr, 0x9e, 0x52);
	/*2nd Channel Selection */
	tw2865_byte_write(addr, 0xcc, 0x00);
	/*1st Channel Selection*/
	tw2865_byte_write(addr, 0xcd, 0x00);
		
	/*Video Channel Output Control 2D1*/
	tw2865_byte_write(addr, 0xca, 0x55);
	
//******************************************************************
//this part is for the setup of audio.	
	/*setup audio*/
/*
	tw2865_write_table(addr, 0xd0, InitAudioSetup, 20);
	tw2865_byte_write(addr, 0xdb, 0xc1);
	tw2865_byte_write(addr, 0x73, 0x01);
	tw2865_byte_write(addr, 0xd2, 0x01);
	tw2865_byte_write(addr, 0xdd, 0x00);
	tw2865_byte_write(addr, 0xde, 0x00);
	tw2865_byte_write(addr, 0xf8, 0xc4);
	tw2865_byte_write(addr, 0xf9, 0x11);
	tw2865_byte_write(addr, 0x70, 0x08);
	tw2865_byte_write(addr, 0x7f, 0x80);
	tw2865_byte_write(addr, 0xcf, 0x80);
*/
//0xd0,0xd1:analog audio input control
//0xd2:number of audio to be record
//0xd3-0xda:sequence of audio to be record
//0xdb:master control
//0xdc:u-Law/A-Law Output and Mix Mute Control
//0xdd:Mix Ratio Value
//0xde:Mix Ratio Value
//0xdf:Analog Audio Output Gain
//0xe1:Audio Detection Period and Audio Detection Threshold
//0xe2:audio detection threshold:ain2 and ain1
//0xe3:audio detection threshold:ain3 and ain4
	tw2865_write_table(addr, 0xd0, InitAudioSetup, 20);

	tw2865_byte_write(addr, 0xdb, 0xd1);//
	tw2865_byte_write(addr, 0x73, 0x01);
	tw2865_byte_write(addr, 0xd2, 0x01);//4 audios
	tw2865_byte_write(addr, 0xdd, 0x00);
	tw2865_byte_write(addr, 0xde, 0x00);
	tw2865_byte_write(addr, 0xf8, 0xc4);
	//tw2865_byte_write(addr, 0xf9, 0x51);
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
	//8 bitwidth
	tw2865_byte_write(addr, 0xd3, 0x10);
	tw2865_byte_write(addr, 0xd4, 0x45);
	tw2865_byte_write(addr, 0xd5, 0x98);
	tw2865_byte_write(addr, 0xd6, 0xdc);
	tw2865_byte_write(addr, 0xd7, 0x32);
	tw2865_byte_write(addr, 0xd8, 0x76);
	tw2865_byte_write(addr, 0xd9, 0xba);
	tw2865_byte_write(addr, 0xda, 0xfe);
	//16bitwiath
	tw2865_byte_write(addr, 0xd3, 0x10);
	tw2865_byte_write(addr, 0xd4, 0x54);
	tw2865_byte_write(addr, 0xd5, 0x98);
	tw2865_byte_write(addr, 0xd6, 0xdc);
	tw2865_byte_write(addr, 0xd7, 0x32);
	tw2865_byte_write(addr, 0xd8, 0x76);
	tw2865_byte_write(addr, 0xd9, 0xba);
	tw2865_byte_write(addr, 0xda, 0xfe);
	/*setup audio {0x10,0x54,0x98,0xdc,0x32,0x76,0xba,0xfe};*/
/*
	tw2865_byte_write(addr, 0xd3, 0x10);
	tw2865_byte_write(addr, 0xd4, 0x54);
	tw2865_byte_write(addr, 0xd5, 0x98);
	tw2865_byte_write(addr, 0xd6, 0xdc);
	tw2865_byte_write(addr, 0xd7, 0x32);
	tw2865_byte_write(addr, 0xd8, 0x76);
	tw2865_byte_write(addr, 0xd9, 0xba);
	tw2865_byte_write(addr, 0xda, 0xfe);
*/
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
};


module_param(norm, uint, S_IRUGO);
module_param(chips, uint, S_IRUGO);


DECLARE_KCOM_GPIO_I2C();
static int __init tw2865_init(void)
{
	int ret = 0;

	printk("tw2865_init norm:%d, chips:%d\n", norm, chips);

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
	ret = KCOM_GPIO_I2C_INIT();
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
	
	printk("tw2865 driver init successful!\n");
	
	return 0;

FAILED:
	misc_deregister(&tw2865a_dev);
	KCOM_GPIO_I2C_EXIT();
	return ret;
}



static void __exit tw2865_exit(void)
{
	misc_deregister(&tw2865a_dev);
	KCOM_GPIO_I2C_EXIT();
}

module_init(tw2865_init);
module_exit(tw2865_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");

