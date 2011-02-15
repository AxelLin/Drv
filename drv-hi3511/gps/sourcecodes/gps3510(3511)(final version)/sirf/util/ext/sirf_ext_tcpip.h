/**
 * @addtogroup platform_src_sirf_util_ext
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *  Copyright (c) 2005-2008 by SiRF Technology, Inc.  All rights reserved. *
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
 * FILENAME:  sirf_ext_tcpip.h
 *
 * DESCRIPTION: This file include the data structures and the functions to
 *              implement the registration of protocols with UI mdoule
 *
 ***************************************************************************/

#ifndef __SIRF_EXT_TCPIP_H__
#define __SIRF_EXT_TCPIP_H__

#if defined(SIRF_EXT_TCPIP)||defined(EPH_AIDING_DEMO)

#include "sirf_types.h"
#include "sirf_ext.h"

#define SIRF_EXT_TCPIP_MAX_MESSAGE_LEN 1024

/* tcpip_addr: use NULL to create listening socket */
tSIRF_RESULT SIRF_EXT_TCPIP_Create( tSIRF_CHAR *tcpip_addr, tSIRF_UINT16 tcpip_port ); 
tSIRF_RESULT SIRF_EXT_TCPIP_Delete( tSIRF_VOID );

tSIRF_RESULT SIRF_EXT_TCPIP_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length );
tSIRF_RESULT SIRF_EXT_TCPIP_Send_Passthrough( tSIRF_VOID *message, tSIRF_UINT32 length );

tSIRF_RESULT SIRF_EXT_TCPIP_VerifyConnection( tSIRF_VOID );

tSIRF_VOID SIRF_EXT_TCPIP_Callback_Register( t_data_callback_func callback_func );

#endif 

#endif /* __SIRF_EXT_TCPIP_H__ */

/**
 * @}
 * End of file.
 */

