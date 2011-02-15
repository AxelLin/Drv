/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_comm_venc.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/11/24
  Last Modified :
  Description   : common struct definition for VENC
  Function List :
  History       :
  1.Date        : 2006/11/24
    Author      : c42025
    Modification: Created file
  2.Date        : 2008/07/25
    Author      : l64467
    Modification: delete the structe which is not need
                  and rework notation

******************************************************************************/

#ifndef __HI_COMM_VENC_H__
#define __HI_COMM_VENC_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "hi_type.h"
#include "hi_common.h"
#include "hi_errno.h"
#include "hi_comm_video.h"
#include "hi_comm_dwm.h"


/* invlalid device ID */
#define HI_ERR_VENC_INVALID_DEVID     HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* invlalid channel ID */
#define HI_ERR_VENC_INVALID_CHNID     HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define HI_ERR_VENC_ILLEGAL_PARAM     HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define HI_ERR_VENC_EXIST             HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* channel exists */
#define HI_ERR_VENC_UNEXIST             HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* using a NULL point */
#define HI_ERR_VENC_NULL_PTR          HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define HI_ERR_VENC_NOT_CONFIG        HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define HI_ERR_VENC_NOT_SURPPORT      HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SURPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define HI_ERR_VENC_NOT_PERM          HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* failure caused by malloc memory */
#define HI_ERR_VENC_NOMEM             HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define HI_ERR_VENC_NOBUF             HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define HI_ERR_VENC_BUF_EMPTY         HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define HI_ERR_VENC_BUF_FULL          HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded*/
#define HI_ERR_VENC_SYS_NOTREADY      HI_DEF_ERR(HI_ID_VENC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)

/*main stream*/
#define TYPE_MAIN_STREAM  (1)

/*minor stream*/
#define TYPE_MINOR_STREAM (0)

/*the nalu type of H264E*/
typedef enum hiH264E_NALU_TYPE_E
{
     H264E_NALU_PSLICE = 1, /*PSLICE types*/
     H264E_NALU_ISLICE = 5, /*ISLICE types*/
     H264E_NALU_SEI    = 6, /*SEI types*/
     H264E_NALU_SPS    = 7, /*SPS types*/
     H264E_NALU_PPS    = 8, /*PPS types*/
     H264E_NALU_BUTT        
} H264E_NALU_TYPE_E;


/*the pack type of JPEGE*/
typedef enum hiJPEGE_PACK_TYPE_E
{
     JPEGE_PACK_ECS = 5, /*ECS types*/
     JPEGE_PACK_APP = 6, /*APP types*/
     JPEGE_PACK_VDO = 7, /*VDO types*/
     JPEGE_PACK_PIC = 8, /*PIC types*/
     JPEGE_PACK_BUTT
} JPEGE_PACK_TYPE_E;

typedef enum hiM4QType
{
     MPEG4E_QUANT_H263 = 0,
     MPEG4E_QUANT_MPEG = 1
} M4QType;

/*the pack type of MPEG4*/
typedef enum hiMPEG4E_PACK_TYPE_E
{
     MPEG4E_PACK_VO  = 1,  /*VO types*/
     MPEG4E_PACK_VOS = 2,  /*VOS types*/
     MPEG4E_PACK_VOL = 3,  /*VOL types*/
     MPEG4E_PACK_VOP = 4,  /*VOP types*/
     MPEG4E_PACK_SLICE = 5 /*SLICE types*/
} MPEG4E_PACK_TYPE_E;

/*the data type of VENC*/
typedef union hiVENC_DATA_TYPE_U
{
    H264E_NALU_TYPE_E    enH264EType; /*H264E NALU types*/
    JPEGE_PACK_TYPE_E    enJPEGEType; /*JPEGE pack types*/
    MPEG4E_PACK_TYPE_E   enMPEG4EType;/*MPEG4E pack types*/
}VENC_DATA_TYPE_U;


typedef struct hiVENC_PACK_S
{
    HI_U32   u32PhyAddr[2];         /*the physics address of stream*/
    HI_U8   *pu8Addr[2];    		/*the virtual address of stream*/
    HI_U32   u32Len[2];             /*the length of stream*/
    
    HI_U64   u64PTS;     /*PTS*/
    HI_BOOL  bFieldEnd;  /*field end£¿*/
    HI_BOOL  bFrameEnd;  /*frame end£¿*/
	
	VENC_DATA_TYPE_U  DataType;   /*the type of stream*/
}VENC_PACK_S;

typedef struct hiVENC_STREAM_S
{
    VENC_PACK_S         *pstPack;		/*stream pack attribute*/
    HI_U32              u32PackCount;   /*the pack number of one frame stream*/
    HI_U32              u32Seq;         /*the list number of stream*/
}VENC_STREAM_S;

typedef struct hiVENC_ATTR_H264_S
{
    HI_U32  u32Priority;         /*channels precedence level*/
    HI_U32  u32PicWidth;         /*the picture width*/
    HI_U32  u32PicHeight;        /*the picture height*/
    HI_U32  u32ViFramerate;      /*barbarism frame rate*/
    HI_U32  u32TargetFramerate;  /*target frame rate*/
    HI_BOOL bMainStream;         /*main stream or minor stream types?*/
	HI_BOOL bVIField;			 /*the sign of the VI picture is field or frame?*/
    HI_BOOL bField;              /*frame coding or field coding£¿*/
    HI_BOOL bCBR;                /*CBR or VBR?*/
    HI_U32  u32Bitrate;          /*target bit rate*/
    HI_U32  u32PicLevel;         /*the picture mass level*/
    HI_U32  u32Gop;              /*the interval of ISLICE*/
    HI_U32  u32MaxDelay;         /*max delay frame*/
    HI_U32  u32BufSize;          /*stream buffer size*/
    HI_BOOL bByFrame;            /*get stream mode is field mode  or frame mode?*/
}VENC_ATTR_H264_S;


typedef struct hiVENC_ATTR_MJPEG_S
{
    HI_U32  u32Priority;		 /*channels precedence level*/
    HI_U32  u32BufSize;          /*stream buffer size*/
    HI_U32  u32PicWidth;         /*the picture width*/
    HI_U32  u32PicHeight;        /*the picture height*/
    HI_BOOL bByFrame;            /*get stream mode is field mode  or frame mode?*/
    HI_U32  u32ViFramerate;      /*barbarism frame rate*/
    HI_BOOL bMainStream;         /*main stream or minor stream types?*/
	HI_BOOL bVIField;			 /*the sign of the VI picture is field or frame?*/
    HI_U32  u32TargetBitrate;    /*target bit rate*/
    HI_U32  u32TargetFramerate;  /*target frame rate*/
    HI_U32  u32MCUPerECS;        /*MCU number of one ECS*/

}VENC_ATTR_MJPEG_S;


typedef struct hiVENC_ATTR_JPEG_S 
{
    HI_U32  u32Priority;	   /*channels precedence level*/
    HI_U32  u32BufSize;        /*stream buffer size*/
    HI_U32  u32PicWidth;       /*the picture width*/
    HI_U32  u32PicHeight;      /*the picture height*/
	HI_BOOL bVIField;		   /*the sign of the VI picture is field or frame?*/
    HI_BOOL bByFrame;          /*get stream mode is field mode  or frame mode?*/
    HI_U32  u32MCUPerECS;      /*MCU number of one ECS*/
    HI_U32  u32ImageQuality;   /*image quality*/
}VENC_ATTR_JPEG_S;


/* attribute of MPEG4*/
typedef struct  hiVENC_ATTR_MPEG4_S
{
    HI_U32  u32Priority;          /*channels precedence level*/
    HI_U32  u32PicWidth;          /*the picture width*/
    HI_U32  u32PicHeight;         /*the picture height*/
    HI_U32  u32TargetBitrate;     /*target bit rate*/
    HI_U32  u32ViFramerate;       /*barbarism frame rate*/
    HI_U32  u32TargetFramerate;   /*target frame rate*/
    HI_U32  u32Gop;               /*the interval of ISLICE*/
    HI_U32  u32MaxDelay;          /*max delay frame*/
    M4QType enQuantType;          /*MPEG4 quant type*/
    HI_U32  u32BufSize;           /*buffer size*/
	HI_BOOL bVIField;			  /*the sign of the VI picture is field or frame?*/
    HI_BOOL bByFrame;             /*get stream mode is field or frame?*/
}VENC_ATTR_MPEG4_S;


typedef struct hiMPEG4E_ABILITY_S
{
    HI_BOOL IVOP_En;          /* I-VOP Enable */
    HI_BOOL PVOP_En;          /* P-VOP Enable */
    HI_BOOL BVOP_En;          /* B-VOP Enable*/
    HI_BOOL DCPred_En;        /* DC Prediction Enable*/
    HI_BOOL ACPred_En;        /*AC Prediction Enable*/
    HI_BOOL M4V_En;           /*M4V Enable*/
    HI_BOOL Resyn_En;         /*GOB Resynchronization Enable*/
    HI_BOOL DataPart_En;      /*Data Partitioning Enable*/
    HI_BOOL RVLC_En;          /*Reversible VLC Enable*/
    HI_BOOL ShortHead_En;     /*Short Header Enable*/
    HI_BOOL Interlace_En;     /*Interlace Enable*/
    HI_BOOL GMC_En;           /*Global Motion Compensation Enable*/
    HI_BOOL QMC_En;           /*Quarter-pel Motion Compensation Enable*/
    HI_BOOL HalfPel_En;       /* I-VOP Enable*/
    HI_BOOL DBlock_En;        /*Deblocking Enable*/
    HI_BOOL DRing_En;         /*Deringing Enable*/

    M4QType QuantType;        /*Method 1/Method 2 Quantisation*/
    HI_U32  SearchWin;        /*Search Window*/
}MPEG4E_ABILITY_S;

typedef struct hiVENC_CHN_ATTR_S
{
    PAYLOAD_TYPE_E enType; /*the type of payload*/
    HI_VOID *pValue;
}VENC_CHN_ATTR_S;


typedef struct hiVENC_CHN_STAT_S
{
	HI_BOOL bRegistered;       /*registered ?*/
	HI_U32 u32LeftPics;        /*left picture number*/
	HI_U32 u32LeftStreamBytes; /*left stream bytes*/
	HI_U32 u32CurPacks;        /*pack number of current frame*/
}VENC_CHN_STAT_S;

typedef struct hiVENC_CAPABILITY_S
{
    PAYLOAD_TYPE_E enType; /*the type of payload*/
    HI_VOID *pCapability;
}VENC_CAPABILITY_S;


typedef struct hiH264_VENC_CAPABILITY_S
{
    HI_U8   u8Profile       ;   /*0:baseline 1:mainprofile  2:externed profile*/
    HI_U8   u8Level         ;   /*eg: 22 denote level2.2*/
    HI_U8   u8BaseAttr      ;   /*bit0¡«bit5 denote MBAFF, PAFF£¬B SLICE£¬FMO£¬ ASO£¬PARTITION  */
    HI_U8   u8ViFormat      ;   /*format£¬ bit.0: PAL(25)£¬bit.1:NTSC(30)  */
    HI_U8   u8MaxWInMb      ;   /*the max width of input picture*/
    HI_U8   u8MaxHInMb      ;   /*the max height of input picture*/
    HI_U16  u16MaxCifNum    ;   /*the max coding capability */
    HI_U16  u16MaxBitrate   ;   /*the max bit rate output*/
    HI_U16  upperbandwidth  ;   /*upper band*/
    HI_U16  lowerbandwidth  ;   /*lower band*/
    HI_U8   palfps          ;   /*PAL: frame per second*/
    HI_U8   ntscfps         ;   /*NTSC: frame per second*/

}H264_VENC_CAPABILITY_S;

typedef struct hiJPEG_VENC_CAPABILITY_S
{
    HI_U8 u8Profile         ;   /*0:baseline 1:extened profile  2:loseless profile 3:hierarchical profile*/
    HI_U8 u8ViFormat        ;   /*format£¬ bit.0: PAL(25)£¬bit.1:NTSC(30)  */
    HI_U8 u8MaxWInMb        ;   /*the max width of input picture*/
    HI_U8 u8MaxHInMb        ;   /*the max height of input picture*/
    HI_U16 u16MaxCifNum     ;   /*the max coding capability */
    HI_U16 u16MaxBitrate    ;   /*the max bit rate output*/
    HI_U16 upperbandwidth   ;   /*upper band*/
    HI_U16 lowerbandwidth   ;   /*lower band*/
    HI_U8 palfps            ;   /*PAL: frame per second*/
    HI_U8 ntscfps           ;   /*NTSC: frame per second*/

}JPEG_VENC_CAPABILITY_S;


/*reserve structe*/
typedef struct hiVENC_ATTR_MEPARA_S
{
    HI_S32 s32HWSize;
    HI_S32 s32VWSize;

    HI_S32 s32IterNum[8];
    HI_S32 s32Denoise[2];
    HI_S32 s32RefPicNum;

} VENC_ATTR_MEPARA_S;

typedef struct hiVENC_ATTR_H264_RC_S
{
    HI_S32 s32MinQP;

} VENC_ATTR_H264_RC_S;

typedef struct hiVENC_ATTR_H264_NALU_S
{
    HI_BOOL bNaluSplitEnable;
    HI_U32  u32NaluSize;

} VENC_ATTR_H264_NALU_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __HI_COMM_VENC_H__ */

