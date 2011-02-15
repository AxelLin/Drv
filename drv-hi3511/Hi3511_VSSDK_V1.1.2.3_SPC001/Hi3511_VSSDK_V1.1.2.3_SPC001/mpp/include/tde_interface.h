/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tde_interface.h
  Version       : Initial Draft
  Author        : w54130
  Created       : 2007/9/15
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2007/9/15
    Author      : w54130
    Modification: Created file

  2.Date        : 2008/10/31
    Author      : c42025
    Modification: redefine HI_LOG_LEVEL_ERROR as 3

******************************************************************************/

#ifndef __TDE_INTERFACE_H__
#define __TDE_INTERFACE_H__

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_errno.h"

//#include "tde_errno.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


typedef HI_S32 TDE_HANDLE; 

#define HI_LOG_LEVEL_ERROR 3

/* TODO: */
#define HI_ID_TDE 100

/* tde device not open yet */ 
#define  HI_ERR_TDE_DEV_NOT_OPEN        HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 1)

/* open tde device failed */ 
#define  HI_ERR_TDE_DEV_OPEN_FAILED     HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 2)

/* reopen tde device   	  */
#define  HI_ERR_TDE_DEV_REOPEN          HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 3)

/* input parameters contain null ptr */
#define  HI_ERR_TDE_NULL_PTR			HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 4)

/* malloc failed  */
#define  HI_ERR_TDE_NO_MEM		        HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 5)  

/* invalid job handle  */
#define  HI_ERR_TDE_INVALID_HANDLE	    HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 6)

/* blocked job wait timeout */
#define  HI_ERR_TDE_JOB_TIMEOUT	        HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 7)

/* blocked job interrupted  */
#define  HI_ERR_TDE_JOB_INTERRUPT	    HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 8)

/* invalid parameter */
#define HI_ERR_TDE_INVALID_PARA         HI_DEF_ERR(HI_ID_TDE, HI_LOG_LEVEL_ERROR, 9)

/* the color format TDE supported */
typedef enum hiTDE_COLOR_FMT_E
{
    TDE_COLOR_FMT_RGB444    = 0,
    TDE_COLOR_FMT_RGB4444   = 1,
    TDE_COLOR_FMT_RGB555    = 2,
    TDE_COLOR_FMT_RGB565    = 3,
    TDE_COLOR_FMT_RGB1555   = 4,
    TDE_COLOR_FMT_RGB888    = 6,
    TDE_COLOR_FMT_RGB8888   = 7,
    TDE_COLOR_FMT_BUTT
}TDE_COLOR_FMT_E;

typedef enum hiTDE_ROP_CODE_E
{
    TDE_ROP_BLACK   = 0,    /* Blackness */
    TDE_ROP_PSDon   = 1,    /* ~(PS+D) */
    TDE_ROP_PSDna   = 2,    /* ~PS & D */
    TDE_ROP_PSn     = 3,    /* ~PS */
    TDE_ROP_DPSna   = 4,    /* PS & ~D */
    TDE_ROP_Dn      = 5,    /* ~D */
    TDE_ROP_PSDx    = 6,    /* PS^D */
    TDE_ROP_PSDan   = 7,    /* ~(PS&D) */
    TDE_ROP_PSDa    = 8,    /* PS & D */
    TDE_ROP_PSDxn   = 9,    /* ~(PS^D) */
    TDE_ROP_D       = 10,    /* D */
    TDE_ROP_PSDno   = 11,    /* ~PS + D */
    TDE_ROP_PS      = 12,    /* PS */
    TDE_ROP_DPSno   = 13,    /* PS + ~D */
    TDE_ROP_PSDo    = 14,    /* PS+D */
    TDE_ROP_WHITE   = 15,    /* Whiteness */
    TDE_ROP_BUTT
}TDE_ROP_CODE_E;

/* the operation TDE will be done */
typedef enum hiTDE_DATA_CONV_E
{
    TDE_DATA_CONV_NONE = 0,         /* no opetation will be done between
                                       srouce and destination surface */
    TDE_DATA_CONV_ROP = 1,          /* the source and destination surface
                                       will do ROP operation */
    TDE_DATA_CONV_ALPHA_SRC = 2,    /* the source and destination surface
                                       will take source's alpha to do alpha
                                       operation */
    TDE_DATA_CONV_ALPHA_DST = 3     /* the source and destination surface
                                       will take destination's alpha to do
                                       alpha operation */
}TDE_DATA_CONV_E;


typedef enum hiTDE_OUTALPHA_FROM_E
{
    TDE_OUTALPHA_FROM_RESERVED,         /* reserved */
    TDE_OUTALPHA_FROM_SRC,              /* out alpha from source bitmap */
    TDE_OUTALPHA_FROM_DST,              /* out alpha from destination bitmap */
    TDE_OUTALPHA_FROM_REG               /* out alpha from global alpha */
}TDE_OUTALPHA_FROM_E;

typedef enum hiTDE_COLORSPACE_TARGET_E
{
    TDE_COLORSPACE_SRC,                 /* do colorspace to source bitmap */
    TDE_COLORSPACE_DST                  /* do colorspace to destination bitmap */
}TDE_COLORSPACE_TARGET_E;

typedef struct hiTDE_RGB_S
{
    HI_U8   u8B;
    HI_U8   u8G;
    HI_U8   u8R;
    HI_U8   u8Reserved;
}TDE_RGB_S;

typedef struct TDE_DEFLICKER_COEF_S
{
    HI_U32  u32HDfLevel;    /* horizontal deflicker level */     
    HI_U32  u32VDfLevel;    /* vertical deflicker level */
    HI_U8   *pu8HDfCoef;    /* horizontal deflicker coefficient */
    HI_U8   *pu8VDfCoef;    /* vertical deflicker coefficient */
}TDE_DEFLICKER_COEF_S;

typedef struct hiTDE_COLORSPACE_S
{
    HI_BOOL bColorSpace;                        /* colorspace enable */
    TDE_COLORSPACE_TARGET_E enColorSpaceTarget; /* colorspace target */
    TDE_RGB_S stColorMin;                       /* minimum color in colorspace */
    TDE_RGB_S stColorMax;                       /* maximum color in colorspace */
}TDE_COLORSPACE_S;

typedef struct hiTDE_ALPHA_S
{
    HI_BOOL bAlphaGlobal;                       /* global alpha enable */
    TDE_OUTALPHA_FROM_E enOutAlphaFrom;         /* the out alpha from */
    HI_U8 u8Alpha0;                             /* used in color format ARGB1555 */
    HI_U8 u8Alpha1;                             /* used in color format ARGB1555 */
    HI_U8 u8AlphaGlobal;                        /* valid when bAlphaGlobal=true */
    HI_U8 u8Reserved;
}TDE_ALPHA_S;

typedef struct hiTDE_SURFACE_S
{
    TDE_COLOR_FMT_E enColorFmt;         /* color format */
    HI_U8  *pu8PhyAddr;               /* physical address */
    HI_U16  u16Height;                /* operation height */
    HI_U16  u16Width;                 /* operation width */
    HI_U16  u16Stride;                /* surface stride */
    HI_U16  u16Reserved;
}TDE_SURFACE_S;

typedef struct hiTDE_PATTERN_S
{
    TDE_COLOR_FMT_E enColorFmt;         /* color format */
    HI_U8  *pu8PhyAddr;               /* physical address */
    HI_U8  u8Height;                /* pattern height */
    HI_U8  u8Width;                 /* pattern width */
    HI_U16 u16Stride;                /* pattern stride */
}TDE_PATTERN_S;

typedef struct hiTDE_OPT_S
{
    TDE_DATA_CONV_E enDataConv;     /* operation choice */
    TDE_ALPHA_S stAlpha;            /* alpha attribute */
    TDE_ROP_CODE_E enRopCode;       /* ROP type */
    TDE_COLORSPACE_S stColorSpace;  /* colorspace attribute */
}TDE_OPT_S;

/*****************************************************************************
 Prototype    : HI_API_TDE_Open
 Description  : Open the Tde device
 Input        : HI_VOID  
 Output       : None
 Return Value : HI_ERR_TDE_DEV_OPEN_FAILED 
                HI_ERR_TDE_DEV_REOPEN
                HI_SUCCESS
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_Open(HI_VOID);

/*****************************************************************************
 Prototype    : HI_API_TDE_BeginJob
 Description  : start a TDE job
 Input        : HI_VOID  
 Output       : None
 Return Value : TDE_HANDLE:job handle
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_INVALID_HANDLE
 Calls        : 
 Called By    : 
 
*****************************************************************************/
TDE_HANDLE  HI_API_TDE_BeginJob(HI_VOID);

/*****************************************************************************
 Prototype    : HI_API_TDE_BitBlt
 Description  : add a blit command to the job
 Input        : TDE_HANDLE s32Handle   job handle
                TDE_SURFACE_S *pstSrc  source surface
                TDE_SURFACE_S *pstDst  destination surface
                TDE_OPT_S *pstOpt      operation attribute
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_NO_MEM
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_INVALID_PARA
                HI_ERR_TDE_NULL_PTR
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_BitBlt(TDE_HANDLE s32Handle, TDE_SURFACE_S *pstSrc, TDE_SURFACE_S *pstDst, TDE_OPT_S *pstOpt);

/*****************************************************************************
 Prototype    : HI_API_TDE_PatternFill
 Description  : add a patternfill command to the job
 Input        : TDE_HANDLE s32Handle       job handle
                TDE_PATTERN_S *pstPattern  pattern surface
                TDE_SURFACE_S *pstDst      destination surface
                TDE_OPT_S *pstOpt          operation attribute
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_NULL_PTR
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_NO_MEM
                HI_ERR_TDE_INVALID_PARA
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_PatternFill(TDE_HANDLE s32Handle, TDE_PATTERN_S *pstPattern,  TDE_SURFACE_S *pstDst, TDE_OPT_S *pstOpt);

/*****************************************************************************
 Prototype    : HI_API_TDE_SolidDraw
 Description  : add a soliddraw command to the job
 Input        : TDE_HANDLE s32Handle   job handle
                TDE_SURFACE_S *pstDst  destination surface
                TDE_RGB_S stFillColor  the draw color
                TDE_OPT_S *pstOpt      operation attribute
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_NULL_PTR
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_NO_MEM
                HI_ERR_TDE_INVALID_PARA
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_SolidDraw(TDE_HANDLE s32Handle, TDE_SURFACE_S *pstDst, TDE_RGB_S stFillColor, TDE_OPT_S *pstOpt);

/*****************************************************************************
 Prototype    : HI_API_TDE_Deflicker
 Description  : add a deflicker command to the job
 Input        : TDE_HANDLE s32Handle            job handle
                TDE_SURFACE_S *pstSrc           source surface, the deflicker object
                TDE_SURFACE_S *pstDst           destination surface which the deflicker result output to 
                TDE_DEFLICKER_COEF_S stDfeCoef  deflicker coefficient 
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_NULL_PTR
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_Deflicker(TDE_HANDLE s32Handle, TDE_SURFACE_S *pstSrc, TDE_SURFACE_S *pstDst, TDE_DEFLICKER_COEF_S stDfeCoef);

/*****************************************************************************
 Prototype    : HI_API_TDE_EndJob
 Description  : submit a job
 Input        : TDE_HANDLE s32Handle  job handle
                HI_BOOL bBlock        can be blockable
                HI_U32 u32TimeOut     the maximal jiffies to wait if \a bBlock is true 
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_JOB_TIMEOUT
                HI_ERR_TDE_JOB_INTERRUPT
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_EndJob(TDE_HANDLE s32Handle, HI_BOOL bBlock, HI_U32 u32TimeOut);

/*****************************************************************************
 Prototype    : HI_API_TDE_WaitForDone
 Description  : to wait all the tde command which submit before the call
                done 
 Input        : HI_VOID  
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_DEV_NOT_OPEN
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_API_TDE_WaitForDone(HI_VOID);


/*****************************************************************************
 Prototype    : HI_API_TDE_Close
 Description  : close the TDE device
 Input        : HI_VOID  
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_DEV_NOT_OPEN
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32 HI_API_TDE_Close(HI_VOID);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __TDE_INTERFACE_H__ */



