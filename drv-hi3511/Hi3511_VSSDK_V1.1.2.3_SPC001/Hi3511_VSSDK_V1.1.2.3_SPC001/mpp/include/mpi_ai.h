/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_ai.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/08
  Description   : Common Def Of aio 
  History       :
  1.Date        : 2006/12/15
    Author      : z50825
    Modification: Created file
******************************************************************************/

#ifndef _MPI_AI_H__
#define _MPI_AI_H__

#include "hi_type.h"
#include "hi_common.h"
#include "hi_comm_aio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


HI_S32 HI_MPI_AI_SetPubAttr(AUDIO_DEV AudioDevId, const AIO_ATTR_S *pstAttr);
HI_S32 HI_MPI_AI_GetPubAttr(AUDIO_DEV AudioDevId, AIO_ATTR_S *pstAttr);

HI_S32 HI_MPI_AI_Enable(AUDIO_DEV AudioDevId);
HI_S32 HI_MPI_AI_Disable(AUDIO_DEV AudioDevId);

HI_S32 HI_MPI_AI_EnableChn(AUDIO_DEV AudioDevId, AI_CHN AiChn);
HI_S32 HI_MPI_AI_DisableChn(AUDIO_DEV AudioDevId, AI_CHN AiChn);

HI_S32 HI_MPI_AI_EnableAec(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_DEV AoDevId, AO_CHN AoChn);
HI_S32 HI_MPI_AI_DisableAec(AUDIO_DEV AiDevId, AI_CHN AiChn);

HI_S32 HI_MPI_AI_GetFrame(AUDIO_DEV AudioDevId, AI_CHN AiChn, 
	    AUDIO_FRAME_S *pstData, AEC_FRAME_S *pstAecData, HI_U32 u32Block);

HI_S32 HI_MPI_AI_GetFd(AUDIO_DEV AudioDevId, AI_CHN AiChn);

/* Close all the Fd of AI device and channel */
HI_S32 HI_MPI_AI_CloseFd(HI_VOID);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

