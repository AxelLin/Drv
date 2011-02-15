#ifndef __PCI_TEST_COMM_H__
#define __PCI_TEST_COMM_H__


#include "hi_common.h"
#include "hi_type.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"

#include "hi_comm_pciv.h"
#include "mpi_pciv.h"

#define PCIV_TEST_VI_PIXEL_FORMAT   PIXEL_FORMAT_YUV_SEMIPLANAR_422 /* 从片VI的图像格式*/

#if 0
typedef enum hiPCIV_VOMAP_TYPE_E
{
    PCIV_VOMAP_CREATE,
    PCIV_VOMAP_RESET,
    PCIV_VOMAP_DESTROY
} PCIV_VOMAP_TYPE_E;
#endif

typedef struct hiPCIV_PORTMAP_VO_S
{
    PCIV_DEVICE_S     stLocalDev;
    HI_S32            s32RmtTgt;
    PCIV_DEVICE_S     stRmtDev;
    PCIV_BIND_OBJ_S   stRmtObj;
} PCIV_PORTMAP_S;


#endif
