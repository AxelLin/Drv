/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_comm_aio.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/06/16
  Description   : Common Def Of aio 
  History       :
  1.Date        : 2006/12/15
    Author      : z50825
    Modification: Created file
  2.Date           : 2006/12/08
    Author         : zhourui 50825
    PreVersion     : 1.5
    NewVersion     : 1.6
    Modification   : modify structure AUDIO_FRAME_S,add AUDIO_FRAME_INFO_S
  3.Date           : 2007/05/18
    Author         : zhourui 50825
    PreVersion     : 1.6
    NewVersion     : 1.7
    Modification   : delete AIO_MODE_PCM in AIO_MODE_E
  4.Date           : 2008/02/29
    Author         : p00123320
    PreVersion     : 1.7
    NewVersion     : 1.8
    Modification   : add structure AEC_FRAME_S to describe frame information of AEC
  5.Date           : 2008/06/12
    Author         : p00123320
    Modification   : modify macro MAX_AUDIO_FRAME_NUM
  6.Date           : 2009/02/24
    Author         : p00123320
    Modification   : add two PCM mode
******************************************************************************/

#ifndef __HI_COMM_AIO_H__
#define __HI_COMM_AIO_H__

#include "hi_common.h"
#include "hi_errno.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


#define MAX_AUDIO_FRAME_NUM    50       /*max count of audio frame in Buffer */
#define MAX_AUDIO_POINT_BYTES  4        /*max bytes of one sample point(now 32bit max)*/

#define MAX_VOICE_POINT_NUM    480      /*max sample per frame for voice encode */

#define MAX_AUDIO_POINT_NUM    2048     /*max sample per frame for all encoder(aacplus:2048)*/
#define MIN_AUDIO_POINT_NUM    80       /*min sample per frame*/

/*samples per frame for AACLC and aacPlus */
#define AACLC_SAMPLES_PER_FRAME         1024  
#define AACPLUS_SAMPLES_PER_FRAME       2048  

/*max length of AAC stream by bytes */
#define MAX_AAC_MAINBUF_SIZE    768*2

/*max length of audio frame by bytes, one frame contain many sample point */
#define MAX_AUDIO_FRAME_LEN    (MAX_AUDIO_POINT_BYTES*MAX_AUDIO_POINT_NUM)  

/*max length of audio stream by bytes */
#define MAX_AUDIO_STREAM_LEN   MAX_AUDIO_FRAME_LEN


typedef enum hiAUDIO_SAMPLE_RATE_E 
{ 
    AUDIO_SAMPLE_RATE_8000   = 8000,    /* 8K samplerate*/
    AUDIO_SAMPLE_RATE_11025  = 11025,   /* 11.025K samplerate*/
    AUDIO_SAMPLE_RATE_16000  = 16000,   /* 16K samplerate*/
    AUDIO_SAMPLE_RATE_22050	 = 22050,   /* 22.050K samplerate*/
    AUDIO_SAMPLE_RATE_24000  = 24000,   /* 24K samplerate*/
    AUDIO_SAMPLE_RATE_32000  = 32000,   /* 32K samplerate*/
    AUDIO_SAMPLE_RATE_44100  = 44100,   /* 44.1K samplerate*/
    AUDIO_SAMPLE_RATE_48000  = 48000,   /* 48K samplerate*/
    AUDIO_SAMPLE_RATE_BUTT,
}AUDIO_SAMPLE_RATE_E; 

typedef enum hiAUDIO_BIT_WIDTH_E
{
    AUDIO_BIT_WIDTH_8   = 0,   /* 8bit width */
    AUDIO_BIT_WIDTH_16  = 1,   /* 16bit width*/
    AUDIO_BIT_WIDTH_32  = 2,   /* 32bit width*/
    AUDIO_BIT_WIDTH_BUTT,
}AUDIO_BIT_WIDTH_E;

typedef enum hiAIO_MODE_E
{
    AIO_MODE_I2S_MASTER  = 0,   /* SIO I2S master mode */
    AIO_MODE_I2S_SLAVE,         /* SIO I2S slave mode */
    AIO_MODE_PCM_SLAVE_STD,     /* SIO PCM slave standard mode */
    AIO_MODE_PCM_SLAVE_NSTD,    /* SIO PCM slave non-standard mode */
    AIO_MODE_BUTT    
}AIO_MODE_E;

typedef enum hiAIO_SOUND_MODE_E
{
    AUDIO_SOUND_MODE_MOMO   =0,/*mono*/
    AUDIO_SOUND_MODE_STEREO =1,/*stereo*/
    AUDIO_SOUND_MODE_BUTT    
}AUDIO_SOUND_MODE_E;

typedef enum hiAMR_MODE_E
{
    AMR_MODE_MR475 = 0,     /* AMR bit rate as  4.75k */
    AMR_MODE_MR515,         /* AMR bit rate as  5.15k */              
    AMR_MODE_MR59,          /* AMR bit rate as  5.90k */
    AMR_MODE_MR67,          /* AMR bit rate as  6.70k (PDC-EFR)*/
    AMR_MODE_MR74,          /* AMR bit rate as  7.40k (IS-641)*/
    AMR_MODE_MR795,         /* AMR bit rate as  7.95k */    
    AMR_MODE_MR102,         /* AMR bit rate as 10.20k */    
    AMR_MODE_MR122,         /* AMR bit rate as 12.20k (GSM-EFR)*/            
    AMR_MODE_BUTT
}AMR_MODE_E;

typedef enum hiAMR_FORMAT_E
{
    AMR_FORMAT_MMS,     /* Using for file saving        */
    AMR_FORMAT_IF1,     /* Using for wireless translate */
    AMR_FORMAT_IF2,     /* Using for wireless translate */
    AMR_FORMAT_BUTT
}AMR_FORMAT_E;

/*
An example of the packing scheme for G726-32 codewords is as shown and
bit A3 is the least significant bit of the first codeword:
RTP G726-32:
0 1
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
|B B B B|A A A A|D D D D|C C C C| ...
|0 1 2 3|0 1 2 3|0 1 2 3|0 1 2 3|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
MEDIA G726-32:
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
|A A A A|B B B B|C C C C|D D D D| ...
|3 2 1 0|3 2 1 0|3 2 1 0|3 2 1 0|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
*/

typedef enum hiG726_BPS_E
{
	G726_16K = 0,       /* G726 16kbps for RTP ... */ 
	G726_24K,           /* G726 24kbps for RTP ... */
	G726_32K,           /* G726 32kbps for RTP ... */
	G726_40K,           /* G726 40kbps for RTP ... */
	MEDIA_G726_16K,     /* G726 16kbps for ASF ... */ 
	MEDIA_G726_24K,     /* G726 24kbps for ASF ... */
	MEDIA_G726_32K,     /* G726 32kbps for ASF ... */
	MEDIA_G726_40K,     /* G726 40kbps for ASF ... */
	G726_BUTT,
}G726_BPS_E;

typedef enum hiADPCM_TYPE_E
{
	ADPCM_TYPE_DVI4 = 0,                 /* 32kbps ADPCM(DVI4) for RTP; see DVI4 diiffers in three 
	                                       respects from the IMA ADPCM at RFC3551.txt 4.5.1 DVI4 */
	ADPCM_TYPE_IMA,                      /* 32kbps ADPCM(IMA) */
	ADPCM_TYPE_BUTT,
}ADPCM_TYPE_E;

typedef enum hiAAC_TYPE_E
{
	AAC_TYPE_AACLC      = 0,            /* AAC LC */                 
	AAC_TYPE_EAAC       = 1,            /* eAAC  (HEAAC or AAC+  or aacPlusV1) */                      
	AAC_TYPE_EAACPLUS   = 2,            /* eAAC+ (AAC++ or aacPlusV2) */
	AAC_TYPE_BUTT,
}AAC_TYPE_E;

typedef enum hiAAC_BPS_E
{
    AAC_BPS_16K     = 16000,
    AAC_BPS_22K     = 22000,
    AAC_BPS_24K     = 24000,
    AAC_BPS_32K     = 32000,
    AAC_BPS_48K     = 48000,
    AAC_BPS_64K     = 64000,
    AAC_BPS_96K     = 96000,
    AAC_BPS_128K    = 128000,
    AAC_BPS_BUTT
}AAC_BPS_E;

typedef struct hiAAC_FRAME_INFO
{
    AUDIO_SAMPLE_RATE_E enSamplerate;   /* sample rate*/
    HI_S32 s32BitRate;                  /* bitrate */
    HI_S32 s32Profile;                  /* profile*/
	HI_S32 S32TnsUsed;                  /* TNS Tools*/
	HI_S32 s32PnsUsed;                  /* PNS Tools*/
}AAC_FRAME_INFO;



#define AI_EXPAND  0x01

typedef struct hiAIO_ATTR_S
{
    AUDIO_SAMPLE_RATE_E enSamplerate;   /*sample rate*/
    AUDIO_BIT_WIDTH_E   enBitwidth;     /*bitwidth*/
    AIO_MODE_E          enWorkmode;     /*master or slave mode*/
    AUDIO_SOUND_MODE_E  enSoundmode;    /*momo or steror*/
    HI_U32              u32EXFlag;      /*expand 8bit to 16bit,use AI_EXPAND(only valid for AI) */
    HI_U32              u32FrmNum;      /*frame num in buf(0,MAX_AUDIO_FRAME_NUM]*/
    HI_U32              u32PtNumPerFrm; /*point num per frame (80、160、240、320、480)
                                            (ADPCM IMA should add 1 point, AMR only support 160)*/
}AIO_ATTR_S;
typedef struct hiAUDIO_FRAME_S
{ 
    AUDIO_BIT_WIDTH_E   enBitwidth;     /*audio frame bitwidth*/
    AUDIO_SOUND_MODE_E  enSoundmode;    /*audio frame momo or stereo mode*/
    HI_U8  aData[MAX_AUDIO_FRAME_LEN*2];/*if momo aData store left channel data
                                            if stereo aData store l/r channel data
                                            l/r chnnel data len is u32Len*/
    HI_U64 u64TimeStamp;                /*audio frame timestamp*/
    HI_U32 u32Seq;                      /*audio frame seq*/
    HI_U32 u32Len;                      /*data lenth per channel in frame*/
}AUDIO_FRAME_S; 

typedef struct hiAEC_FRAME_S
{
    AUDIO_FRAME_S 	stRefFrame;	/* AEC reference audio frame */
    HI_BOOL			bValid;   	/* whether frame is valid */
}AEC_FRAME_S;


typedef struct hiAUDIO_FRAME_INFO_S
{
    AUDIO_FRAME_S *pstFrame;/*frame ptr*/
    HI_U32         u32Id;   /*frame id*/
}AUDIO_FRAME_INFO_S;

typedef struct hiAUDIO_STREAM_S 
{ 
    HI_U8 *pStream;         /*stream buf */ 
    HI_U32 u32Len;          /*stream lenth, by bytes */
    HI_U64 u64TimeStamp;    /*frame time stamp*/
    HI_U32 u32Seq;          /*frame seq,if stream is not a valid frame,u32Seq is 0*/
}AUDIO_STREAM_S;


#define HI_TRACE_SIO(level, fmt...)\
do{ \
        HI_TRACE(level, HI_ID_SIO,"[Func]:%s [Line]:%d [Info]:", __FUNCTION__, __LINE__);\
        HI_TRACE(level,HI_ID_SIO,##fmt);\
}while(0)


//#define AUDIO_DBG

#ifdef AUDIO_DBG
#define ERR_RETURN(mod,err)\
do{\
        HI_TRACE_SIO(HI_DBG_WARN, "Audio some err:0x%x\n", err);\
        return err;\
}while(0)
#else
#define ERR_RETURN(mod,err)\
do{\
        return err;\
}while(0)

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_COMM_AI_H__ */

