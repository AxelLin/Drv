/**
 * @addtogroup app
 * @{
 */

 /***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2007 by SiRF Technology, Inc.  All rights reserved.    *
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
 * FILENAME: sirf_threads.c
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Yaxon/Software/app/SiRFNavIIITestApp/Linux_ARM_gcc/main/sirf_threads.c $
 *
 *  $DateTime: 2009/05/20 20:11:21 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#include "sirf_pal_config.h"
#include "sirf_threads.h"

#include "sirfnav_config.h"

#ifdef SIRF_CLM
   #include "sirfclm_config.h"
#endif

#ifdef SIRF_EXT_AUX
   #include "sirf_ext_aux.h"
#endif

#if defined (SIRFNAV_DEMO) || defined (SIRF_COMPACT_DEMO)
   #include "sirfnav_demo_config.h"
#endif


/**
 * Thread definition table ----------------------------------------------------
 */

tSIRF_THREAD_TABLE   SIRF_THREAD_Table[] =
{
   /**
    * THREAD ID                PRIORITY                   QUANTUM  STACK SIZE
    */
   { SIRFNAV_THREAD_NAV,       25,       0,      0 },
   { SIRFNAV_THREAD_TRK_COM,   50,      0,      0 },

#ifdef SIRF_CLM
   { SIRFCLM_THREAD_CGEE_PREDICTION,  THREAD_PRIORITY_BELOW_NORMAL, 0,      0 },
#endif

#if defined (SIRFNAV_DEMO)
   #ifdef SIRF_EXT_AUX
   { SIRF_EXT_AUX_THREAD_1,    25, 0,      0 },
   { SIRF_EXT_AUX_THREAD_2,    25, 0,      0 },
   { SIRF_EXT_AUX_THREAD_3,    25, 0,      0 },
   #endif
#endif

};


/**
 *=============================================================================
 *
 *-----------------------------------------------------------------------------
 */
tSIRF_UINT32 SIRF_THREAD_MaxThreads( tSIRF_VOID )
{

   return sizeof(SIRF_THREAD_Table) / sizeof(tSIRF_THREAD_TABLE);

} /* SIRF_Threads_Max() */

/**
 * @}
 * End of file.
 */

