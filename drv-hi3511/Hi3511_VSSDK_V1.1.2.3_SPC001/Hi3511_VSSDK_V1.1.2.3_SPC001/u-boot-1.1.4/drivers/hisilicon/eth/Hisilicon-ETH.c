/*
 * Cirrus Logic CS8900A Ethernet
 *
 * (C) 2003 Wolfgang Denk, wd@denx.de
 *     Extension to synchronize ethaddr environment variable
 *     against value in EEPROM
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 1999 Ben Williamson <benw@pobox.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is loaded into SRAM in bootstrap mode, where it waits
 * for commands on UART1 to read and write memory, jump to code etc.
 * A design goal for this program is to be entirely independent of the
 * target board.  Anything with a CL-PS7111 or EP7211 should be able to run
 * this code in bootstrap mode.  All the board specifics can be handled on
 * the host.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <command.h>
#include <config.h>


#include "Hisilicon-ETH.h"
#include "ETH_Reg.h"
#include "ETH_Struct.h"
#include "ETH_TypeDef.h"
#include <net.h>
#include <linux-adapter.h>
extern UINT32 ETH_FrameTransmit (In UINT16 u16FrameLen, In UINT8 *pu8FrameTransmitData) ;
extern UINT32 ETH_FrameReceive (Out UINT8 *pu8FrameReceiveData ,Out UINT16 *pu16Length) ;
extern void ETH_SET_MAC(unsigned char*mac);
extern UINT32 ETH_Init(void);

#if (CONFIG_COMMANDS & CFG_CMD_NET)

#undef DEBUG

#define MAC_LEN 6

#define print_mac(mac) 	do{ int i;\
	printf("MAC:  ");\
	for (i = 0; i < MAC_LEN; i++)\
		printf("%c%02X", i ? '-' : ' ', *(((unsigned char*)mac)+i));\
	printf("\n");\
	}while(0)

int string_to_mac(unsigned char *mac, char* s)
{
	int i;
	char *e;
	int ret = 0;

	if(s == NULL)
		return 1;

	for (i=0; i<MAC_LEN; ++i) {
		mac[i] = simple_strtoul(s, &e, 16);
		if (s) {
			s = (*e) ? e+1 : e;
		}

		if((*(((unsigned char*)mac)+i) == 0)){
			ret += 1;
		}
	}

	return (ret == MAC_LEN?1:0);
}

//added by wzh 2009-4-9
#define CONFIG_RANDOM_ETHADDR

static inline int is_multicast_ether_addr(const u8 *addr)
{
	return (0x01 & addr[0]);
}

static inline int is_zero_ether_addr(const u8 *addr)
{
	return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

static int is_valid_ether_addr(const u8 * addr)
{
	/* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
	 * explicitly check for it here. */
	return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}

static void print_mac_address(const char *pre_msg, const unsigned char *mac, const char *post_msg)
{
	int i;
	
	if(pre_msg)
		printf(pre_msg);

	for(i=0; i<6; i++)
		printf("%02X%s", mac[i], i==5 ? "" : ":");

	if(post_msg)
		printf(post_msg);
}

#ifdef CONFIG_RANDOM_ETHADDR
static unsigned long rstate = 1;
/* trivial congruential random generators. from glibc. */
void srandom(unsigned long seed)
{
	rstate = seed ? seed : 1;  /* dont allow a 0 seed */
}

unsigned long random(void)
{
  unsigned int next = rstate;
  int result;

  next *= 1103515245;
  next += 12345;
  result = (unsigned int) (next / 65536) % 2048;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  rstate = next;

  return result;
}


void random_ether_addr(char *mac)
{
    unsigned long ethaddr_low, ethaddr_high;

    srandom(get_timer(0) );

    /*
     * setting the 2nd LSB in the most significant byte of
     * the address makes it a locally administered ethernet
     * address
     */
    ethaddr_high = (random() & 0xfeff) | 0x0200;
    ethaddr_low = random();

    mac[0] = ethaddr_high >> 8;
    mac[1] = ethaddr_high & 0xff;
    mac[2] = ethaddr_low >>24;
    mac[3] = (ethaddr_low >> 16) & 0xff;
    mac[4] = (ethaddr_low >> 8) & 0xff;
    mac[5] = ethaddr_low & 0xff;    

    mac [0] &= 0xfe;	/* clear multicast bit */
    mac [0] |= 0x02;	/* set local assignment bit (IEEE802) */
    
 }

static int set_random_mac_address(unsigned char * mac, unsigned char * ethaddr)
{
	random_ether_addr(mac);
	print_mac_address( "Set Random MAC address: ", mac, "\n");

	sprintf (ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	setenv("ethaddr",ethaddr);

	ETH_SET_MAC(mac);

}
#endif


void init_eth_mac_env(void)
{
	unsigned char mac[MAC_LEN];
	unsigned char ethaddr[20];
	char* s=getenv("ethaddr");

	if(!s){
		printf("none ethaddr. \n"); 
		#ifdef CONFIG_RANDOM_ETHADDR
			set_random_mac_address(mac, ethaddr);
		#endif
		return 0;
	}

	memset(mac,0,MAC_LEN);

	string_to_mac(mac,s);
	if(!is_valid_ether_addr(mac))
	{
		printf("MAC address invalid!\n");
		#ifdef CONFIG_RANDOM_ETHADDR
			set_random_mac_address(mac, ethaddr);
		#endif
		return 0;
	}

	print_mac(mac);
	ETH_SET_MAC(mac);
}

//added by wzh 2009-4-9
void eth_init_reset(void)
{
	eth_soft_reset();
}

	 

void ETH_SoftResetPort(In UINT32 u32PortReset)
{
}

static void eth_reset (void)
{
	ETH_SoftResetPort(1);
	ETH_SoftResetPort(0);
}

static void eth_reginit (void)
{
     UINT32 ulIntEn = 0;

		/*disable the interrupt*/
	ulIntEn = *((volatile UINT32 *) ETH_GLB_REG(1));
	*((volatile UINT32 *) ETH_GLB_REG(1)) = (ulIntEn & (0x0000));
	

}


void eth_halt (void)
{
	ETH_SoftResetPort(1);
}

extern void	hisf_set_mac_addr( char* addr );
int eth_init (bd_t * bd)
{
	dbg_info("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);

	ETH_Init();
	init_eth_mac_env();

	return 0;
}

	UINT16 rxlen=0;

/* Get a data block via Ethernet */
extern int eth_rx (void)
{

	dbg_info("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);

       if( ETH_FrameReceive((UINT8 *)NetRxPackets[0],&rxlen) ==1 )
	/* Pass the packet up to the protocol layers. */
	NetReceive (NetRxPackets[0], rxlen);

	return rxlen;
}

/* Send a data block via Ethernet. */
extern int eth_send (volatile void *packet, int length)
{
//	int i =0;
	
	dbg_info("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);

	ETH_FrameTransmit((UINT16)length+4,(UINT8 *)packet);	//add 4 as crc check
	return 0;
}

#endif	/* COMMANDS & CFG_NET */


