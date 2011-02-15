/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_adec.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/08
  Description   : Common Def Of aio 
  History       :
  1.Date        : 2006/12/15
    Author      : z50825
    Modification: Created file
  2.Date           : 2006/12/22
    Author         : zhourui 50825
    PreVersion     : 1.3
    NewVersion     : 1.4
    Modification   : Modify AUDIO_FRAME_S to AUDIO_FRAME_INFO_S. 
                     Resolve the problem about audio frame release
******************************************************************************/

#ifndef _MPI_ADEC_H__
#define _MPI_ADEC_H__

#include "hi_common.h"
#include "hi_comm_aio.h"
#include "hi_comm_adec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */



HI_S32 HI_MPI_ADEC_CreateChn(ADEC_CHN AdChn, ADEC_CHN_ATTR_S *pstAttr);
HI_S32 HI_MPI_ADEC_DestroyChn(ADEC_CHN AdChn);
HI_S32 HI_MPI_ADEC_SetChnAttr(ADEC_CHN AdChn, ADEC_CHN_ATTR_S * pstAttr);
HI_S32 HI_MPI_ADEC_GetChnAttr(ADEC_CHN AdChn, ADEC_CHN_ATTR_S * pstAttr);
HI_S32 HI_MPI_ADEC_SendStream(ADEC_CHN AdChn, const AUDIO_STREAM_S *pstStream) ;
HI_S32 HI_MPI_ADEC_GetData(ADEC_CHN AdChn, AUDIO_FRAME_INFO_S    *pstFrm); 
HI_S32 HI_MPI_ADEC_ReleaseData(ADEC_CHN AdChn, AUDIO_FRAME_INFO_S *pstFrm);

HI_S32 HI_MPI_ADEC_GetAacFrmInfo(ADEC_CHN AdChn, AAC_FRAME_INFO *pstAacFrmInfo);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

