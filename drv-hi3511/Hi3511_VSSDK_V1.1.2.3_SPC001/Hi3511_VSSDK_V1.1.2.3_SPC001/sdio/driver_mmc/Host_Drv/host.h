#ifndef SD_HOST_H
#define SD_HOST_H

#define _HI_3511_

#ifdef _HI_3560_

/*Detect card with GPIO, otherwise with registers in IP*/
#define _GPIO_CARD_DETECT_

/*Detect write protect of card with GPIO, otherwise with registers in IP*/
#define _GPIO_WR_PROTECT_

/*Enable power of card with GPIO, otherwise with registers in IP*/
#define _GPIO_POWEREN_

/*When power enale is open, explicitly turning on or off power is needed*/
#define _POWER_EN_NEED_

/*Base address of SD card register*/
#define SD_MMC_SDIO_BASE                     0x90020000

/*Base address of system control register*/
#define SCTL_BASE                                0x101e0000
#define SCTL_BASE_ADDR                       IO_ADDRESS(SCTL_BASE)

/*Address of switch register for multiplexing between SDIO and ETH */
#define SDIO_EN_SCTL_ADDR                    (SCTL_BASE_ADDR+0x040)
/*Bit of the switch register for multiplexing between SDIO and ETH */
#define SDIO_EN_SCTL_ADDR_LOC            27

/*Address of switch register for GPIO multiplexing between SDIO and other IPs */
#define SDIO_GPIO_SELECT                    (SCTL_BASE_ADDR+0x040)
/*The two bits of the switch register for GPIO multiplexing between SDIO and other IPs */
#define SDIO_GPIO_SELECT_LOC1            16
#define SDIO_GPIO_SELECT_LOC2            17

/*irq number*/
#define GPIO_INT_NUM                     8
#define MMC_INT_NUM                     24

/*Address of the PLL clock divided register for SDIO clock*/
#define SDIO_FDIV_SCTL                       (SCTL_BASE_ADDR+0x01c)
/*Bit of the PLL clock divided register for SDIO clock*/
#define SDIO_FDIV_SCTL_LOC                 26


/*Address of soft reset register for SDIO IP*/
#define SDIO_SOFT_RESET_SCTL               (SCTL_BASE_ADDR+0x01c)
/*Bit of soft reset register for SDIO IP*/
#define SDIO_SOFT_RESET_SCTL_LOC        25

#define GPIO_BASE                                IO_ADDRESS(0x101e4000)
#define GPIO_BASE_ADDR(x)                    (GPIO_BASE+x*0x1000)
/*Number of GPIO pin for GPIO card detect*/
#define CARD_DETECT_GPIO_NUM            22
/*Number of GPIO pin for GPIO power enble*/
#define CARD_POWER_EN_NUM            25
/*Number of GPIO pin for GPIO card write protect*/
#define CARD_WRITE_PRT_NUM            24

/*Group and bit of GPIO for GPIO card detect*/
#define CARD_DETECT_GPIO_GNUM            ((int)(CARD_DETECT_GPIO_NUM/8))
#define CARD_DETECT_GPIO_PNUM            (CARD_DETECT_GPIO_NUM%8)

/*Group and bit of GPIO for GPIO power enable*/
#define CARD_POWER_EN_GNUM               ((int)(CARD_POWER_EN_NUM/8))
#define CARD_POWER_EN_PNUM               (CARD_POWER_EN_NUM%8)

/*Group and bit of GPIO for GPIO card write protect*/
#define CARD_WRITE_PRT_GNUM              ((int)(CARD_WRITE_PRT_NUM/8))
#define CARD_WRITE_PRT_PNUM              (CARD_WRITE_PRT_NUM%8)
//#define CARD_DETECT_GPIO_DIR             GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x400
//#define CARD_POWEREN_WRITEPRT_DIR     GPIO_BASE_ADDR(CARD_POWER_EN_GNUM)+0x400

/*To set of GPIO direction*/
#define _GPIO_SET_DIR_(DIR,GROUP,BIT) do    {    \
    unsigned char result;    \
    result=(*((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+0x400)));    \
    result|=(DIR << BIT);        \
    *((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+0x400)) = result;        \
}while(0)

/*To read a certain bit of GPIO*/
#define _GPIO_RDBIT_(GROUP,BIT) ((*((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+(0x1<<(BIT+2)))) >> BIT)&1)

/*To write a certain bit of GPIO*/
#define _GPIO_WRBIT_(VALUE,GROUP,BIT) do {        \
    unsigned char result;        \
    result=(*((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+(0x1<<(BIT+2)))));    \
    result|=(VALUE<<BIT);    \
    *((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+(0x1<<(BIT+2))))=result;    \
}while(0)

//#define GPIO_CARD_DETECT_IN                HISILICON_GPIO_SET_BITDIR(0, CARD_DETECT_GPIO_GNUM, CARD_DETECT_GPIO_PNUM)
#define GPIO_CARD_DETECT_IN                _GPIO_SET_DIR_(0, CARD_DETECT_GPIO_GNUM, CARD_DETECT_GPIO_PNUM)
#define GPIO_CARD_DETECT_READ            _GPIO_RDBIT_(CARD_DETECT_GPIO_GNUM, CARD_DETECT_GPIO_PNUM)
#define GPIO_WRITE_PRT_IN                _GPIO_SET_DIR_(0, CARD_WRITE_PRT_GNUM, CARD_WRITE_PRT_PNUM)
#define GPIO_WRITE_PRT_READ            _GPIO_RDBIT_(CARD_WRITE_PRT_GNUM, CARD_WRITE_PRT_PNUM)
#define GPIO_POWER_EN_OUT                _GPIO_SET_DIR_(1, CARD_POWER_EN_GNUM, CARD_POWER_EN_PNUM)
#define GPIO_POWER_EN_ENABLE            _GPIO_WRBIT_(1, CARD_POWER_EN_GNUM, CARD_POWER_EN_PNUM)
#define GPIO_POWER_EN_DISABLE            _GPIO_WRBIT_(0, CARD_POWER_EN_GNUM, CARD_POWER_EN_PNUM)
#endif

#ifdef _HI_3511_

/*Detect card with GPIO, otherwise with registers in IP*/
#define _GPIO_CARD_DETECT_

/*Detect write protect of card with GPIO, otherwise with registers in IP*/
#define _GPIO_WR_PROTECT_

/*Base address of SD card register*/
#define SD_MMC_SDIO_BASE                    0x90020000

/*Base address of system control register*/
#define SCTL_BASE                           0x101e0000
#define SCTL_BASE_ADDR                      IO_ADDRESS(SCTL_BASE)

/*Address of switch register for multiplexing between SDIO and ETH */
#define SDIO_EN_SCTL_ADDR                   (SCTL_BASE_ADDR+0x040)
/*Bit of the switch register for multiplexing between SDIO and ETH */
#define SDIO_EN_SCTL_ADDR_LOC               27

/*Address of switch register for GPIO multiplexing between SDIO and other IPs */
#define SDIO_GPIO_SELECT                    (SCTL_BASE_ADDR+0x040)
/*The two bits of the switch register for GPIO multiplexing between SDIO and other IPs */
#define SDIO_GPIO_SELECT_LOC1               16
#define SDIO_GPIO_SELECT_LOC2               17

/*Irq number*/
#define GPIO_INT_NUM                     8
#define MMC_INT_NUM                     24


/*
00£º8 divided PLL output clock
01£º10 divided PLL output clock
10£º12 divided PLL output clock
11£º14 divided PLL output clock
*/
#define SDIO_FDIV_SCTL                      (SCTL_BASE_ADDR+0x034)
#define SDIO_FDIV_SCTL_LOC                  4

/*Address of soft reset register for SDIO IP*/
#define SDIO_SOFT_RESET_SCTL                (SCTL_BASE_ADDR+0x01c)
#define SDIO_SOFT_RESET_SCTL_LOC            9

#define GPIO_BASE                           IO_ADDRESS(0x101F3000)
#define GPIO_BASE_ADDR(x)                   (GPIO_BASE+x*0x1000)

/*Group and bit of GPIO for GPIO card detect*/
#define CARD_DETECT_GPIO_GNUM               7
#define CARD_DETECT_GPIO_PNUM               6

/*Group and bit of GPIO for GPIO card write protect*/
#define CARD_WRITE_PRT_GNUM                 7
#define CARD_WRITE_PRT_PNUM                 5

/*To set of GPIO direction*/
#define _GPIO_SET_DIR_(DIR,GROUP,BIT) do    {    \
    unsigned char result;    \
    result=(*((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+0x400)));    \
    result|=(DIR << BIT);        \
    *((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+0x400)) = result;        \
}while(0)

/*To read a certain bit of GPIO*/
#define _GPIO_RDBIT_(GROUP,BIT) ((*((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+(0x1<<(BIT+2)))) >> BIT)&1)

/*To write a certain bit of GPIO*/
#define _GPIO_WRBIT_(VALUE,GROUP,BIT) do {        \
    unsigned char result;        \
    result=(*((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+(0x1<<(BIT+2)))));    \
    result|=(VALUE<<BIT);    \
    *((volatile unsigned char *)(GPIO_BASE+0x1000*GROUP+(0x1<<(BIT+2))))=result;    \
}while(0)

#define GPIO_CARD_DETECT_IN             _GPIO_SET_DIR_(0, CARD_DETECT_GPIO_GNUM, CARD_DETECT_GPIO_PNUM)
#define GPIO_CARD_DETECT_READ           _GPIO_RDBIT_(CARD_DETECT_GPIO_GNUM, CARD_DETECT_GPIO_PNUM)
#define GPIO_WRITE_PRT_IN               _GPIO_SET_DIR_(0, CARD_WRITE_PRT_GNUM, CARD_WRITE_PRT_PNUM)
#define GPIO_WRITE_PRT_READ             _GPIO_RDBIT_(CARD_WRITE_PRT_GNUM, CARD_WRITE_PRT_PNUM)

#endif

#define ENABLE                   1
#define DISABLE                  0

#define sd_writeb(addr,val)      *((volatile unsigned char *)(addr))=(unsigned char)(val)
#define sd_readb(addr,result)    (unsigned char)(result)=*((volatile unsigned char *)(addr))
#define sd_writel(addr,val)      *((volatile unsigned long *)(addr))=(unsigned long)(val)
#define sd_readl(addr,result)    (unsigned long)(result)=*((volatile unsigned long *)(addr))

#endif
