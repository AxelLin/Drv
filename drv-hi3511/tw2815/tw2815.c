/* extdrv/peripheral/vad/tw2815a.c
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
 *      10-April-2006 create this file
 *      2006-04-29  add record path half d1 mod
 *      2006-05-13  set the playpath default output mod to full
 *      2006-05-24  add record mod 2cif
 *      2006-06-15  support mod changing between every record mod
 *      2006-08-12  change the filters when record mod change
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
#include "tw2815_def.h"
#include "tw2815.h"

#define DEBUG_2815 1


static unsigned int tw2815a_dev_open_cnt =0;
static unsigned int tw2815b_dev_open_cnt =0;
static unsigned int  cascad_judge = 0;


unsigned char channel_array[8] = {0x20,0x64,0xa8,0xec,0x31,0x75,0xb9,0xfd};
unsigned char channel_array_default[8] = {0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe};
unsigned char channel_array_our[8] = {0x10,0x32,0x98,0xba,0x54,0x76,0xdc,0xfe};
unsigned char channel_array_4chn[8] = {0x10,0x54,0x98,0xdc,0x32,0x76,0xba,0xfe};




static unsigned char tw2815_byte_write(unsigned char chip_addr,unsigned char addr,unsigned char data) {
    gpio_i2c_write(chip_addr,addr,data);   
    return 0;
}

static unsigned char tw2815_byte_read(unsigned char chip_addr,unsigned char addr)
{
    return gpio_i2c_read_tw2815(chip_addr,addr);   
}


static void tw2815_write_table(unsigned char chip_addr,unsigned char addr,unsigned char *tbl_ptr,unsigned char tbl_cnt)
{
    unsigned char i = 0;
    for(i = 0;i<tbl_cnt;i++)
    {
        gpio_i2c_write(chip_addr,(addr+i),*(tbl_ptr+i));
    }
}

static void tw2815_read_table(unsigned char chip_addr,unsigned char addr,unsigned char reg_num)
{
	unsigned char i = 0,temp = 0;
	for(i =0; i < reg_num;i++ )
	{
		temp = tw2815_byte_read(chip_addr,addr+i);
		printk("reg 0x%x=0x%x,",addr+i,temp);
		if(((i+1)%4)==0)
		printk("\n");
		
	}
}


static void set_2_d1(unsigned char chip_addr,unsigned char ch1,unsigned char ch2)
{
    unsigned char temp=0;
    
    if(ch1 >3 || ch2 > 3)
     {
        printk("tw2815 video chunnel error\n");
        return;
     }
     
    temp = tw2815_byte_read(chip_addr,0x4d);
    CLEAR_BIT(temp,0x33);
    SET_BIT(temp,0x88);
    tw2815_byte_write(chip_addr,0x4d,temp);
    
    
    temp = tw2815_byte_read(chip_addr,0x43);
    SET_BIT(temp,0x3);
    tw2815_byte_write(chip_addr,0x43,temp);
   
    temp = tw2815_byte_read(chip_addr,0x75);
    CLEAR_BIT(temp,0x1<<ch1);
    tw2815_byte_write(chip_addr,0x75,temp);
     //printk("file:%s,line:%d\r\n",__FILE__,__LINE__);
     temp = tw2815_byte_read(chip_addr,0x0d+ch1*0x10);
     SET_BIT(temp,0x04);
     tw2815_byte_write(chip_addr,(0x0d+ch1*0x10),temp);

     temp = tw2815_byte_read(chip_addr,0x0d+ch1*0x10);
     CLEAR_BIT(temp,0x03);
     SET_BIT(temp,ch2);
     tw2815_byte_write(chip_addr,(0x0d+ch1*0x10),temp);
     /*
     temp = tw2815_byte_read(chip_addr,0x0d+ch2*0x10);
     SET_BIT(temp,0x04);
     tw2815_byte_write(chip_addr,(0x0d+ch2*0x10),temp);

     temp = tw2815_byte_read(chip_addr,0x0d+ch2*0x10);
     CLEAR_BIT(temp,0x03);
     SET_BIT(temp,ch1);
     tw2815_byte_write(chip_addr,(0x0d+ch2*0x10),temp);
     */
     return;
     
   
}
static void tw2815_video_mode_init(unsigned chip_addr,unsigned char video_mode,unsigned char ch)
{
     unsigned char video_mode_ctrl = 0,temp = 0,mode_temp = 0;
     mode_temp = video_mode;
   //soft reset 
    temp = tw2815_byte_read(chip_addr, ch*0x10+0x0d);
    SET_BIT(temp,0x08);
    tw2815_byte_write(chip_addr,ch*0x10+0x0d,temp);
    udelay(50);
    if(video_mode == AUTOMATICALLY)
    	{
	      video_mode_ctrl = tw2815_byte_read(chip_addr,ch*0x10+0x1);
	      CLEAR_BIT(video_mode_ctrl,0x80);
	      tw2815_byte_write(chip_addr,ch*0x10+0x01,video_mode_ctrl);
	    //  usleep(50);//delay for automatically
	      mode_temp = (tw2815_byte_read(chip_addr,ch*0x10+0x0))>>5;
	      if(mode_temp <= 3)
	      	{
	      		mode_temp = PAL;
	      	}
	      else
	      	{
	      		mode_temp = NTSC;
	      	}
	}
    
    if(mode_temp == NTSC)
    {
        
        tw2815_write_table(chip_addr,0x00+0x10*ch,tbl_ntsc_tw2815_common,15);
        tw2815_write_table(chip_addr,0x40,tbl_ntsc_tw2815_sfr1,16);
        tw2815_write_table(chip_addr,0x50,tbl_ntsc_tw2815_sfr2,10);
    }
    else
    {
        
        tw2815_write_table(chip_addr,0x00+0x10*ch,tbl_pal_tw2815_common,15);
        tw2815_write_table(chip_addr,0x40,tbl_pal_tw2815_sfr1,16);
        tw2815_write_table(chip_addr,0x50,tbl_pal_tw2815_sfr2,10);
    }

    temp = tw2815_byte_read(chip_addr,0x43);
    SET_BIT(chip_addr,0x80);
    tw2815_byte_write(chip_addr,0x43,temp);	
   printk("tw2815 channel=%d set videomode %d\n",ch,video_mode); 
}


static int tw2815_device_video_init(unsigned char chip_addr,unsigned char video_mode)
{
    unsigned char tw2815_id =0;
    unsigned int i;
    
  tw2815_id = tw2815_byte_read(chip_addr,TW2815_ID);
    if(tw2815_id != 0x20)
    {
	 if(DEBUG_2815)
	 printk(" tw2815_id =%x\n",tw2815_id);
        return -1;
    }
    
    tw2815_video_mode_init(chip_addr,video_mode,0);
    tw2815_video_mode_init(chip_addr,video_mode,1);
    tw2815_video_mode_init(chip_addr,video_mode,2);
    tw2815_video_mode_init(chip_addr,video_mode,3);
    
    for(i=0;i<4;i++)
    {
	    tw2815_byte_write(chip_addr,i*0x10+0x0b,0x9a);        
    }
    
    return 0;
}

static void tw2815_reg_dump(unsigned char chip_addr)
{
	tw2815_read_table(chip_addr,0x0,0x76);
	printk("tw2815_reg_dump ok\n");
}
/*restart alloc channel
default: 0
right and left alloc:  1
our:2*/

static void channel_alloc(unsigned char chip_addr,unsigned char ch)
{
	unsigned char i = 0;
	if(ch == 0)
	{
		for(i =0 ;i < 8;i++)
		{
			tw2815_byte_write(chip_addr,0x64+i,0);
			tw2815_byte_write(chip_addr,0x64+i,channel_array_default[i]);
		}
	}
	else if(ch == 1)
	{
		for(i =0 ;i < 8;i++)
		{
			tw2815_byte_write(chip_addr,0x64+i,0);
			tw2815_byte_write(chip_addr,0x64+i,channel_array[i]);
		}
		
	}
	else if(ch == 2)
	{
			for(i =0 ;i < 8;i++)
		{
			tw2815_byte_write(chip_addr,0x64+i,0);
			tw2815_byte_write(chip_addr,0x64+i,channel_array_our[i]);
		}
	}
	else if(ch == 3)
	{
		for(i = 0; i < 8; i++)
		{
			tw2815_byte_write(chip_addr,0x64+i,0);
			tw2815_byte_write(chip_addr,0x64+i,channel_array_4chn[i]);
		}
	}
}
/*
init 
when tw2815a:chip_addr = 0x50 
when tw2815b:chip_addr = 0x52
*/
static int tw2815_device_audio_init(unsigned char chip_addr)
{
    unsigned char temp;
    tw2815_write_table(chip_addr,0x5a,tbl_tw2815_audio,28);
    channel_alloc(chip_addr,1);
/*enable output*/
    temp = tw2815_byte_read(chip_addr,0x43);
    SET_BIT(temp,0x80);
    tw2815_byte_write(chip_addr,0x43,temp);

    printk("tw2815 audio init ok\n");			
    return 0;
}

#if 0
static void tw2815_vin_cropping(unsigned chip_addr,unsigned int path,unsigned int hdelay,unsigned int hactive,unsigned int vdelay,unsigned int vactive)
{
    
    tw2815_byte_write(chip_addr,(0x02+path*0x10),(hdelay&0xff));
    tw2815_byte_write(chip_addr,(0x06+path*0x10),((hdelay&0x300)>>8));
    tw2815_byte_write(chip_addr,(0x03+path*0x10),(hdelay&0xff));
    tw2815_byte_write(chip_addr,(0x06+path*0x10),((hdelay&0xc00)>>8));
    tw2815_byte_write(chip_addr,(0x04+path*0x10),(hdelay&0xff));
    tw2815_byte_write(chip_addr,(0x06+path*0x10),((hdelay&0x3000)>>8));
    tw2815_byte_write(chip_addr,(0x05+path*0x10),(hdelay&0xff));
    tw2815_byte_write(chip_addr,(0x06+path*0x10),((hdelay&0xc000)>>8));
     printk("tw2815_vin_cropping ok\n");
}
#endif

static void setd1(unsigned char chip_addr)
{
    unsigned char t1 = 0,temp = 0;
#if 0    
    temp = tw2815_byte_read(chip_addr,0x4d);
    CLEAR_BIT(temp,0x33);
    SET_BIT(temp,0x22);
    tw2815_byte_write(chip_addr,0x4d,temp);
#endif    
    temp = tw2815_byte_read(chip_addr,0x75);
    CLEAR_BIT(temp,0xff);
    tw2815_byte_write(chip_addr,0x75,temp);
	
    for(t1 = 0;t1 < 4;t1++)
    {
        temp = tw2815_byte_read(chip_addr,0x0d+t1*0x10);
        CLEAR_BIT(temp,0x04);
        tw2815_byte_write(chip_addr,(0x0d+t1*0x10),temp);
    }
    temp = tw2815_byte_read(chip_addr,0x43);
    CLEAR_BIT(temp,0x3);
    tw2815_byte_write(chip_addr,0x43,temp);
    printk("setd1 ok\n");
    return;
}



static void set_4half_d1(unsigned char chip_addr,unsigned char ch)
{
    unsigned char temp = 0;
    
   temp = tw2815_byte_read(chip_addr,0x4d);
   CLEAR_BIT(temp,0x33);
   SET_BIT(temp,0x88);
   tw2815_byte_write(chip_addr,0x4d,temp);
    
    
    temp = tw2815_byte_read(chip_addr,0x0d+ch*0x10);
    CLEAR_BIT(temp,0x7);
    tw2815_byte_write(chip_addr,0x0d+ch*0x10,temp);


    temp = tw2815_byte_read(chip_addr,0x75);
    CLEAR_BIT(temp,0xf);
    SET_BIT(temp,0x1<<ch);
    tw2815_byte_write(chip_addr,0x75,temp);

    temp = tw2815_byte_read(chip_addr,0x71);
    CLEAR_BIT(temp,0x40);
    tw2815_byte_write(chip_addr,0x71,temp);

    temp = tw2815_byte_read(chip_addr,0x43);
    SET_BIT(temp,0x03);
    tw2815_byte_write(chip_addr,0x43,temp);
    
     printk("set_4half_d1 ok\n");
     
     
    return;
}

static void set_4cif(unsigned char chip_addr,unsigned char ch)
{
    unsigned char temp = 0;
    
    temp = tw2815_byte_read(chip_addr,0x4d);
    CLEAR_BIT(temp,0x33);
    SET_BIT(temp,0x88);
    tw2815_byte_write(chip_addr,0x4d,temp);
    
    
    temp = tw2815_byte_read(chip_addr,0x0d+ch*0x10);
    CLEAR_BIT(temp,0x7);
    tw2815_byte_write(chip_addr,0x0d+ch*0x10,temp);

		
    temp = tw2815_byte_read(chip_addr,0x75);
    CLEAR_BIT(temp,0xf);
    SET_BIT(temp,0x1<<ch);
    tw2815_byte_write(chip_addr,0x75,temp);


    temp = tw2815_byte_read(chip_addr,0x71);
    CLEAR_BIT(temp,0x40);
    //SET_BIT(temp,0x20); //skip sync code as cif
    tw2815_byte_write(chip_addr,0x71,temp);


    temp = tw2815_byte_read(chip_addr,0x43);
    SET_BIT(temp,0x03);
    tw2815_byte_write(chip_addr,0x43,temp);	
           
    
    
    return;
}
/*set samplerate 
control_reg = serial_playback_control(playback control)(0x6c)
control_reg = serial_control(normal control)(0x62)
tmp = SET_8K_SAMPLERATE
tmp = SET_16K_SAMPLERATE
*/
static void tw2815_set_ada_playback_samplerate(unsigned char chip_addr,unsigned char control_reg,unsigned char samplerate)
{
    unsigned int ada_samplerate = 0;	
                 switch(samplerate)
                        {
                                case SET_8K_SAMPLERATE:
                                    ada_samplerate = tw2815_byte_read(chip_addr,control_reg);
                                    CLEAR_BIT(ada_samplerate,0x04);
                                    tw2815_byte_write(chip_addr,control_reg,ada_samplerate);
                                    break;

                                case SET_16K_SAMPLERATE:   
                                    ada_samplerate = tw2815_byte_read(chip_addr,control_reg);
                                    SET_BIT(ada_samplerate,0x04);
                                    tw2815_byte_write(chip_addr,control_reg,ada_samplerate);
                                    break;
				default:
				    break;                                
                         }
        printk("tw2815 set samplerate ok\n");
        return;                      
}

/*set bitwidth 
control_reg = serial_playback_control(playback control)
control_reg = serial_control(normal control)
tmp = SET_8_BITWIDTH
tmp = SET_16_BITWIDTH
*/
static void tw2815_set_ada_playback_bitwidth(unsigned char chip_addr,unsigned char control_reg,unsigned char bitwidth)
{
	unsigned int ada_bitwidth = 0;
                        switch(bitwidth)
                             {
                                case SET_8_BITWIDTH:
                                    ada_bitwidth = tw2815_byte_read(chip_addr,control_reg);
                                    SET_BIT(ada_bitwidth,0x02);
                                    tw2815_byte_write(chip_addr,control_reg,ada_bitwidth);
                                    break;

                                case SET_16_BITWIDTH:   
                                    ada_bitwidth = tw2815_byte_read(chip_addr,control_reg);
                                    CLEAR_BIT(ada_bitwidth,0x02);
                                    tw2815_byte_write(chip_addr,control_reg,ada_bitwidth);
                                    break;
				default:
				    break;
                              }
	 printk("tw2815 set bitwidth ok\n");
         return;         
}
/*
control_reg = serial_playback_control(playback control)
control_reg = serial_control(normal control)
tmp = SET_256_BITRATE(0)
tmp = SET_384_BITRATE(1)
*/
static void tw2815_set_ada_playback_bitrate(unsigned char chip_addr,unsigned char control_reg,unsigned char bitrate)
{
	
	 unsigned int ada_bitrate = 0;
                        switch(bitrate)
                             {
                                case SET_256_BITRATE:
                                    ada_bitrate = tw2815_byte_read(chip_addr,control_reg);
                                    CLEAR_BIT(ada_bitrate,0x10);
                                    tw2815_byte_write(chip_addr,control_reg,ada_bitrate);
                                    break;

                                case SET_384_BITRATE:   
                                    ada_bitrate = tw2815_byte_read(chip_addr,control_reg);
                                    SET_BIT(ada_bitrate,0x10);
                                    tw2815_byte_write(chip_addr,control_reg,ada_bitrate);
                                    break;
				default:
				   break;
			    }
          printk("tw2815 set bitrate ok\n");                     
         return;	
}

static void set_audio_output(unsigned char chip_addr,unsigned char num_path)
{
    unsigned char temp = 0;
    unsigned char help = 0;
  #if 0
   if(cascad_judge)
    {
	return;
    }
  #endif		
    if(0 < num_path && num_path <=2)
	 help = 0;
   else if(2 < num_path && num_path <= 4)
        help = 1;
    else if(4 < num_path && num_path <= 8)
        help = 2;
    else if(8 <num_path && num_path <=16)
        help = 3;
    else
    {
        printk("tw2815a audio path choice error\n");
        return;
    }
    temp = tw2815_byte_read(chip_addr,0x63);
   	
    CLEAR_BIT(temp,0x3);
    	
    SET_BIT(temp,help);
 
    tw2815_byte_write(chip_addr,0x63,temp);

     printk("tw2815 set output path=%d  ok\n",num_path);	
   	
    return;
    
}
/*set cascad*/
static void set_audio_cascad(void)
{
	unsigned char temp = 0;
	temp = tw2815_byte_read(0x50,0x62);
	CLEAR_BIT(temp,0xc0);
	SET_BIT(temp,0x40);
	tw2815_byte_write(0x50,0x62,temp);
	
	temp = tw2815_byte_read(0x52,0x62);
	CLEAR_BIT(temp,0xc0);
	SET_BIT(temp,0x80);
	tw2815_byte_write(0x52,0x62,temp);

        temp = tw2815_byte_read(0x52,0x5b);
	CLEAR_BIT(temp,0xff);
	tw2815_byte_write(0x52,0x5b,temp);

 	temp = tw2815_byte_read(0x52,0x5c);
	CLEAR_BIT(temp,0x0);
	SET_BIT(temp,0xa0);
	tw2815_byte_write(0x52,0x5c,temp);

	set_audio_output(0x52,4);        
	set_audio_output(0x50,8);
	cascad_judge = 1;
	 printk("tw2815 set  cascad ok\n");
	
}

static void set_audio_mix_out(unsigned char chip_addr,unsigned char ch)
{
    unsigned char temp = 0;
    temp = tw2815_byte_read(chip_addr,0x63);
    CLEAR_BIT(temp,0x04);
    tw2815_byte_write(chip_addr,0x63,temp);
    temp = tw2815_byte_read(chip_addr,0x71);
    CLEAR_BIT(temp,0x1f);
    SET_BIT(temp,ch);
    tw2815_byte_write(chip_addr,0x71,temp);
    if(ch == 16)
        printk("tw2815 select playback audio out\n");
    if(ch == 17)
        printk("tw2815 select mix digital and analog audio data\n");
    printk("set_audio_mix_out ok\n");    
    return;
}
/*single chip*/
static void set_single_chip(void)
{
	unsigned char temp = 0;
	temp = tw2815_byte_read(0x50,0x62);
	CLEAR_BIT(temp,0xc0);
	SET_BIT(temp,0xc0);
	tw2815_byte_write(0x50,0x62,temp);
	
	temp = tw2815_byte_read(0x52,0x62);
	CLEAR_BIT(temp,0xc0);
	SET_BIT(temp,0xc0);
	tw2815_byte_write(0x52,0x62,temp);
	
	
	temp = tw2815_byte_read(0x52,0x5b);
	SET_BIT(temp,0xff);
	tw2815_byte_write(0x52,0x5b,temp);

 	temp = tw2815_byte_read(0x52,0x5c);
	CLEAR_BIT(temp,0x0);
	SET_BIT(temp,0x2f);
	tw2815_byte_write(0x52,0x5c,temp);
	cascad_judge = 0;	
	printk("set_single_chip ok\n");
}
static void set_audio_record_m(unsigned char chip_addr,unsigned char num_path)
{
    unsigned char temp =0 ,help =0;
    temp = tw2815_byte_read(chip_addr,0x63);
    SET_BIT(temp,0x04);

    
    CLEAR_BIT(temp,0x3);
   if(0 < num_path && num_path <=2)
	 help = 0;
   else if(2 < num_path && num_path <= 4)
        help = 1;
    else if(4 < num_path && num_path <= 8)
        help = 2;
    else if(8 <num_path && num_path <=16)
        help = 3;
    else
    {
        printk("tw2815a audio path choice error\n");
        return;
    }
    SET_BIT(temp,help);
    tw2815_byte_write(chip_addr,0x63,temp);
    printk("set_audio_record_m ok");
    return;
    
    
}

static void set_audio_mix_mute(unsigned char chip_addr,unsigned char ch) 
{
    unsigned char temp = 0 ;
    temp = tw2815_byte_read(chip_addr,0x6d);
    SET_BIT(temp,1<<ch);
    tw2815_byte_write(chip_addr,0x6d,temp);
    printk("set_audio_mix_mute ch =%d ok\n",ch);
    return;
}

static void clear_audio_mix_mute(unsigned char chip_addr,unsigned char ch)
{
    unsigned char temp = 0;
    temp = tw2815_byte_read(chip_addr,0x6d);
    CLEAR_BIT(temp,1<<ch);
    tw2815_byte_write(chip_addr,0x6d,temp);
    printk("tw2815 clear mute ch = %d ok\n",ch);
    return;
}

static void hue_control(unsigned char chip_addr,unsigned char ch ,unsigned char hue_value)
{   
    tw2815_byte_write(chip_addr,ch*0x10+0x7,hue_value);

}
static void hue_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *hue_value)
{
    *hue_value = tw2815_byte_read(chip_addr,ch*0x10+0x7);
}


static void saturation_control(unsigned char chip_addr,unsigned char ch ,unsigned char saturation_value)
{
    tw2815_byte_write(chip_addr,ch*0x10+0x8,saturation_value);
}
static void saturation_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *saturation_value)
{
    *saturation_value = tw2815_byte_read(chip_addr,ch*0x10+0x8);
}


static void contrast_control(unsigned char chip_addr,unsigned char ch ,unsigned char contrast_value)
{
    tw2815_byte_write(chip_addr,ch*0x10+0x9,contrast_value);
}
static void contrast_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *contrast_value)
{
    *contrast_value = tw2815_byte_read(chip_addr,ch*0x10+0x9);
}

static void brightness_control(unsigned char chip_addr,unsigned char ch ,unsigned char brightness_value)
{
    tw2815_byte_write(chip_addr,ch*0x10+0xa,brightness_value);
}
static void brightness_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *brightness_value)
{
    *brightness_value = tw2815_byte_read(chip_addr,ch*0x10+0xa);
}


static int luminance_peaking_control(unsigned char chip_addr,unsigned char ch ,unsigned char luminance_peaking_value)
{
    unsigned char temp = 0;
    if(luminance_peaking_value > 15)
    {
        return -1;
    }
    temp = tw2815_byte_read(chip_addr,ch*0x10+0xb);
    CLEAR_BIT(temp,0xf);
    SET_BIT(temp,luminance_peaking_value);
    tw2815_byte_write(chip_addr,ch*0x10+0xb,temp);
    return 0;
}
static void luminance_peaking_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *luminance_peaking_value)
{
    *luminance_peaking_value = (tw2815_byte_read(chip_addr,ch*0x10+0xb)&0x0f);
}

static int cti_control(unsigned char chip_addr,unsigned char ch ,unsigned char cti_value)
{
    unsigned char temp = 0;
    if(cti_value > 15)
    {
        return -1;
    }
    temp = tw2815_byte_read(chip_addr,ch*0x10+0xc);
    CLEAR_BIT(temp,0xf);
    SET_BIT(temp,cti_value);
    tw2815_byte_write(chip_addr,ch*0x10+0xc,temp);
    return 0;
}
static void cti_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *cti_value)
{
     *cti_value = (tw2815_byte_read(chip_addr,ch*0x10+0xc)&0x0f);
}
/*
 * tw2815a read routine.
 * do nothing.
 */
ssize_t tw2815a_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}

/*
 * tw2815a write routine.
 * do nothing.
 */
ssize_t tw2815a_write(struct file * file, const char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}

/*
 * tw2834 open routine.
 * do nothing.
 */
 
 int tw2815a_open(struct inode * inode, struct file * file)
{
#if 0
   if(tw2815a_dev_open_cnt)
   {
   	printk("error tw2815adev had open\n");
        return -EFAULT;        
   }	
   tw2815a_dev_open_cnt++;
    if(DEBUG_2815)
    {
    	printk("tw2815a_dev open ok\n");
    }	
#endif    
    return 0;
} 
/*
 * tw2815a close routine.
 * do nothing.
 */
int tw2815a_close(struct inode * inode, struct file * file)
{
    tw2815a_dev_open_cnt--;	
    return 0;
}

Tw2815Ret tw2815a_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int __user *argp = (unsigned int __user *)arg;
    unsigned int tmp,tmp_help,samplerate,ada_samplerate,bitwidth,ada_bitwidth,bitrate,ada_bitrate;
    tw2815_w_reg tw2815reg;
    tw2815_set_2d1 tw2815_2d1;
    tw2815_set_videomode tw2815_videomode;
    tw2815_set_controlvalue tw2815_controlvalue;
    switch(cmd)
            {
            case TW2815_READ_REG:
                if (copy_from_user(&tmp_help, argp, sizeof(tmp_help)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_READ_REG_FAIL;
                }
                tmp = tw2815_byte_read(TW2815A_I2C_ADDR,(u8)tmp_help);
                copy_to_user(argp, &tmp, sizeof(tmp));
                break;
            case TW2815_WRITE_REG:
                if (copy_from_user(&tw2815reg, argp, sizeof(tw2815reg)))
                {
                    printk("ttw2815a_ERROR");
                    return TW2815_WRITE_REG_FAIL;
                }
                tw2815_byte_write(TW2815A_I2C_ADDR,(u8)tw2815reg.addr,(u8)tw2815reg.value);
                break;
     
            
	     case TW2815_SET_ADA_PLAYBACK_SAMPLERATE :
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_PLAYBACK_SAMPLERATE_FAIL;
                }    
                     samplerate = tmp;
                     tw2815_set_ada_playback_samplerate(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,samplerate);    
                        break;
                        
                        
	    case TW2815_GET_ADA_PLAYBACK_SAMPLERATE :
				ada_samplerate = tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL);
				if((ada_samplerate & 0x04)==0x04)
			        {
                                    ada_samplerate = SET_16K_SAMPLERATE;
                                }
                                else
                                {
                                    ada_samplerate = SET_8K_SAMPLERATE;
                                }
                                copy_to_user(argp,&ada_samplerate,sizeof(ada_samplerate));
				break;
				
            case TW2815_SET_ADA_PLAYBACK_BITWIDTH:
                 if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_PLAYBACK_BITWIDTH_FAIL;
                }      
                        bitwidth = tmp;
                 tw2815_set_ada_playback_bitwidth(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,bitwidth);             
                        break;


             case TW2815_GET_ADA_PLAYBACK_BITWIDTH :
				ada_bitwidth = tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL);
				if((ada_bitwidth & 0x02)==0x02)
			        {
                                    ada_bitwidth = SET_8_BITWIDTH;
                                }
                                else
                                {
                                    ada_bitwidth = SET_16_BITWIDTH;
                                }
                                copy_to_user(argp,&ada_bitwidth,sizeof(ada_bitwidth));
				break;


				
            case TW2815_SET_ADA_PLAYBACK_BITRATE:
                 if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_PLAYBACK_BITRATE_FAIL;
                }       
                        bitrate= tmp;
                        tw2815_set_ada_playback_bitrate(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,bitrate);
                        break;

              case TW2815_GET_ADA_PLAYBACK_BITRATE:
				ada_bitrate = tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL);
				if((ada_bitrate & 0x10)==0x10)
                                {
                                    ada_bitrate = SET_384_BITRATE;
                                }
                                else
                                {
                                    ada_bitrate = SET_256_BITRATE;
                                }
                                copy_to_user(argp,&ada_bitrate,sizeof(ada_bitrate));
				break;          


           case TW2815_SET_ADA_SAMPLERATE :
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_SAMPLERATE_FAIL;
                }        
                      samplerate = tmp;
                      if((tmp !=SET_8K_SAMPLERATE)&&(tmp !=SET_16K_SAMPLERATE))
                      {
                      	        return 	TW2815_SET_ADA_SAMPLERATE_FAIL;
                      }
			printk("samplerate = %d\n",samplerate);
                      tw2815_set_ada_playback_samplerate(TW2815A_I2C_ADDR,SERIAL_CONTROL,samplerate);
                      
                      break;


             case TW2815_GET_ADA_SAMPLERATE:
				ada_samplerate= tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_CONTROL);
				if((ada_samplerate & 0x04)==0x04)
                                {
                                    ada_samplerate = SET_16K_SAMPLERATE;
                                }
                                else
                                {
                                    ada_samplerate = SET_8K_SAMPLERATE;
                                }
                                copy_to_user(argp,&ada_samplerate,sizeof(ada_samplerate));
				break;               
                        
             case TW2815_SET_ADA_BITWIDTH:
               if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_BITWIDTH_FAIL;
                }                    
                      bitwidth = tmp;
                      if(bitwidth == SET_8_BITWIDTH)
                      {
                      	   set_audio_cascad();
                      }
                      else if(bitwidth == SET_16_BITWIDTH)
                      {
                      	   set_single_chip();		
                      }
                      else
                      {
                      	   return TW2815_SET_ADA_BITWIDTH_FAIL;
                      }
                      tw2815_set_ada_playback_bitwidth(TW2815A_I2C_ADDR,SERIAL_CONTROL,bitwidth);
                      break;

                   case TW2815_GET_ADA_BITWIDTH:
				ada_bitwidth = tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_CONTROL);
				if((ada_bitwidth & 0x02)==0x02)
                                {
                                    ada_bitwidth = SET_8_BITWIDTH;
                                }
                                else
                                {
                                    ada_bitwidth = SET_16_BITWIDTH;
                                }
                                copy_to_user(argp,&ada_bitwidth,sizeof(ada_bitwidth));
				break;      
				
            case TW2815_SET_ADA_BITRATE:
             if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_BITRATE_FAIL;
                }                       
                      bitrate= tmp;
                      if((tmp !=SET_256_BITRATE)&&(tmp !=SET_384_BITRATE))
                      {
                      		return TW2815_SET_ADA_BITRATE_FAIL;
                      }	
                      tw2815_set_ada_playback_bitrate(TW2815A_I2C_ADDR,SERIAL_CONTROL,bitrate);
                     break;
           case TW2815_GET_ADA_BITRATE:
		            ada_bitrate = tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_CONTROL);
				if((ada_bitrate & 0x10)==0x10)
                                {
                                    ada_bitrate = SET_384_BITRATE;
                                }
                                else
                                {
                                    ada_bitrate = SET_256_BITRATE;
                                }
                                copy_to_user(argp,&ada_bitrate,sizeof(ada_bitrate));
				break;     

				
             case TW2815_SET_D1:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_D1_FAIL;
                    
                }
                setd1(TW2815A_I2C_ADDR);
               break;
                
             case TW2815_SET_2_D1:
                
                if (copy_from_user(&tw2815_2d1, argp, sizeof(tw2815_2d1)))
                {
                    return TW2815_SET_2_D1_FAIL;
                }
                set_2_d1(TW2815A_I2C_ADDR,tw2815_2d1.ch1,tw2815_2d1.ch2);
                break;
                
             case TW2815_SET_4HALF_D1:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_4HALF_D1_FAIL;
                }
                set_4half_d1(TW2815A_I2C_ADDR,tmp);
                break;
                
             case TW2815_SET_4_CIF:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_4_CIF_FAIL;
                    
                } 
                set_4cif(TW2815A_I2C_ADDR,tmp);
                break;
                
             case  TW2815_SET_AUDIO_OUTPUT:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_OUTPUT_FAIL;
                }
                if((tmp<=0)||(tmp>16))
                {
                	return TW2815_SET_AUDIO_OUTPUT_FAIL;
                }
                set_audio_output(TW2815A_I2C_ADDR,tmp);
                
                break;
                
             case TW2815_GET_AUDIO_OUTPUT:
                 tmp = tw2815_byte_read(TW2815A_I2C_ADDR,0x63);
                 if((tmp & 0x03)==0x0)
                 {
                     tmp_help = 2;
                 }
                 else if((tmp & 0x03)==0x1)
                 {
                     tmp_help =4;
                 }
                 else if((tmp & 0x03)==0x2)
                 {
                     tmp_help =8;
                 }
                 else if((tmp & 0x03)==0x3)
                 {
                     tmp_help =16;
                 }
                 copy_to_user(argp,&tmp_help,sizeof(tmp_help));
                
               break; 
             case  TW2815_SET_AUDIO_MIX_OUT:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_MIX_OUT_FAIL;
                }
                set_audio_mix_out(TW2815A_I2C_ADDR,tmp);
                break;
                
             case TW2815_SET_AUDIO_RECORD_M:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_RECORD_M_FAIL;
                }
                set_audio_record_m(TW2815A_I2C_ADDR,tmp);
                break;
                
             case TW2815_SET_MIX_MUTE:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_MIX_MUTE_FAIL;
                }
		if(tmp<0||tmp>4)
		{
	          printk("TW2815A_SET_MIX_MUTE_FAIL\n");
		   return TW2815_SET_MIX_MUTE_FAIL;
		}  
                set_audio_mix_mute(TW2815A_I2C_ADDR,tmp);
                break;
                
             case TW2815_CLEAR_MIX_MUTE:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_CLEAR_MIX_MUTE_FAIL;
                }
	         if(tmp<0||tmp>4)
                {
	           printk("TW2815A_CLEAR_MIX_MUTE_FAIL\n");
                   return TW2815_CLEAR_MIX_MUTE_FAIL;
                }
	
                clear_audio_mix_mute(TW2815A_I2C_ADDR,tmp);
                break;

             case TW2815_SET_VIDEO_MODE:
                if (copy_from_user(&tw2815_videomode, argp, sizeof(tw2815_videomode)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tw2815_videomode.mode);
                    return TW2815_SET_VIDEO_MODE_FAIL;
                }
                if((tw2815_videomode.mode != NTSC)&&(tw2815_videomode.mode !=PAL)&&(tw2815_videomode.mode !=AUTOMATICALLY))
                {
                	printk("set video mode %d error\n ",tw2815_videomode.mode);
                	return TW2815_SET_VIDEO_MODE_FAIL;
                }
                tw2815_video_mode_init(TW2815A_I2C_ADDR,tw2815_videomode.mode,tw2815_videomode.ch);
               break; 
              case TW2815_GET_VIDEO_MODE:
              
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_GET_VIDEO_MODE_FAIL;
                }
	     #if 0	
              tmp_help = tw2815_byte_read(TW2815A_I2C_ADDR,tmp*0x10+0x1);
	      CLEAR_BIT(tmp_help,0x80);
	      tw2815_byte_write(TW2815A_I2C_ADDR,tmp*0x10+0x01,tmp_help);
	      #endif
		tmp_help = (tw2815_byte_read(TW2815A_I2C_ADDR,tmp*0x10+0x0))>>5;
	      
		if(tmp_help <= 3)
	      	{
	      		tmp = PAL;
	      	}
	      else
	      	{
	      		tmp = NTSC;
	      	}
                copy_to_user(argp,&tmp,sizeof(tmp));
                break;
             case TW2815_REG_DUMP:
             tw2815_reg_dump(TW2815A_I2C_ADDR);
                break;
             case TW2815_SET_CHANNEL_SEQUENCE:
             if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_CHANNEL_SEQUENCE_FAIL;
                }
                if((tmp>3)||(tmp<0))
                return TW2815_SET_CHANNEL_SEQUENCE_FAIL;
                channel_alloc(TW2815A_I2C_ADDR,tmp);
		break;
		case TW2815_SET_AUDIO_CASCAD:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_CASCAD_FAIL;
                }
                if(tmp==SET_AUDIO_CASCAD)
                {
                    set_audio_cascad();
                }
                else if(tmp == SET_AUDIO_SINGLE)
                {
                    set_single_chip();
                }else
                return TW2815_SET_AUDIO_CASCAD_FAIL;
                         
                break;
                
                case TW2815_HUE_CONTROL:
                if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_HUE_CONTROL_FAIL;
                } 
                hue_control(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue);
                break;
                case TW2815_GET_HUE_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_HUE_SET_FAIL;
                }
                 hue_control_get(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                break;
                
                case TW2815_SATURATION_CONTROL:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_SATURATION_CONTROL_FAIL;
                } 
                saturation_control(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue); 
                break;

                case TW2815_GET_SATURATION_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_SATURATION_SET_FAIL;
                }
                 saturation_control_get(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                 break;
               
                case TW2815_CONTRAST_CONTROL:
                  if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_CONTRAST_CONTROL_FAIL;
                }
                
                contrast_control(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue);

                break;
                case TW2815_GET_CONTRAST_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_CONTRAST_SET_FAIL;
                }
                contrast_control_get(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                break;
                case TW2815_BRIGHTNESS_CONTROL:
                   if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_BRIGHTNESS_CONTROL_FAIL;
                }
                   brightness_control(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue);
                break;
                case TW2815_GET_BRIGHTNESS_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_BRIGHTNESS_SET_FAIL;
                }
                   brightness_control_get(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                   break; 
                case TW2815_LUMINANCE_PEAKING_CONTROL:
                    if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_LUMINANCE_PEAKING_FAIL;
                }
                    if(0 != luminance_peaking_control(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue))
                    {
                        return TW2815_LUMINANCE_PEAKING_FAIL;
                    }

                break;
                case TW2815_GET_LUMINANCE_PEAKING_SET:
                   if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_LUMINANCE_PEAKING_SET_FAIL;
                }
                luminance_peaking_control_get(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);   
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                break;
                case TW2815_CTI_CONTROL:
                     if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                     {
                         printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                        return TW2815_CTI_CONTROL_FAIL;
                     }
                    if(0 != cti_control(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue))
                      {
                         return TW2815_CTI_CONTROL_FAIL;
                      }
                     
                break;

                case TW2815_GET_CTI_SET: 
                    if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                     {
                         printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                        return TW2815_GET_CTI_SET_FAIL;
                     }
                    cti_control_get(TW2815A_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                    copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));  
                    break; 
               case TW2815_SET_PLAYBACK_MODE:
                   if (copy_from_user(&tmp, argp, sizeof(tmp))) 
                   {
                       printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                       return TW2815_SET_PLAYBACK_MODE_FAIL;
                   }
                 tmp &= 0x1;
                 tmp_help = (tw2815_byte_read(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL)&0xbf); 
                 tmp_help |=(tmp << 6);
                 tw2815_byte_write(TW2815A_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,tmp_help); 
                case TW2815_SET_CLOCK_OUTPUT_DELAY:
                 if (copy_from_user(&tmp, argp, sizeof(tmp))) 
                   {
                       printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                       return TW2815_SET_CLOCK_OUTPUT_DELAY_FAIL;
                   }
                  tw2815_byte_write(TW2815A_I2C_ADDR,0x4d,tmp);
               default:
                break;
	    }
                                    
    return TW2815_IOCTL_OK;
}

/*
 * tw2815b read routine.
 * do nothing.
 */
ssize_t tw2815b_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}

/*
 * tw2815b write routine.
 * do nothing.
 */
ssize_t tw2815b_write(struct file * file, const char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}

/*
 * tw2834 open routine.
 * do nothing.
 */
int tw2815b_open(struct inode * inode, struct file * file)
{
    return 0;
#if 0
    if(tw2815b_dev_open_cnt)
   {
   	printk("tw2815b_dev is still on");
   	return -EFAULT;
   }	
    tw2815b_dev_open_cnt++;	
    if(DEBUG_2815)
    {
    	printk("tw2815b_dev open ok\n");
    }	
    return 0;
#endif    
}

/*
 * tw2815b close routine.
 * do nothing.
 */
int tw2815b_close(struct inode * inode, struct file * file)
{
   tw2815b_dev_open_cnt --;	
    return 0;
}




Tw2815Ret tw2815b_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int __user *argp = (unsigned int __user *)arg;
    unsigned int tmp,tmp_help,samplerate,ada_samplerate,bitwidth,ada_bitwidth,bitrate,ada_bitrate;
    tw2815_w_reg tw2815reg; 
    tw2815_set_2d1 tw2815_2d1;
    tw2815_set_videomode tw2815_videomode;
    tw2815_set_controlvalue tw2815_controlvalue;
	switch(cmd)
            {
            case TW2815_READ_REG:
                if (copy_from_user(&tmp_help, argp, sizeof(tmp_help)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_READ_REG_FAIL;
                }
                tmp = tw2815_byte_read(TW2815B_I2C_ADDR,(u8)tmp_help);
                copy_to_user(argp, &tmp, sizeof(tmp));
                break;
            case TW2815_WRITE_REG:
                if (copy_from_user(&tw2815reg, argp, sizeof(tw2815reg)))
                {
                    printk("ttw2815b_ERROR");
                    return TW2815_WRITE_REG_FAIL;
                }
                tw2815_byte_write(TW2815B_I2C_ADDR,(u8)tw2815reg.addr,(u8)tw2815reg.value);
                break;
     
            
	       case TW2815_SET_ADA_PLAYBACK_SAMPLERATE :
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_PLAYBACK_SAMPLERATE_FAIL;
                }    
                        samplerate = tmp;
                        tw2815_set_ada_playback_samplerate(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,(u8)samplerate); 
                        break;
                        
	    case TW2815_GET_ADA_PLAYBACK_SAMPLERATE :
				ada_samplerate = tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL);
				if((ada_samplerate & 0x04)==0x04)
                                {
                                    ada_samplerate = SET_16K_SAMPLERATE;
                                }
                                else
                                {
                                    ada_samplerate = SET_8K_SAMPLERATE;
                                }
                                copy_to_user(argp,&ada_samplerate,sizeof(ada_samplerate));
				break;
				
            case TW2815_SET_ADA_PLAYBACK_BITWIDTH:
                 if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_PLAYBACK_BITWIDTH_FAIL;
                }      
                        bitwidth = tmp;
                        tw2815_set_ada_playback_bitwidth(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,(u8)bitwidth);
                        break;


             case TW2815_GET_ADA_PLAYBACK_BITWIDTH :
				ada_bitwidth = tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL);
				if((ada_bitwidth & 0x02)==0x02)
			        {
                                    ada_bitwidth = SET_8_BITWIDTH;
                                }
                                else
                                {
                                    ada_bitwidth = SET_16_BITWIDTH;
                                }
                                copy_to_user(argp,&ada_bitwidth,sizeof(ada_bitwidth));
				break;


				
            case TW2815_SET_ADA_PLAYBACK_BITRATE:
                 if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_PLAYBACK_BITRATE_FAIL;
                }       
                        bitrate= tmp;
       
                   tw2815_set_ada_playback_bitrate(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,bitrate);           
                        break;

              case TW2815_GET_ADA_PLAYBACK_BITRATE:
				ada_bitrate = tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL);
				if((ada_bitrate & 0x10)==0x10)
                                {
                                    ada_bitrate = SET_384_BITRATE;
                                }
                                else
                                {
                                    ada_bitrate = SET_256_BITRATE;
                                }
                                copy_to_user(argp,&ada_bitrate,sizeof(ada_bitrate));
				break;          


           case TW2815_SET_ADA_SAMPLERATE :
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_SAMPLERATE_FAIL;
                }        
                      samplerate = tmp;
                       if((tmp !=SET_8K_SAMPLERATE)&&(tmp !=SET_16K_SAMPLERATE))
                      {
                      	        return 	TW2815_SET_ADA_SAMPLERATE_FAIL;
                      }
                      tw2815_set_ada_playback_samplerate(TW2815B_I2C_ADDR,SERIAL_CONTROL,samplerate);
                        break;

             case TW2815_GET_ADA_SAMPLERATE:
				ada_samplerate= tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_CONTROL);
				if((ada_samplerate & 0x04)==0x04)
			        {
                                    ada_samplerate = SET_16K_SAMPLERATE;
                                }
                                else
                                {
                                    ada_samplerate = SET_8K_SAMPLERATE;
                                }
				copy_to_user(argp,&ada_samplerate,sizeof(ada_samplerate));
				break;               
                        
             case TW2815_SET_ADA_BITWIDTH:
               if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_BITWIDTH_FAIL;
                }                    
                      bitwidth = tmp;
                      
                      if(bitwidth == SET_8_BITWIDTH)
                      {
                      	   set_audio_cascad();
                      }
                      else if(bitwidth == SET_16_BITWIDTH)
                      {
                      	   set_single_chip();		
                      }
                      else
                      {
                      	return TW2815_SET_ADA_BITWIDTH_FAIL;
                      }
                	tw2815_set_ada_playback_bitwidth(TW2815B_I2C_ADDR,SERIAL_CONTROL,bitwidth);
                      break;

                   case TW2815_GET_ADA_BITWIDTH:
				ada_bitwidth = tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_CONTROL);
				if((ada_bitwidth & 0x02)==0x02)
                                {
                                    ada_bitwidth = SET_8_BITWIDTH;
                                }
                                else
                                {
                                    ada_bitwidth = SET_16_BITWIDTH;
                                }
                                copy_to_user(argp,&ada_bitwidth,sizeof(ada_bitwidth));
				break;      
				
             case TW2815_SET_ADA_BITRATE:
             if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_ADA_BITRATE_FAIL;
                }                       
                   bitrate= tmp;
                   if((tmp !=SET_256_BITRATE)&&(tmp!=SET_384_BITRATE))
                      {
                      		return TW2815_SET_ADA_BITRATE_FAIL;
                      }	
                   tw2815_set_ada_playback_bitrate(TW2815B_I2C_ADDR,SERIAL_CONTROL,bitrate);
                   break;
           case TW2815_GET_ADA_BITRATE:
		                ada_bitrate = tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_CONTROL);
				if((ada_bitrate & 0x10)==0x10)
                                {
                                    ada_bitrate = SET_384_BITRATE;
                                }
                                else
                                {
                                    ada_bitrate = SET_256_BITRATE;
                                }
                                copy_to_user(argp,&ada_bitrate,sizeof(ada_bitrate));
				break;     
             case TW2815_SET_D1:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_D1_FAIL;
                }
                setd1(TW2815B_I2C_ADDR);
                break;
                
             case TW2815_SET_2_D1:
                if (copy_from_user(&tw2815_2d1, argp,sizeof(tw2815_2d1)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_2_D1_FAIL;
                }
                set_2_d1(TW2815B_I2C_ADDR,tw2815_2d1.ch1,tw2815_2d1.ch2);
                break;
                
             case TW2815_SET_4HALF_D1:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_4HALF_D1_FAIL;
                }
                set_4half_d1(TW2815B_I2C_ADDR,tmp);
                break;
                
             case TW2815_SET_4_CIF: 
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_4_CIF_FAIL;
                }
                set_4cif(TW2815B_I2C_ADDR,tmp);
                break;
                
             case  TW2815_SET_AUDIO_OUTPUT:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_OUTPUT_FAIL;
                }
                if((tmp<=0)||(tmp>16))
                {
                	return TW2815_SET_AUDIO_OUTPUT_FAIL;
                }
                set_audio_output(TW2815B_I2C_ADDR,tmp);
                break;
                
             case TW2815_GET_AUDIO_OUTPUT:
                 tmp = tw2815_byte_read(TW2815B_I2C_ADDR,0x63);
                 if((tmp & 0x03)==0x0)
                 {
                     tmp_help = 2;
                 }
                 else if((tmp & 0x03)==0x1)
                 {
                     tmp_help =4;
                 }
                 else if((tmp & 0x03)==0x2)
                 {
                     tmp_help =8;
                 }
                 else if((tmp & 0x03)==0x3)
                 {
                     tmp_help =16;
                 }
                 copy_to_user(argp,&tmp_help,sizeof(tmp_help));
                break;
             case  TW2815_SET_AUDIO_MIX_OUT:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_MIX_OUT_FAIL;
                }
                set_audio_mix_out(TW2815B_I2C_ADDR,tmp);
                break;
                
             case TW2815_SET_AUDIO_RECORD_M:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_RECORD_M_FAIL;
                }
                set_audio_record_m(TW2815B_I2C_ADDR,tmp);
                break;
                
             case TW2815_SET_MIX_MUTE:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_MIX_MUTE_FAIL;
                }
		if(tmp<0||tmp>4)
                {
                  printk("TW2815B_SET_MIX_MUTE_FAIL\n");
                   return TW2815_SET_MIX_MUTE_FAIL;
                }

                set_audio_mix_mute(TW2815B_I2C_ADDR,tmp);
                break;
                
             case TW2815_CLEAR_MIX_MUTE:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_CLEAR_MIX_MUTE_FAIL;
                }
		 if(tmp<0||tmp>4)
                {
                   printk("TW2815B_CLEAR_MIX_MUTE_FAIL\n");
                   return TW2815_CLEAR_MIX_MUTE_FAIL;
                }

                clear_audio_mix_mute(TW2815B_I2C_ADDR,tmp);
                break;

             case TW2815_SET_VIDEO_MODE:
                 if (copy_from_user(&tw2815_videomode, argp, sizeof(tw2815_videomode)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tw2815_videomode.mode);
                    return TW2815_SET_VIDEO_MODE_FAIL;
                }
                if((tw2815_videomode.mode != NTSC)&&(tw2815_videomode.mode !=PAL)&&(tw2815_videomode.mode !=AUTOMATICALLY))
                {
                        printk("set video mode %d error\n ",tw2815_videomode.mode);
                        return TW2815_SET_VIDEO_MODE_FAIL;
                }
                tw2815_video_mode_init(TW2815B_I2C_ADDR,tw2815_videomode.mode,tw2815_videomode.ch);
               break;
             case TW2815_GET_VIDEO_MODE:
              
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_VIDEO_MODE_FAIL;
                }
	#if 0
              tmp_help = tw2815_byte_read(TW2815B_I2C_ADDR,tmp*0x10+0x1);
	      CLEAR_BIT(tmp_help,0x80);
	      tw2815_byte_write(TW2815B_I2C_ADDR,tmp*0x10+0x01,tmp_help);
	#endif    
	       tmp_help = (tw2815_byte_read(TW2815B_I2C_ADDR,tmp*0x10+0x0))>>5;
	      if(tmp_help <= 3)
	      	{
	      		tmp = PAL;
	      	}
	      else
	      	{
	      		tmp = NTSC;
	      	}
                copy_to_user(argp,&tmp,sizeof(tmp));
		break;
             case TW2815_REG_DUMP:
             tw2815_reg_dump(TW2815B_I2C_ADDR);
	    break;
             case TW2815_SET_CHANNEL_SEQUENCE:
             if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_CHANNEL_SEQUENCE_FAIL;
                }
                if((tmp>2)||(tmp<0))
                return TW2815_SET_CHANNEL_SEQUENCE_FAIL;
                channel_alloc(TW2815B_I2C_ADDR,tmp);
             break;
            case TW2815_SET_AUDIO_CASCAD:
                if (copy_from_user(&tmp, argp, sizeof(tmp)))
                {
                    printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                    return TW2815_SET_AUDIO_CASCAD_FAIL;
                }
                if(tmp==SET_AUDIO_CASCAD)
                {
                    set_audio_cascad();
                }
                else if(tmp == SET_AUDIO_SINGLE)
                {
                    set_single_chip();
                }
                else
		return TW2815_SET_AUDIO_CASCAD_FAIL;
	    break;
            case TW2815_HUE_CONTROL:
                if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_HUE_CONTROL_FAIL;
                } 
                hue_control(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue);
                break;
                case TW2815_GET_HUE_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_HUE_SET_FAIL;
                }
                 hue_control_get(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                case TW2815_SATURATION_CONTROL:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_SATURATION_CONTROL_FAIL;
                } 
                saturation_control(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue); 
                break;
                case TW2815_GET_SATURATION_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_SATURATION_SET_FAIL;
                }
                 saturation_control_get(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                break; 
                case TW2815_CONTRAST_CONTROL:
                  if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_CONTRAST_CONTROL_FAIL;
                }
                contrast_control(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue);

                break;
                case TW2815_GET_CONTRAST_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_CONTRAST_SET_FAIL;
                }
                contrast_control_get(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                break;

                case TW2815_BRIGHTNESS_CONTROL:
                   if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_BRIGHTNESS_CONTROL_FAIL;
                }
                   brightness_control(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue);
                break;
                case TW2815_GET_BRIGHTNESS_SET:
                 if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_BRIGHTNESS_SET_FAIL;
                }
                   brightness_control_get(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                   break; 
                case TW2815_LUMINANCE_PEAKING_CONTROL:
                    if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_LUMINANCE_PEAKING_FAIL;
                }
                    if(0 != luminance_peaking_control(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue))
                    {
                        return TW2815_LUMINANCE_PEAKING_FAIL;
                    }

                break;
                case TW2815_GET_LUMINANCE_PEAKING_SET:
                   if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                {
                      printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                      return TW2815_GET_LUMINANCE_PEAKING_SET_FAIL;
                }
                luminance_peaking_control_get(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);   
                copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));
                break;

                case TW2815_CTI_CONTROL:
                     if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                     {
                         printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                        return TW2815_CTI_CONTROL_FAIL;
                     }
                    if(0 != cti_control(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,tw2815_controlvalue.controlvalue))
                      {
                         return TW2815_CTI_CONTROL_FAIL;
                      }
                     
                break;
                case TW2815_GET_CTI_SET: 
                    if (copy_from_user(&tw2815_controlvalue, argp, sizeof(tw2815_controlvalue)))
                     {
                         printk("\ttw2815b_ERROR: WRONG cpy tmp is %x\n",tmp);
                        return TW2815_GET_CTI_SET_FAIL;
                     }
                    cti_control_get(TW2815B_I2C_ADDR,tw2815_controlvalue.ch,&tw2815_controlvalue.controlvalue);
                    copy_to_user(argp,&tw2815_controlvalue, sizeof(tw2815_controlvalue));  
                    break; 
                case TW2815_SET_PLAYBACK_MODE:
                   if (copy_from_user(&tmp, argp, sizeof(tmp))) 
                   {
                       printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                       return TW2815_SET_PLAYBACK_MODE_FAIL;
                   }
                 tmp &= 0x1;
                 tmp_help = (tw2815_byte_read(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL)&0xbf); 
                 tmp_help |=(tmp << 6);
                 tw2815_byte_write(TW2815B_I2C_ADDR,SERIAL_PLAYBACK_CONTROL,tmp_help); 
                case TW2815_SET_CLOCK_OUTPUT_DELAY:
                 if (copy_from_user(&tmp, argp, sizeof(tmp))) 
                   {
                       printk("\ttw2815a_ERROR: WRONG cpy tmp is %x\n",tmp);
                       return TW2815_SET_CLOCK_OUTPUT_DELAY_FAIL;
                   }
                  tw2815_byte_write(TW2815B_I2C_ADDR,0x4d,tmp);
           
                default:
                break;
	    }
                                    
    return TW2815_IOCTL_OK;
}


/*
 *      The various file operations we support.
 */
static struct file_operations tw2815a_fops = {
    .owner      = THIS_MODULE,
    .read       = tw2815a_read,
    .write      = tw2815a_write,
    .ioctl      = tw2815a_ioctl,
    .open       = tw2815a_open,
    .release    = tw2815a_close
};


static struct miscdevice tw2815a_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "tw2815adev",
   .fops  = &tw2815a_fops,
};



/*
 *      The various file operations we support.
 */
static struct file_operations tw2815b_fops = {
    .owner      = THIS_MODULE,
    .read       = tw2815b_read,
    .write      = tw2815b_write,
    .ioctl      = tw2815b_ioctl,
    .open       = tw2815b_open,
    .release    = tw2815b_close
};

static struct miscdevice tw2815b_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "tw2815bdev",
   .fops  = &tw2815b_fops,
};

DECLARE_KCOM_GPIO_I2C();
static int __init tw2815_init(void)
{
    int ret = 0;
    
    ret = KCOM_GPIO_I2C_INIT();
    if(ret)
    {
        printk("GPIO I2C module is not load.\n");
        return -1;
    }

#if 1
    /*register tw2815a_dev*/
    ret = misc_register(&tw2815a_dev);
    printk("TW2815 driver init start ... \n");
    if (ret)
    {
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2815a_ERROR: could not register tw2815a devices. \n");
        return -1;
    }
   if ((tw2815_device_video_init(TW2815A_I2C_ADDR,AUTOMATICALLY) < 0)||(tw2815_device_audio_init(TW2815A_I2C_ADDR)<0))
    {
        misc_deregister(&tw2815a_dev);
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2815a_ERROR: tw2815a driver init fail for device init error!\n");
        return -1;
    }

    printk("tw2815a driver init successful!\n");

#endif
#if 1

    /*register tw2815b_dev*/
    ret = misc_register(&tw2815b_dev);
    if (ret)
    {
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2815b_ERROR: could not register tw2815b devices. \n");
        return -1;
    }
   if ((tw2815_device_video_init(TW2815B_I2C_ADDR,AUTOMATICALLY) < 0)||(tw2815_device_audio_init(TW2815B_I2C_ADDR)<0))
    {
        misc_deregister(&tw2815b_dev);
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2815b_ERROR: tw2815b driver init fail for device init error!\n");
        return -1;
    }
    printk("tw2815b driver init successful!\n");
#endif

    return ret;
}



static void __exit tw2815_exit(void)
{
    misc_deregister(&tw2815a_dev);
    misc_deregister(&tw2815b_dev);
    KCOM_GPIO_I2C_EXIT();
}

module_init(tw2815_init);
module_exit(tw2815_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");

