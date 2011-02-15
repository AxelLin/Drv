/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_aenc.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/08
  Description   : Common Def Of aio 
  History       :
  1.Date        : 2006/12/15
    Author      : z50825
    Modification: Created file
******************************************************************************/

#ifndef _MPI_AENC_H__
#define _MPI_AENC_H__

#include "hi_common.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


HI_S32 HI_MPI_AENC_CreateChn(AENC_CHN AeChn, const AENC_CHN_ATTR_S *pstAttr);
HI_S32 HI_MPI_AENC_DestroyChn(AENC_CHN AeChn);
HI_S32 HI_MPI_AENC_SetChnAttr(AENC_CHN AeChn, const AENC_CHN_ATTR_S * pstAttr);
HI_S32 HI_MPI_AENC_GetChnAttr(AENC_CHN AeChn, AENC_CHN_ATTR_S * pstAttr);
HI_S32 HI_MPI_AENC_SendFrame(AENC_CHN AeChn, const AUDIO_FRAME_S * pstData, const AUDIO_FRAME_S * pstRefData);
HI_S32 HI_MPI_AENC_GetStream(AENC_CHN AeChn, AUDIO_STREAM_S *pstStream); 
HI_S32 HI_MPI_AENC_ReleaseStream(AENC_CHN AeChn, const AUDIO_STREAM_S *pstStream);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

