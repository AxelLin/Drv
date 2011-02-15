/**
 * @addtogroup platform_src_sirf_sirfnaviii
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005-2008 by SiRF Technology, Inc. All rights reserved.*
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
 * FILE: sirfnav_ui_io.h
 *
 ***************************************************************************/

#ifndef __SIRFNAV_UI_IO_H__
#define __SIRFNAV_UI_IO_H__


#include "sirf_types.h"


/* SiRFNavIII input/output interface =======================================*/


/* SiRFNav_Input() return values -------------------------------------------*/
#define SIRFNAV_UI_IO_ERROR_INVALID_MSG_LENGTH   0x2101
#define SIRFNAV_UI_IO_ERROR_QUEUE_ERROR          0x2102
#define SIRFNAV_UI_IO_ERROR_NULL_POINTER         0x2103
#define SIRFNAV_UI_ERROR_NOT_STARTED             0x2104


/* Prototypes --------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

tSIRF_RESULT SiRFNav_Input( tSIRF_UINT32 message_id,
                            tSIRF_VOID  *message_structure,
                            tSIRF_UINT32 message_length );

/* SiRFNav_Output() is a callback exported from the user code */
tSIRF_VOID SiRFNav_Output( tSIRF_UINT32 message_id,
                           tSIRF_VOID  *message_structure,
                           tSIRF_UINT32 message_length );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SIRFNAV_UI_IO_H__ */

/**
 * @}
 */

