/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_comm_ai.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/15
  Description   : Common Def Of AI 
  History       :
  1.Date        : 2006/11/16
    Author      : z50825
    Modification: Created file
  2.Date        : 2007/05/10
    Author      : z50825
    Modification: add err code

******************************************************************************/

#ifndef __HI_COMM_AI_H__
#define __HI_COMM_AI_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/* invlalid device ID */
#define HI_ERR_AI_INVALID_DEVID     HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* invlalid channel ID */
#define HI_ERR_AI_INVALID_CHNID     HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define HI_ERR_AI_ILLEGAL_PARAM     HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* using a NULL point */
#define HI_ERR_AI_NULL_PTR          HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define HI_ERR_AI_NOT_CONFIG        HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define HI_ERR_AI_NOT_SURPPORT      HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SURPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define HI_ERR_AI_NOT_PERM          HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* the devide is not enabled  */
#define HI_ERR_AI_NOT_ENABLED             HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* failure caused by malloc memory */
#define HI_ERR_AI_NOMEM             HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define HI_ERR_AI_NOBUF             HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define HI_ERR_AI_BUF_EMPTY         HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define HI_ERR_AI_BUF_FULL          HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded*/
#define HI_ERR_AI_SYS_NOTREADY      HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)

#define HI_ERR_AI_BUSY      HI_DEF_ERR(HI_ID_AI, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_COMM_AI_H__ */

