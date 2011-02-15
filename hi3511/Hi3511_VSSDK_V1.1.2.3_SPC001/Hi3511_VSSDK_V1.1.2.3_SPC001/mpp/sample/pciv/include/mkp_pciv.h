/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mkp_pciv.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/04/05
  Description   : 
  History       :
  1.Date        : 2008/06/18
    Author      : zhourui50825
    Modification: Created file

******************************************************************************/
#ifndef __MKP_PCIV_H__
#define __MKP_PCIV_H__

#include "hi_common.h"
#include "hi_comm_pciv.h"
#include "mkp_ioctl.h"
#include "hi_comm_vdec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum hiIOC_NR_PCIV_E
{
	IOC_NR_PCIV_CREATE = 0,
	IOC_NR_PCIV_DESTROY,	
	IOC_NR_PCIV_SETATTR,
	IOC_NR_PCIV_GETATTR,	
	IOC_NR_PCIV_START,
	IOC_NR_PCIV_STOP,	
    IOC_NR_PCIV_MALLOC,
    IOC_NR_PCIV_FREE,  
    IOC_NR_PCIV_BIND,
    IOC_NR_PCIV_UNBIND,    
	IOC_NR_PCIV_ENABLE_DEFLICKER,
	IOC_NR_PCIV_GETBASEWINDOW,
	IOC_NR_PCIV_GETLOCALID,
	IOC_NR_PCIV_DMATASK,
} IOC_NR_PCIV_E;

#define PCIV_CREATE_DEVICE_CTRL  _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_CREATE, PCIV_DEVICE_S)
#define PCIV_DESTROY_DEVICE_CTRL _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_DESTROY, PCIV_DEVICE_S)
#define PCIV_SETATTR_CTRL  _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_SETATTR,PCIV_DEVICE_ATTR_S)
#define PCIV_GETATTR_CTRL  _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_GETATTR,PCIV_DEVICE_ATTR_S)
#define PCIV_START_CTRL    _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_START, PCIV_DEVICE_S)
#define PCIV_STOP_CTRL     _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_STOP, PCIV_DEVICE_S)
#define PCIV_MALLOC_CTRL   _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_MALLOC,PCIV_DEVICE_ATTR_S)
#define PCIV_FREE_CTRL     _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_FREE,PCIV_DEVICE_S)
#define PCIV_BIND_CTRL     _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_BIND,PCIV_BIND_S)
#define PCIV_UNBIND_CTRL   _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_UNBIND,PCIV_BIND_S)
#define PCIV_DMATASK_CTRL  _IOW (IOC_TYPE_PCIV, IOC_NR_PCIV_DMATASK, PCIV_DMA_TASK_S)
#define PCIV_ENABLE_DEFLICKER_CTRL _IO(IOC_TYPE_PCIV, IOC_NR_PCIV_ENABLE_DEFLICKER)
#define PCIV_GETBASEWINDOW_CTRL _IOWR(IOC_TYPE_PCIV, IOC_NR_PCIV_GETBASEWINDOW, PCIV_BASEWINDOW_S)
#define PCIV_GETLOCALID_CTRL    _IOR (IOC_TYPE_PCIV, IOC_NR_PCIV_GETLOCALID, HI_U32)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MPI_PRIV_PCIV_H__ */



