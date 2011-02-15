/**
 * @addtogroup platform_src_sirf_sirfnaviii
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005-2009 by SiRF Technology, Inc. All rights reserved.*
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.’s confidential and       *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.’s  express written            *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 ***************************************************************************
 *
 * FILE:  sirfnav_ui_ctrl.h
 *
 ***************************************************************************/

#ifndef __SIRFNAV_UI_CTRL_H__
#define __SIRFNAV_UI_CTRL_H__

#include "sirf_types.h"


/* SiRFNavIII control interface ============================================ */


/* SiRFNav_Start() and SiRFNav_Stop() return values ------------------------ */
#define SIRFNAV_UI_CTRL_ALREADY_STARTED             0x2501
#define SIRFNAV_UI_CTRL_ALREADY_STOPPED             0x2502
#define SIRFNAV_UI_CTRL_INVALID_MODE                0x2503
/* 0x2504 is reserved */
#define SIRFNAV_UI_CTRL_ERROR_OPENING_PORT          0x2505
#define SIRFNAV_UI_CTRL_OS_ERROR                    0x2506
#define SIRFNAV_UI_CTRL_INVALID_STORAGE_MODE        0x2507
/* 0x2508 is reserved */
#define SIRFNAV_UI_CTRL_INVALID_PORT_BAUD_RATE      0x2509
/***************************************************************************
 *   Control Start Mode Bit Field Definitions - Refer to the ICD
***************************************************************************/

/* Bits 0 - 3 are for the Start Type */
#define SIRFNAV_UI_CTRL_MODE_AUTO                   0x00000000
#define SIRFNAV_UI_CTRL_MODE_HOT                    0x00000001
#define SIRFNAV_UI_CTRL_MODE_WARM                   0x00000002
#define SIRFNAV_UI_CTRL_MODE_COLD                   0x00000003
#define SIRFNAV_UI_CTRL_MODE_FACTORY                0x00000004
#define SIRFNAV_UI_CTRL_MODE_TEST                   0x00000005
#define SIRFNAV_UI_CTRL_MODE_MASK                   0x0000000F

/* Bits 4 and 5 are for the On hw lines usage */
#define SIRFNAV_UI_CTRL_MODE_HW_ON_MASK             0x00000030
#define SIRFNAV_UI_CTRL_MODE_HW_ON_GPIO             0x00000000
#define SIRFNAV_UI_CTRL_MODE_HW_ON_UART_RX          0x00000010
#define SIRFNAV_UI_CTRL_MODE_HW_ON_UART_CTS         0x00000020
#define SIRFNAV_UI_CTRL_MODE_HW_ON_SPI_SS           0x00000030

/* Bits 6 is reserved */
#define SIRFNAV_UI_CTRL_MODE_BIT6_MASK              0x00000040

/* Bit 7 */
#define SIRFNAV_UI_CTRL_MODE_HOST_WAKEUP_PREAMBLE   0x00000080

/* Bits 8 and 9 */
#define SIRFNAV_UI_CTRL_MODE_FORCE_CLOCK_OFFSET     0x00000100
#define SIRFNAV_UI_CTRL_MODE_USE_TRACKER_RTC        0x00000200

/* Bits 10 - 12 are used for the Storage Mode */
#define SIRFNAV_UI_CTRL_MODE_STORAGE_NONE           0x00000400
#define SIRFNAV_UI_CTRL_MODE_STORAGE_PERIODIC_ALL   0x00000800
#define SIRFNAV_UI_CTRL_MODE_STORAGE_ON_DEMAND      0x00000C00
#define SIRFNAV_UI_CTRL_MODE_STORAGE_ON_STOP        0x00001000
#define SIRFNAV_UI_CTRL_MODE_STORAGE_MASK           0x00001C00

/* Bit 13 is the hardware flow control setting */
#define SIRFNAV_UI_CTRL_MODE_HW_FLOW_CTRL_MASK      0x00002000
#define SIRFNAV_UI_CTRL_MODE_HW_FLOW_CTRL_OFF       0x00000000
#define SIRFNAV_UI_CTRL_MODE_HW_FLOW_CTRL_ON        0x00002000

/* Bit 14 is reserved */
#define SIRFNAV_UI_CTRL_MODE_LNA_TYPE_OFFSET        14
#define SIRFNAV_UI_CTRL_MODE_LNA_TYPE_MASK          0x00004000
#define SIRFNAV_UI_CTRL_MODE_EXT_LNA_ON             0x00004000
#define SIRFNAV_UI_CTRL_MODE_EXT_LNA_OFF            0x00000000

/* Bits 15 - 17 */
#define SIRFNAV_UI_CTRL_MODE_TEXT_ENABLE            0x00008000
#define SIRFNAV_UI_CTRL_MODE_RAW_MSG_ENABLE         0x00010000
#define SIRFNAV_UI_CTRL_MODE_ECHO_DISABLE           0x00020000

/* Bits 18 - 22 are for the TCXO warmup delay */
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_OFFSET      18
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_MASK        0x007C0000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_NONE        0x00000000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_100MS       0x00040000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_200MS       0x00080000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_300MS       0x000C0000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_400MS       0x00100000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_500MS       0x00140000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_600MS       0x00180000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_700MS       0x001C0000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_800MS       0x00200000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_900MS       0x00240000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1000MS      0x00280000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1100MS      0x002C0000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1200MS      0x00300000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1300MS      0x00340000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1400MS      0x00380000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1500MS      0x003C0000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1600MS      0x00400000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1700MS      0x00440000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1800MS      0x00480000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_1900MS      0x004C0000
#define SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_2000MS      0x00500000

/* Bit 23 for internal use only */
#define SIRFNAV_UI_CTRL_MODE_BIT23_MASK             0x00800000

/* Bits 24 - 31 are used for the Initial Clock Uncertainty */
#define SIRFNAV_UI_CTRL_MODE_CLK_UNCERTAINTY_OFFSET    24
#define SIRFNAV_UI_CTRL_MODE_CLK_UNCERTAINTY_PPM(ppm)  (ppm << SIRFNAV_UI_CTRL_MODE_CLK_UNCERTAINTY_OFFSET)
#define SIRFNAV_UI_CTRL_MODE_CLK_UNCERTAINTY_MASK      0xFF000000

#define SIRFNAV_UI_CTRL_DEFAULT_BAUD_RATE 115200

/***************************************************************************
 *   SiRFNav_Stop() stop_mode values
***************************************************************************/

#define SIRFNAV_UI_CTRL_STOP_MODE_TERMINATE   (0)  /* All GPS threads will be deleted and     */
                                                   /* communication interface closed.         */
#define SIRFNAV_UI_CTRL_STOP_MODE_SUSPEND     (1)  /* GPS activity will be suspended but GPS  */
                                                   /* threads will not be deleted; tracker    */
                                                   /* communication interface will not be     */
                                                   /* closed                                  */
#define SIRFNAV_UI_CTRL_STOP_MODE_FREEZE      (2)  /* GPS will be notified to make a restart  */
                                                   /* as soon as the CPU is back available.   */
                                                   /* Not any OS system function call nor GPS */
                                                   /* data storage call is made during this   */
                                                   /* SiRFNav_Stop execution                  */


/* Prototypes -------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

tSIRF_RESULT SiRFNav_Start( tSIRF_UINT32 start_mode, tSIRF_UINT32 clock_offset, tSIRF_UINT32 port_num, tSIRF_UINT32 baud_rate );

tSIRF_RESULT SiRFNav_Stop(  tSIRF_UINT32 stop_mode );


#ifdef __cplusplus
}
#endif

#endif /* __SIRFNAV_UI_CTRL_H__ */

/**
 * @}
 */

