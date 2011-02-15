/**
 * @addtogroup platform_src_sirf_sirfnaviii
 * @{
 */

/*
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2005-2008 by SiRF Technology, Inc.  All rights reserved.
 *
 *    This Software is protected by United States copyright laws and
 *    international treaties.  You may not reverse engineer, decompile
 *    or disassemble this Software.
 *
 *    WARNING:
 *    This Software contains SiRF Technology Inc.’s confidential and
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
 *    Software without SiRF Technology, Inc.’s  express written
 *    permission.   Use of any portion of the contents of this Software
 *    is subject to and restricted by your signed written agreement with
 *    SiRF Technology, Inc.
 *
 */

/**
 * @file   sirfnav_config.h
 *
 * @brief  SiRFNav configuration defines.
 */

#ifndef SIRFNAV_CONFIG_H_INCLUDED
#define SIRFNAV_CONFIG_H_INCLUDED



/* ----------------------------------------------------------------------------
 *    Preprocessor Definitions
 * ------------------------------------------------------------------------- */



/* Thread IDs */
#define SIRFNAV_THREAD_NAV       0x0020
#define SIRFNAV_THREAD_TRK_COM   0x0021


/* Thread stack sizes */
#define SIRFNAV_THREAD_NAV_STACK_SIZE      32768
#define SIRFNAV_THREAD_TRK_COM_STACK_SIZE  16384

/* Aggregate stack size for SiRFNav */
#define SIRFNAV_STACK_MAX  (SIRFNAV_THREAD_NAV_STACK_SIZE + SIRFNAV_THREAD_TRK_COM_STACK_SIZE)

/* Number of mutexes */
#define SIRFNAV_MUTEX_MAX             16

/* Number of semaphores */
#define SIRFNAV_SEM_MAX               16



#endif /* !SIRFNAV_CONFIG_H_INCLUDED */

/**
 * @}
 */


