/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_tde.h
  Version       : Initial Draft
  Author        : w54130
  Created       : 2007/5/21
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2007/5/21
    Author      : w54130
    Modification: Created file

  2.Date        : 2008/09/05
    Author      : w54130
    Modification:CR20080829027(add function HI_TDE_WMemcpy)
******************************************************************************/
#ifndef __HI_TDE_H__
#define __HI_TDE_H__

#include <tde_interface.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/*****************************************************************************
 Prototype    : HI_TDE_BeginJob
 Description  : start a TDE job
 Input        : HI_VOID  
 Output       : None
 Return Value : TDE_HANDLE:job handle
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
TDE_HANDLE  HI_TDE_BeginJob(HI_VOID);

/*****************************************************************************
 Prototype    : HI_TDE_BitBlt
 Description  : add a blit command to the job
 Input        : TDE_HANDLE s32Handle   job handle
                TDE_SURFACE_S *pstSrc  source surface
                TDE_SURFACE_S *pstDst  destination surface
                TDE_OPT_S *pstOpt      operation attribute
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_NULL_PTR
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_BitBlt(TDE_HANDLE s32Handle, TDE_SURFACE_S *pstSrc, TDE_SURFACE_S *pstDst, TDE_OPT_S *pstOpt);

/*****************************************************************************
 Prototype    : HI_TDE_PatternFill
 Description  : add a patternfill command to the job
 Input        : TDE_HANDLE s32Handle       job handle
                TDE_PATTERN_S *pstPattern  pattern surface
                TDE_SURFACE_S *pstDst      destination surface
                TDE_OPT_S *pstOpt          operation attribute
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_NULL_PTR
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_PatternFill(TDE_HANDLE s32Handle, TDE_PATTERN_S *pstPattern,  TDE_SURFACE_S *pstDst, TDE_OPT_S *pstOpt);

/*****************************************************************************
 Prototype    : HI_TDE_SolidDraw
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
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_SolidDraw(TDE_HANDLE s32Handle, TDE_SURFACE_S *pstDst, TDE_RGB_S stFillColor, TDE_OPT_S *pstOpt);

/*****************************************************************************
 Prototype    : HI_TDE_Deflicker
 Description  : add a deflicker command to the job
 Input        : TDE_HANDLE s32Handle            job handle
                TDE_SURFACE_S *pstSrc           source surface, the deflicker object
                TDE_SURFACE_S *pstDst           destination surface which the deflicker result output to 
                TDE_DEFLICKER_COEF_S stDfeCoef  deflicker coefficient 
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_NULL_PTR
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_Deflicker(TDE_HANDLE s32Handle, TDE_SURFACE_S *pstSrc, TDE_SURFACE_S *pstDst, TDE_DEFLICKER_COEF_S stDfeCoef);

/*****************************************************************************
 Prototype    : HI_TDE_EndJob
 Description  : submit a job
 Input        : TDE_HANDLE s32Handle  job handle
                HI_BOOL bBlock        can be blockable
                HI_U32 u32TimeOut     the maximal jiffies to wait if \a bBlock is true 
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
                HI_ERR_TDE_JOB_TIMEOUT
                HI_ERR_TDE_JOB_INTERRUPT
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_EndJob(TDE_HANDLE s32Handle, HI_BOOL bBlock, HI_U32 u32TimeOut);

/*****************************************************************************
 Prototype    : HI_TDE_EndJob4Int
 Description  : submit a job in interrupt routine
 Input        : TDE_HANDLE s32Handle  job handle
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_INVALID_HANDLE
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_EndJob4Int(TDE_HANDLE s32Handle);


/*****************************************************************************
 Prototype    : HI_TDE_WaitForDone
 Description  : to wait all the tde command which submit before the call
                done 
 Input        : HI_VOID  
 Output       : None
 Return Value : HI_SUCCESS
                HI_ERR_TDE_NO_MEM
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  HI_TDE_WaitForDone(HI_VOID);

HI_S32 HI_TDE_Open(HI_VOID);
HI_S32 HI_TDE_Close(HI_VOID);

HI_S32 HI_TDE_WMemset( 
    HI_U32 u32Width, HI_U32 u32Height, 
    HI_U32* u32Addr, HI_U32 u32Stride,
    HI_U32 u32Data);

HI_S32 HI_TDE_WMemcpy( 
    HI_U8 *pu8SrcPhyAddr, HI_U32 u32SrcStride, TDE_COLOR_FMT_E enSrcFmt,
    HI_U8 *pu8DstPhyAddr, HI_U32 u32DstStride, TDE_COLOR_FMT_E enDstFmt,
    HI_U32 u32Width, HI_U32 u32Height);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __HI_TDE_H__ */






