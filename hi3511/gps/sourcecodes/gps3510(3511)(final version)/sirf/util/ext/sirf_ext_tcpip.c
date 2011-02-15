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
 * FILENAME:  sirf_ext_tcpip.c
 *
 * DESCRIPTION: This file include the data structures and the functions to
 *              implement the registration of protocols with UI mdoule
 *
 ***************************************************************************/

#ifdef SIRF_EXT_TCPIP

#include <string.h>
#include <stdio.h>
#include "sirf_types.h"
#include "sirf_pal.h"

//!!#include "string_sif.h"

#include "sirf_proto_common.h"
#include "sirf_codec_ssb.h"
#include "sirf_ext_tcpip.h"
#include "sirf_demo.h"
#include "sirf_codec.h"

#include "sirfnav_demo_config.h"  /* @TODO: review making a global define instead with Stefan */ 

#define TRANSMIT_BUFFER_SIZE 6000
#define RECEIVE_BUFFER_SIZE  6000

#define DEFAULT_TIMEOUT      1000
#define CONNECT_TIMEOUT      5000

/* This needs to be redefined and renamed */
#define GPS_MAX_PACKET_LEN 512

static tSIRF_BOOL      c_running;
static tSIRF_THREAD    c_tcp_connect_handle;
static tSIRF_SOCKET    c_tcp_sock;
static tSIRF_UINT8     c_transmit_buffer[ TRANSMIT_BUFFER_SIZE ];
static tSIRF_UINT16    c_transmit_first = 0;
static tSIRF_UINT16    c_transmit_last  = 0; /* actually index of next free byte */
static tSIRF_BOOL      c_anything_received;
static tSIRF_UINT16    c_tcpip_port;
static tSIRF_CHAR      c_tcpip_addr[256];

static t_data_callback_func f_callback = NULL;

/***************************************************************************
 * Global Data Declarations
 ***************************************************************************/

tSIRF_MUTEX g_EXT_TCPIP_WRITE_Critical;

/***************************************************************************
 * Description: Send a byte-stream buffer out on the AUX port(s)
 * Parameters:  packed byte-stream, size of same
 * Returns:     nothing
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_TCPIP_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
   tSIRF_UINT8  payload[SIRF_EXT_TCPIP_MAX_MESSAGE_LEN];
   tSIRF_UINT8  packet[SIRF_EXT_TCPIP_MAX_MESSAGE_LEN+10];
   tSIRF_UINT32 payload_length = sizeof(payload);
   tSIRF_UINT32 packet_length = sizeof(packet);
   tSIRF_UINT8 *buf;
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   if ( !c_running || (0 == message_length) || !message_structure )
   {
      return SIRF_FAILURE;
   }

   if (SIRF_SUCCESS == SIRF_PAL_OS_MUTEX_Enter(g_EXT_TCPIP_WRITE_Critical))
   {
      /* Call the generic encode function, but does not support AI3  */
      tRet = SIRF_CODEC_Encode( message_id, message_structure, message_length, payload, &payload_length );
      if ( tRet != SIRF_SUCCESS )
      {
         SIRF_PAL_OS_MUTEX_Exit( g_EXT_TCPIP_WRITE_Critical );
         return tRet;
      }

      /* Apply the protocol wrapper */
      tRet = SIRF_PROTO_Wrapper( payload, payload_length, packet, &packet_length );
      if ( tRet != SIRF_SUCCESS )
      {
         SIRF_PAL_OS_MUTEX_Exit( g_EXT_TCPIP_WRITE_Critical );
         return tRet;
      }

      if (c_transmit_first >= c_transmit_last)
      {
         c_transmit_last  = 0;
         c_transmit_first = 0;
      }

      if ( c_transmit_last <= sizeof(c_transmit_buffer) - GPS_MAX_PACKET_LEN )
      {
         /* Set up the pointer to where the data will be copied to */
         buf = c_transmit_buffer + c_transmit_last;

         /* copy the data and update the index to the next available location */
         memcpy( (tSIRF_VOID*)&buf, (void*)packet, packet_length );
         c_transmit_last += (tSIRF_UINT16)packet_length;
      }

      SIRF_PAL_OS_MUTEX_Exit(g_EXT_TCPIP_WRITE_Critical);
   }   
   return tRet;
}

/** 
 * Send a message that requires no Codec
 * 
 * @param message Message to send
 * @param length  Length of message
 * 
 * @return SIRF_SUCCESS if sent successfully.  TCP error code otherwise.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Send_Passthrough( tSIRF_VOID *message, tSIRF_UINT32 length )
{
   tSIRF_UINT8 *buf;

   if ( !c_running || (0 == length) || !message )
   {
      return SIRF_FAILURE;
   }

   if (SIRF_SUCCESS == SIRF_PAL_OS_MUTEX_Enter(g_EXT_TCPIP_WRITE_Critical))
   {
      if (c_transmit_first >= c_transmit_last)
      {
         c_transmit_last  = 0;
         c_transmit_first = 0;
      }

      if ( c_transmit_last <= sizeof(c_transmit_buffer) - GPS_MAX_PACKET_LEN )
      {
         /* Set up the pointer to where the data will be copied to */
         buf = c_transmit_buffer + c_transmit_last;

         /* copy the data and update the index to the next available location */
         memcpy( (tSIRF_VOID*)&buf, (void*)message, length );
         c_transmit_last += (tSIRF_UINT16)length;
      }

      SIRF_PAL_OS_MUTEX_Exit(g_EXT_TCPIP_WRITE_Critical);
   }
   return SIRF_SUCCESS;
}

/** 
 * Set the callback function.
 * 
 * @param callback_func pointer to the callback function.
 * 
 */
tSIRF_VOID SIRF_EXT_TCPIP_Callback_Register( t_data_callback_func callback_func )
{
   f_callback = callback_func;
}

/** 
 * Guts of the thread for reading and writing to the socket.
 *
 * Uppon data it calls the fucntion set by SIRF_EXT_TCPIP_Callback_Register
 */
static tSIRF_VOID TCPReaderWriter(tSIRF_VOID)
{
   static tSIRF_UINT32 byte_count;
   static tSIRF_UINT16 buf_len;
   static tSIRF_UINT8  buf[RECEIVE_BUFFER_SIZE];
   static tSIRF_INT32 result;
   static tSIRF_UINT32 payload_length;
   static tSIRF_UINT16 last, first;

   SIRF_PAL_OS_THREAD_START();

   /* runs a simplified SiRFBinary import/export parser while connection is open: */
   buf_len = 0;
   memset( buf, 0, sizeof(buf) );
   do
   {
      tSIRF_SOCKET readset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};
      tSIRF_SOCKET writeset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};

      /* reset input buffer if full: */
      if (buf_len==sizeof(buf))
         buf_len = 0;

      SIRF_PAL_OS_MUTEX_Enter(g_EXT_TCPIP_WRITE_Critical);
      first = c_transmit_first;
      last = c_transmit_last;
      SIRF_PAL_OS_MUTEX_Exit(g_EXT_TCPIP_WRITE_Critical);

      /* wait for socket: */
      readset[0] = c_tcp_sock;
      if ( last > first ) /* any data to send? */
         writeset[0] = c_tcp_sock;
      result = SIRF_PAL_NET_Select( readset, writeset, 0, DEFAULT_TIMEOUT );
      if ( result!=SIRF_SUCCESS || !c_running )
         continue; /* timed out or error */

      /* socket reads: */
      if ( readset[0] )
      {
         result = SIRF_PAL_NET_Read( c_tcp_sock, buf+buf_len, sizeof(buf)-buf_len, &byte_count );
         if ( result==SIRF_PAL_NET_CONNECTION_CLOSED )
            break; /* while c_running */
         if ( result!=SIRF_SUCCESS )
         {
            DEBUGMSG(1, (DEBUGTXT("tcpip: read error, result=%d"), result));
            continue; /* while c_running */
         }

         buf_len += (tSIRF_UINT16)byte_count;

         /* parse all messages received in whole: */
         while ( buf_len >= 8 )
         {
            payload_length = ((tSIRF_UINT32)buf[2]<<8) | buf[3];
            if ( payload_length+8 > buf_len )
               break; /* insufficient bytes in buffer */

            /* check for errors: */
            if (  buf[0]!=SIRF_MSG_HEAD0 || buf[1]!=SIRF_MSG_HEAD1
               || buf[payload_length+6]!=SIRF_MSG_TAIL0 || buf[payload_length+7]!=SIRF_MSG_TAIL1 )
            {
               buf_len = 0;
               break;
            }

            /* Call the data callback if it is available to parse the data */
            if ( f_callback )
            {
               f_callback( buf, payload_length );
            }

            /* Not currently used, but should be? */
            c_anything_received = SIRF_TRUE;

            /* shift remaining bytes: */
            memmove( buf, buf+payload_length+8, buf_len-(payload_length+8) );
            buf_len -= (tSIRF_UINT16)(payload_length+8);
            if (buf_len>sizeof(buf))
               buf_len = 0;
         };
      }

      /* socket writes: */
      if ( writeset[0] && last>first )
      {
         result = SIRF_PAL_NET_Write( c_tcp_sock, c_transmit_buffer+first, last-first, &byte_count );
         if ( result == SIRF_SUCCESS )
         {
            SIRF_PAL_OS_MUTEX_Enter(g_EXT_TCPIP_WRITE_Critical);
            c_transmit_first += (tSIRF_UINT16)byte_count;
            if ( c_transmit_first >= c_transmit_last )
               c_transmit_first = c_transmit_last = 0;
            SIRF_PAL_OS_MUTEX_Exit(g_EXT_TCPIP_WRITE_Critical);
         }
      }

   } while ( c_running ); /* repeat until connection closed or module exiting */
}


/** 
 * Thread that creates a connection and runs the reader writer.
 * 
 * @param SIRF_PAL_OS_THREAD_PARAMS OS specific parameters.
 * 
 * @return OS specific.
 */
static SIRF_PAL_OS_THREAD_DECL TCPConnect( SIRF_PAL_OS_THREAD_PARAMS )
{
   tSIRF_INT16 reconnect = 1;
   tSIRF_RESULT result    = 0;

   /* Unused Parameters. */
   SIRF_PAL_OS_THREAD_UNUSED
   
   SIRF_PAL_OS_THREAD_START();

   do
   {
      tSIRF_SOCKET writeset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};
      /* create socket and initiate connection: */
      result = SIRF_PAL_NET_CreateSocket(&c_tcp_sock, SIRF_PAL_NET_SOCKET_TYPE_DEFAULT);
      DEBUGCHK(result==0);

      DEBUGMSG(1, (DEBUGTXT("tcpip: connecting...\n")));
      result = SIRF_PAL_NET_Connect( c_tcp_sock, c_tcpip_addr, c_tcpip_port, SIRF_PAL_NET_SECURITY_NONE );

      writeset[0] = c_tcp_sock;
      result |= SIRF_PAL_NET_Select( NULL, writeset, NULL, CONNECT_TIMEOUT );

      /* if connection established, do read/write: */
      if ( result==SIRF_SUCCESS )
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: connection established\n")));

         TCPReaderWriter();

         #ifdef EPH_AIDING_DEMO
            reconnect = 0;
         #endif
      }
      else
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: unable to connect; result=%d; retrying in 1 second\n"), result));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
      }

      /* close socket and release memory: */
      SIRF_PAL_NET_CloseSocket(c_tcp_sock);

   } while ( c_running && reconnect );

   SIRF_PAL_OS_THREAD_RETURN();
}

/** 
 * TCP Listener thread
 * 
 * @param SIRF_PAL_OS_THREAD_PARAMS OS Specific
 * 
 * @return OS specific
 */
static SIRF_PAL_OS_THREAD_DECL TCPListener( SIRF_PAL_OS_THREAD_PARAMS )
{
   tSIRF_SOCKET    listener;
   tSIRF_SOCKET    readset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};
   tSIRF_UINT32    result;
   tSIRF_SOCKADDR  name, incoming;

   /* Unused Parameters. */
   SIRF_PAL_OS_THREAD_UNUSED
   
   SIRF_PAL_OS_THREAD_START();

   result = SIRF_PAL_NET_CreateSocket(&listener, SIRF_PAL_NET_SOCKET_TYPE_DEFAULT);
   DEBUGCHK( result==SIRF_SUCCESS && listener );

   memset(&name, 0, sizeof(name));
   #ifdef _ENDIAN_BIG
      name.sin_port = c_tcpip_port;
   #else
      name.sin_port = c_tcpip_port>>8 | c_tcpip_port<<8;
   #endif

   do
   {
      result = SIRF_PAL_NET_Bind(listener, &name);
      if ( result==SIRF_SUCCESS )
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: bound to port %d...\n"), c_tcpip_port));
      }
      else
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: error binding to port %d, result=0x%04ld...\n"), c_tcpip_port, result));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
      }
   } while ( c_running && result!=SIRF_SUCCESS );

   while (c_running)
   {
      result = SIRF_PAL_NET_Listen(listener);
      if (result!=0)
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: error listening\n")));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
         continue;
      }

      readset[0] = listener;
      result = SIRF_PAL_NET_Select(readset, 0, 0, DEFAULT_TIMEOUT);
      if (result != SIRF_SUCCESS)
         continue;

      memset(&incoming, 0, sizeof(incoming));
      result = SIRF_PAL_NET_Accept(listener, &c_tcp_sock, &incoming, SIRF_PAL_NET_SECURITY_NONE);
      if (result!=0)
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: unable to accept; result=%d; continuing listening\n"), result));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
         continue;
      }

      DEBUGMSG(1, (DEBUGTXT("tcpip: connection established\n")));

      TCPReaderWriter();
   }

   /* shutdown and close listener sockets, preventing any further connections: */
   SIRF_PAL_NET_CloseSocket(listener);
   DEBUGMSG(1, (DEBUGTXT("tcpip: listener closed\n")));

   /* close socket and release memory: */
   if (c_tcp_sock)
      SIRF_PAL_NET_CloseSocket(c_tcp_sock);

   SIRF_PAL_OS_THREAD_RETURN();
}


/** 
 * Creates a thread to connect to a tcpip address or to listen to a connection
 * 
 * @param tcpip_addr tcpip address, If null creates a listening thread
 * @param tcpip_port tcpip port
 * 
 * @return SIRF_SUCCESS if the threads to do the request action is successful
 *         thread error code otherwise.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Create( tSIRF_CHAR *tcpip_addr, tSIRF_UINT16 tcpip_port )
{
   tSIRF_RESULT ret = 0;

   /* Create write mutex */
   ret = SIRF_PAL_OS_MUTEX_Create(&g_EXT_TCPIP_WRITE_Critical);

   if (c_running) return SIRF_FAILURE;

   c_tcp_sock = c_transmit_first = c_transmit_last = 0;
   c_anything_received = SIRF_FALSE;

   c_tcpip_port = tcpip_port;
   c_running = SIRF_TRUE;

   if (tcpip_addr)
   {
      strlcpy(c_tcpip_addr, tcpip_addr, sizeof(c_tcpip_addr));
      ret |= SIRF_PAL_OS_THREAD_Create(SIRFNAV_DEMO_THREAD_EXT1, (tSIRF_HANDLE)TCPConnect, &c_tcp_connect_handle);
   }
   else
   {
      ret |= SIRF_PAL_OS_THREAD_Create(SIRFNAV_DEMO_THREAD_EXT1, (tSIRF_HANDLE)TCPListener, &c_tcp_connect_handle);
   }

   return ret;
}

/** 
 * Shuts down the EXT tcp threads and releases all resources allocated.
 * 
 * @return SIRF_SUCCESS if successful
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Delete(tSIRF_VOID)
{
   if (!c_running) return SIRF_FAILURE;

   /* signal shutdown: */
   c_running = SIRF_FALSE;

   /* stop threads: */
   SIRF_PAL_OS_THREAD_Delete(c_tcp_connect_handle);

   /* Delete Write Mutex */
   SIRF_PAL_OS_MUTEX_Delete(g_EXT_TCPIP_WRITE_Critical);
   g_EXT_TCPIP_WRITE_Critical = 0;

   DEBUGMSG(1, (DEBUGTXT("tcpip: connection closed\n")));

   return SIRF_SUCCESS;
}


/** 
 * Validates that anything has been received on the connection.
 * 
 * @return SIRF_SUCCESS if any data has ever been received.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_VerifyConnection(tSIRF_VOID)
{
   return c_anything_received ? SIRF_SUCCESS : SIRF_FAILURE;
}


#endif /* SIRF_EXT_TCPIP */

/**
 * @}
 */

