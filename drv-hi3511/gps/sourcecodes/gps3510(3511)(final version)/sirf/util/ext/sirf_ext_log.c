/**
 * @addtogroup platform_src_sirf_util_ext
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
 * MODULE:  HOST
 *
 * FILENAME:  sirf_ext_log.c
 *
 * DESCRIPTION: Routines to log to local disk the I/O messages passed ro
 *              and from this program, for example when to connected to
 *              SiRFLoc Demo or its equivalent.
 *
  ***************************************************************************/

#ifdef SIRF_EXT_LOG

/***************************************************************************
 * Include Files
 ***************************************************************************/

#include <string.h>
#include <stdio.h>

#include "sirf_types.h"
#include "sirf_pal.h"
#include "sirf_ext_log.h"
#include "sirf_codec.h"
#include "sirf_proto_common.h"

#include "sirf_codec_ascii.h"
#include "sirf_codec_ssb.h"
#include "sirf_msg.h"

#include "sirf_pal_log.h"

/***************************************************************************
 * Global variables
 ***************************************************************************/

tSIRF_EXT_LOG_TYPE log_type = SIRF_EXT_LOG_SIRF_ASCII_TEXT;


/* ----------------------------------------------------------------------------
 *    Local Variables
 * ------------------------------------------------------------------------- */

static tSIRF_LOG_HANDLE log_file = NULL;

/***************************************************************************
 * Description: 
 * Parameters:  
 * Returns:     
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_LOG_Open( tSIRF_EXT_LOG_TYPE type, tSIRF_CHAR *filename )
{
   tSIRF_RESULT        result;
   tSIRF_DATE_TIME     date_time;
   tSIRF_MSG_SSB_COMM_MESSAGE_TEXT  header;

   log_type = type;

   if ( log_file != NULL )
   {
      return SIRF_EXT_LOG_ALREADY_OPEN;
   }

   result = SIRF_PAL_LOG_Open(filename, &log_file, SIRF_PAL_LOG_MODE_OVERWRITE);
   if ( SIRF_SUCCESS != result )
   {
      return SIRF_PAL_LOG_OPEN_ERROR;
   }

   /* Write header (demo version number and time when opened): */
   result = SIRF_PAL_OS_TIME_DateTime( &date_time );
   if ( SIRF_SUCCESS == result )
   {
      snprintf( header.msg_text, sizeof(header.msg_text), "Log file opened: %02d/%02d/%02d %02d:%02d:%02d",
         date_time.month, date_time.day, date_time.year%100,
         date_time.hour, date_time.minute, date_time.second );

      SIRF_EXT_LOG_Send( SIRF_MSG_SSB_TEXT, (void*)&header, strlen(header.msg_text) );
   }

   return SIRF_SUCCESS;

} /* SIRF_EXT_LOG_Open() */


/***************************************************************************
 * Description: 
 * Parameters:  
 * Returns:     
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_LOG_Close( tSIRF_VOID )
{
   tSIRF_RESULT    result;
   tSIRF_DATE_TIME date_time;
   tSIRF_MSG_SSB_COMM_MESSAGE_TEXT  footer;

   if ( NULL == log_file ) 
   {
      return SIRF_EXT_LOG_ALREADY_CLOSED;
   }

   result = SIRF_PAL_OS_TIME_DateTime( &date_time );
   if ( SIRF_SUCCESS == result )
   {
      snprintf( footer.msg_text, sizeof(footer.msg_text), "Log file closed: %02d/%02d/%02d %02d:%02d:%02d",
         date_time.month, date_time.day, date_time.year%100,
         date_time.hour, date_time.minute, date_time.second );
   
      SIRF_EXT_LOG_Send( SIRF_MSG_SSB_TEXT, (void*)&footer, strlen(footer.msg_text) );
   }

   result = SIRF_PAL_LOG_Close( log_file );
   log_file = NULL;

   return result;

} /* SIRF_EXT_LOG_Close() */


/***************************************************************************
 * Description: 
 * Parameters:  
 * Returns:     
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_LOG_Send_Passthrough( tSIRF_VOID *PktBuffer, tSIRF_UINT32 PktLen )
{
   return SIRF_PAL_LOG_Write( log_file, (char *)PktBuffer, PktLen );

}  /* SIRF_EXT_LOG_Send_Passthrough */


/** 
 * Send a messsage to the log file.  Encodes and adds the protocol wrapper 
 * then calls SIRF_PAL_LOG_Write to store the data.
 * 
 * @param message_id         Message id
 * @param message_structure  pointer to the message structure
 * @param message_length     length of data pointed to by message_structure
 * 
 * @return SIRF_SUCCESS if successful or appropriate error code
 */
tSIRF_RESULT SIRF_EXT_LOG_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   /* write log entry if log file is open: */
   if ( log_file != NULL )
   {
      tSIRF_UINT8  payload[SIRF_EXT_LOG_MAX_MESSAGE_LEN];
      tSIRF_UINT8  packet[SIRF_EXT_LOG_MAX_MESSAGE_LEN+10];
      tSIRF_UINT32 payload_length = sizeof(payload);
      tSIRF_UINT32 packet_length = sizeof(packet);
      
      if ( SIRF_EXT_LOG_SIRF_ASCII_TEXT == log_type ) 
      {
         tRet = SIRF_CODEC_ASCII_Encode( message_id, message_structure, message_length, 
                                         (char*)packet, &packet_length );
      }
      else /* SIRF_EXT_LOG_SIRF_BINARY_STREAM */
      {
         /* Call the generic encode function, but it does not support AI3 */
         tRet = SIRF_CODEC_Encode( message_id, message_structure, message_length, 
                                   payload, &payload_length );
         if ( tRet != SIRF_SUCCESS )
         {
            return tRet;
         }

         /* Apply the protocol wrapper */
         tRet = SIRF_PROTO_Wrapper( payload, payload_length, packet, &packet_length );
      }

      if ( tRet != SIRF_SUCCESS )
      {
         return tRet;
      }

      /* Send the data */
      tRet = SIRF_PAL_LOG_Write( log_file, (char *)packet, packet_length );
   }
   return tRet;   

} /* SIRF_EXT_LOG_Send */


/** 
 * Checks if log file is open. 
 * 
 * @return SIRF_TRUE if log file is open or SIRF_FALSE if is not open
 */
tSIRF_BOOL SIRF_EXT_LOG_IsOpen( tSIRF_VOID )
{
   if ( log_file != NULL  )
   {
      return SIRF_TRUE;
   }
   else
   {
      return SIRF_FALSE;
   }
} /* SIRF_EXT_LOG_IsOpen() */

#endif /* SIRF_EXT_LOG */

/**
 * @}
 */

