/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_common.h
Version       : Initial Draft
Author        : Hi3511 MPP Team
Created       : 2006/11/09
Last Modified :
Description   : The common defination
Function List :
History       :
 1.Date        : 2006/11/03
   Author      : c42025
   Modification: Created file

 2.Date        : 2007/06/12
   Author      : c42025
   Modification: add data structure POINT_S

 3.Date        : 2007/11/26
   Author      : c42025
   Modification: add definition about maxium chanels and devices

 4.Date        : 2008/01/30
   Author      : c42025
   Modification: modified value of macro "VI_MAX_CHN_NUM"
   "VI_MAX_DEV_NUM"  "VIU_MAX_CHN_NUM_PER_DEV".

 5.Date        : 2008/03/03
   Author      : c42025
   Modification: add definition for name of modules and version management.

 6.Date        : 2008/03/06
   Author      : c42025
   Modification: add definition HI_ID_AVENC and MOD_NAME_CMPI

 7.Date        : 2008/03/26
   Author      : c42025
   Modification: add definition "HI3511_VER_V100" & "HI3511_VER_V110"

 8.Date        : 2008/04/28
   Author      : c42025
   Modification: add definition "VER_SP" for patch

 9.Date        : 2008/06/18
   Author      : c42025
   Modification: add serveral reservation module ID.

10.Date        : 2008/06/28
   Author      : c42025
   Modification: add function to operation cache.

11.Date         : 2008/9/8
   Author       : x00100808
   Modification : add enum type "VOU_WHO_SENDPIC_E".

12.Date         : 2008/9/26
   Author       : x00100808
   Modification : add vo mulit-channel synchronous group macro definition.

13.Date         :   2008/10/31
   Author       :   z44949
   Modification :   Translate the chinese comment

14.Date         :   2008/11/15
   Author       :   p00123320
   Modification :   modify VIU_MAX_CHN_NUM_PER_DEV to 4 from 2

15.Date         :   2008/12/08
   Author       :   z44949
   Modification :   modify AVENC_CHN_MAXNUM from 16 to 32

16.Date         : 2009/3/19
   Author       : x00100808
   Modification : extend vou channel max number from 16 to 32

******************************************************************************/
#ifndef __HI_COMMON_H__
#define __HI_COMMON_H__

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define HI3511_VER_V100 0x35110100
#define HI3511_VER_V110 0x35110110

#ifndef VER_X
#define VER_X 1
#endif

#ifndef VER_Y
#define VER_Y 0
#endif

#ifndef VER_Z
#define VER_Z 0
#endif

#ifndef VER_P
#define VER_P 0
#endif

#ifndef VER_S
#define VER_S 0
#endif

#if VER_S==0
#define VER_SP(s) ""
#else
#define __VER_SP(s) #s
#define VER_SP(s) "_SP"__VER_SP(s)
#endif

#ifdef HI_DEBUG
#define VER_D " Debug"
#else
#define VER_D " Release"
#endif

#define MPP_VER_PRIX "Hi3511_MPP_V"

#define __MK_VERSION(x,y,z,p) #x"."#y"."#z"."#p
#define MK_VERSION(x,y,z,p) __MK_VERSION(x,y,z,p)
#define MPP_VERSION MPP_VER_PRIX MK_VERSION(VER_X,VER_Y,VER_Z,VER_P) VER_SP(VER_S) VER_D

typedef struct hiMPP_VERSION_S
{
	HI_CHAR aVersion[64];
}MPP_VERSION_S;

typedef HI_S32 AI_CHN;
typedef HI_S32 AO_CHN;
typedef HI_S32 AENC_CHN;
typedef HI_S32 ADEC_CHN;
typedef HI_S32 AUDIO_DEV;
typedef HI_S32 AVENC_CHN;

typedef HI_S32 VI_DEV;
typedef HI_S32 VI_CHN;
typedef HI_S32 VO_CHN;
typedef HI_S32 VENC_CHN;
typedef HI_S32 VDEC_CHN;
typedef HI_S32 VENC_GRP;

typedef HI_S32 VO_GRP;

#define HI_INVALID_CHN (-1)
#define HI_INVALID_DEV (-1)

/* total number of video input channels including
 * VIU hardware channle and virtual channle.
 */
#define VI_MAX_CHN_NUM       8
#define VI_MAX_DEV_NUM       8

/* number of channle and device on video input unit of chip
 * Note! VIU_MAX_CHN_NUM is NOT equal to VIU_MAX_DEV_NUM
 * multiplied by VIU_MAX_CHN_NUM_PER_DEV, because all VI devices
 * can't work at mode of 4 channles at the same time.
 */
#define VIU_MAX_DEV_NUM      4
#define VIU_MAX_CHN_NUM_PER_DEV      4
#define VIU_MAX_CHN_NUM      8

#define VO_MAX_CHN_NUM      32

#define VENC_MAX_CHN_NUM    32
#define VDEC_MAX_CHN_NUM    32
#define VENC_MAX_GRP_NUM    32

#define SIO_MAX_NUM         2
#define AIO_MAX_CHN_NUM      8

#define AENC_MAX_CHN_NUM    32
#define ADEC_MAX_CHN_NUM    32

#define AVENC_CHN_MAXNUM    32

#define VO_SYNC_MAX_GROUP   8
#define VO_SYNC_MAX_CHNL    16

#define HI_IO_BLOCK   0
#define HI_IO_NOBLOCK 1

#define  HI_LITTLE_ENDIAN   1234       /* little endian */
#define  HI_BIG_ENDIAN      4321       /* big endian    */

typedef struct hiPOINT_S
{
    HI_S32 s32X;
    HI_S32 s32Y;
}POINT_S;

typedef struct hiDIMENSION_S
{
    HI_S32 s32Width;
    HI_S32 s32Height;
}DIMENSION_S;

typedef struct hiRECT_S
{
    HI_S32 s32X;
    HI_S32 s32Y;
    HI_U32 u32Width;
    HI_U32 u32Height;
}RECT_S;

typedef enum hiMOD_ID_E
{
	HI_ID_CMPI = 0,
	HI_ID_VB,
	HI_ID_SYS,
	HI_ID_VALG,

	HI_ID_CHNL = 4,
	HI_ID_GROUP,
	HI_ID_VENC,
	HI_ID_VPP,

	HI_ID_MD = 8,
	HI_ID_H264E,
	HI_ID_JPEGE,
	HI_ID_MPEG4E,

	HI_ID_VDEC = 12,
	HI_ID_H264D,
	HI_ID_JPEGD,
	HI_ID_VOU,

	HI_ID_VIU = 16,
	HI_ID_DSU,

	HI_ID_SIO = 20,
	HI_ID_AI,
	HI_ID_AO,
	HI_ID_AENC,
	HI_ID_ADEC,

	HI_ID_AVENC = 25,
    HI_ID_PCIV = 26,

    HI_ID_RSV0 = 27,
    HI_ID_RSV1 = 28,
    HI_ID_RSV2 = 29,
    HI_ID_RSV3 = 30,

	HI_ID_BUTT,
} MOD_ID_E;

#define MPP_MOD_VIU    "vi"
#define MPP_MOD_VOU    "vo"
#define MPP_MOD_DSU    "dsu"

#define MPP_MOD_CHNL   "chnl"
#define MPP_MOD_VENC   "venc"
#define MPP_MOD_GRP    "grp"
#define MPP_MOD_MD     "md"
#define MPP_MOD_VPP    "vpp"

#define MPP_MOD_H264E  "h264e"
#define MPP_MOD_JPEGE  "jpege"
#define MPP_MOD_MPEG4E "mpeg4e"

#define MPP_MOD_VDEC   "vdec"
#define MPP_MOD_H264D  "h264d"
#define MPP_MOD_JPEGD  "jpegd"

#define MPP_MOD_AI     "ai"
#define MPP_MOD_AO     "ao"
#define MPP_MOD_AENC   "aenc"
#define MPP_MOD_ADEC   "adec"
#define MPP_MOD_SIO   "sio"

#define MPP_MOD_VB     "vb"
#define MPP_MOD_SYS    "sys"
#define MPP_MOD_CMPI   "cmpi"

#define MPP_MOD_PCIV   "pciv"

/* We just coyp this value of payload type from RTP/RTSP definition */
typedef enum
{
	PT_PCMU = 0,
	PT_1016 = 1,
	PT_G721 = 2,
	PT_GSM = 3,
	PT_G723 = 4,
	PT_DVI4_8K = 5,
	PT_DVI4_16K = 6,
	PT_LPC = 7,
	PT_PCMA = 8,
	PT_G722 = 9,
	PT_S16BE_STEREO,
	PT_S16BE_MONO = 11,
	PT_QCELP = 12,
	PT_CN = 13,
	PT_MPEGAUDIO = 14,
	PT_G728 = 15,
	PT_DVI4_3 = 16,
	PT_DVI4_4 = 17,
	PT_G729 = 18,
	PT_G711A = 19,
	PT_G711U = 20,
	PT_G726 = 21,
	PT_G729A = 22,
	PT_LPCM = 23,
	PT_CelB = 25,
	PT_JPEG = 26,
	PT_CUSM = 27,
	PT_NV = 28,
	PT_PICW = 29,
	PT_CPV = 30,
	PT_H261 = 31,
	PT_MPEGVIDEO = 32,
	PT_MPEG2TS = 33,
	PT_H263 = 34,
	PT_SPEG = 35,
	PT_MPEG2VIDEO = 36,
	PT_AAC = 37,
	PT_WMA9STD = 38,
	PT_HEAAC = 39,
	PT_PCM_VOICE = 40,
	PT_PCM_AUDIO = 41,
	PT_AACLC = 42,
	PT_MP3 = 43,
	PT_ADPCMA = 49,
	PT_AEC = 50,
	PT_X_LD = 95,
	PT_H264 = 96,
	PT_D_GSM_HR = 200,
	PT_D_GSM_EFR = 201,
	PT_D_L8 = 202,
	PT_D_RED = 203,
	PT_D_VDVI = 204,
	PT_D_BT656 = 220,
	PT_D_H263_1998 = 221,
	PT_D_MP1S = 222,
	PT_D_MP2P = 223,
	PT_D_BMPEG = 224,
	PT_MP4VIDEO = 230,
	PT_MP4AUDIO = 237,
	PT_VC1 = 238,
	PT_JVC_ASF = 255,
	PT_D_AVI = 256,
	PT_MAX = 257,

	PT_AMR = 1001, /* add by mpp */
	PT_MJPEG = 1002,
}PAYLOAD_TYPE_E;

typedef enum hiVOU_WHO_SENDPIC_E
{
    VOU_WHO_SENDPIC_VIU,
    VOU_WHO_SENDPIC_VDEC,
    VOU_WHO_SENDPIC_PCIV,
    VOU_WHO_SENDPIC_BUTT
} VOU_WHO_SENDPIC_E;

static inline HI_VOID HI_MemSet32(HI_VOID *pDest,
    HI_U32 u32Value, HI_U32 u32Cnt)
{
    HI_U32 i, *p = (HI_U32 *)pDest;
    for (i = 0; i < u32Cnt; i++)
    {
        *p++ = u32Value;
    }
}

#define LINE_LEN_BIT            5
#define LINE_LEN                (1<<LINE_LEN_BIT)
#define LINE_BASE_MASK          (~(LINE_LEN-1))
static inline void InvalidateDcache(unsigned long addr, unsigned long len)
{
    unsigned long end;

    addr &= LINE_BASE_MASK;
    len >>= LINE_LEN_BIT;
    end   = addr + len*LINE_LEN;

    while(addr != end)
    {
        asm("mcr p15, 0, %0, c7, c6, 1"::"r"(addr));
        addr += LINE_LEN;
    }
    return;
}

static inline  void FlushDcache(unsigned long addr, unsigned long len)
{
    unsigned long end;

    addr &= LINE_BASE_MASK;
    len >>= LINE_LEN_BIT;
    end   = addr + len*LINE_LEN;

    while(addr != end)
    {
        asm("mcr p15, 0, %0, c7, c10, 1"::"r"(addr));
        addr += LINE_LEN;
    }
    return;
}


#ifdef __KERNEL__
#include <asm/system.h>
#include <asm/io.h>

static inline void HI_RegSetBit(unsigned long value, unsigned long offset,
    unsigned long addr)
{
	unsigned long t, mask;

	mask = 1 << offset;
	t = readl(addr);
	t &= ~mask;
	t |= (value << offset) & mask;
	writel(t, addr);
}

static inline void HI_RegWrite32(unsigned long value, unsigned long mask,
    unsigned long addr)
{
	unsigned long t;

	t = readl(addr);
	t &= ~mask;
	t |= value & mask;
	writel(t, addr);
}

#endif /* __KERNEL__ */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* _HI_COMMON_H_ */

