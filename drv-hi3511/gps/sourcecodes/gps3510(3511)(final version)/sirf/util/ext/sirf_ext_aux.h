/**
 * @addtogroup platform_src_sirf_util_ext
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *  Copyright (c) 2005-2009 by SiRF Technology, Inc.  All rights reserved. *
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
 * MODULE:  HOST
 *
 * FILENAME:  sirf_ext_aux.h
 *
 * DESCRIPTION: This file include the data structures and the functions to
 *              implement the registration of protocols with UI mdoule
 *
 ***************************************************************************/

#ifndef __SIRF_EXT_AUX_H__
#define __SIRF_EXT_AUX_H__

/***************************************************************************
   Include files
***************************************************************************/

#include "sirf_types.h"
#include "sirf_ext.h"

/***************************************************************************
  Defines
***************************************************************************/

#define SIRF_EXT_AUX_MAX_MESSAGE_LEN   1024
#define SIRF_EXT_AUX_DEFAULT_BAUD_RATE 115200

/* Thread IDs */
#define SIRF_EXT_AUX_THREAD_1   0x0030
#define SIRF_EXT_AUX_THREAD_2   0x0031
#define SIRF_EXT_AUX_THREAD_3   0x0032

/* Thread stack sizes */
#ifdef SIRF_EXT_AUX
#define SIRF_EXT_AUX_THREAD_1_STACK_SIZE  ( 32 * 1024 )
#else
#define SIRF_EXT_AUX_THREAD_1_STACK_SIZE  0
#endif

#ifdef SIRF_EXT_AUX_2
#define SIRF_EXT_AUX_THREAD_2_STACK_SIZE  ( 32 * 1024 )
#else
#define SIRF_EXT_AUX_THREAD_2_STACK_SIZE  0
#endif

#ifdef SIRF_EXT_AUX_3
#define SIRF_EXT_AUX_THREAD_3_STACK_SIZE  ( 32 * 1024 )
#else
#define SIRF_EXT_AUX_THREAD_3_STACK_SIZE  0
#endif

#define SIRF_EXT_AUX_THREAD_SIZE (SIRF_EXT_AUX_THREAD_1_STACK_SIZE + \
                                  SIRF_EXT_AUX_THREAD_2_STACK_SIZE + \
                                  SIRF_EXT_AUX_THREAD_3_STACK_SIZE)

/* Return values */
#define SIRF_EXT_AUX_ALREADY_RUNNING   0xC000
#define SIRF_EXT_AUX_NOT_RUNNING       0xC001


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Prototype Definitions
***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Create(tSIRF_UINT32 port, tSIRF_UINT32 baud_rate);
tSIRF_RESULT SIRF_EXT_AUX_Delete( tSIRF_VOID );

tSIRF_RESULT SIRF_EXT_AUX_Open(   tSIRF_UINT8 auxno, tSIRF_UINT32 port, tSIRF_UINT32 baud_rate );
tSIRF_RESULT SIRF_EXT_AUX_Close(  tSIRF_UINT8 auxno );
tSIRF_UINT32 SIRF_EXT_AUX_Reopen( tSIRF_UINT8 auxno );

tSIRF_RESULT SIRF_EXT_AUX_Send( tSIRF_UINT8 auxno, tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length );
tSIRF_RESULT SIRF_EXT_AUX_Send_Passthrough( tSIRF_UINT8 auxno, tSIRF_VOID *PktBuffer, tSIRF_UINT32 PktLen );

tSIRF_VOID SIRF_EXT_AUX_Callback_Register( t_data_callback_func callback_func );

#ifdef __cplusplus
}
#endif


#endif /* __SIRF_EXT_AUX_H__ */

/**
 * @}
 */

