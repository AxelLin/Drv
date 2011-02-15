/**
 * @addtogroup platform_src_sirf_sirfnav_demo
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
 * @file   SiRFNav_Demo_config.h
 *
 * @brief  SiRFNav Demo Configuration.
 */

#ifndef SIRFNAV_DEMO_CONFIG_H_INCLUDED
#define SIRFNAV_DEMO_CONFIG_H_INCLUDED

/* Includes and Configuration for Ext Aux */

#ifdef SIRF_EXT_AUX
#include "sirf_ext_aux.h"
#else
#define SIRF_EXT_AUX_THREAD_SIZE 0
#endif

/* Includes and Configuration for stress tests */

#ifdef SIRF_STRESS_TEST
#include "sirf_stress_test.h"
#else
#define SIRF_STRESS_THREAD_STACK_SIZE 0
#endif

/* ----------------------------------------------------------------------------
 *   Preprocessor Definitions
 * ------------------------------------------------------------------------- */

/* Test App: */
// Numbers 0x30 to 0x35 are resevred for EXT AUX module
#define SIRFNAV_DEMO_THREAD_TRANSMIT  0x0036
#define SIRFNAV_DEMO_THREAD_CLIENT    0x0037
#define SIRFNAV_DEMO_THREAD_EXT1      0x0038
#define SIRFNAV_DEMO_THREAD_IDLE      0x0039
#define SIRFNAV_DEMO_THREAD_MAIN      0x003A

/* Aggregate stack size for demo */
#define SIRFNAV_DEMO_STACK_MAX  (SIRF_EXT_AUX_THREAD_SIZE + SIRF_STRESS_THREAD_STACK_SIZE)

/** Number of mutexes */
#ifndef SIRFNAV_DEMO_MUTEX_MAX
#define SIRFNAV_DEMO_MUTEX_MAX  16
#endif
/** Number of semaphores: */
#ifndef SIRFNAV_DEMO_SEM_MAX
#define SIRFNAV_DEMO_SEM_MAX    16
#endif


#endif /* !SIRFNAV_DEMO_CONFIG_H_INCLUDED */

/**
 * @}
 */


