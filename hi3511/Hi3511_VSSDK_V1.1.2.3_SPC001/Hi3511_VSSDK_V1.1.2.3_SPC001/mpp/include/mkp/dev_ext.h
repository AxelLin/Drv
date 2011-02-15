/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : dev_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/11/03
  Description   :
  History       :
  1.Date        : 2006/11/03
    Author      : c42025
    Modification: Created file

  1.Date        : 2007/11/16
    Author      : c42025
    Modification: modified definition of UMAP_DEV_NUM

  2.Date        : 2008/10/16
    Author      : z44949
    Modification: Modify the log's device path from "umap/video" to "umap"

  3.Date         : 2009/3/19
    Author       : x00100808
    Modification : extend UMAP_DEV_NUM from 192 to 256, and modity each device's
                   minor base address.

******************************************************************************/
#ifndef _DEV_EXT_H_
#define _DEV_EXT_H_

#include <linux/device.h>
#include <linux/fs.h>

#include "hi_type.h"

#define UMAP_NAME	"umap"
#define UMAP_DEV_NUM	256

#define UMAP_VI_MINOR_BASE   0x10  /* 16 device nodes */
#define UMAP_VO_MINOR_BASE   0x20  /* 32 device nodes */
#define UMAP_VENC_MINOR_BASE 0x40  /* 32 device nodes */
#define UMAP_VDEC_MINOR_BASE 0x60  /* 32 device nodes */
#define UMAP_MD_MINOR_BASE   0x80  /* 32 device nodes */

#define UMAP_AI_MINOR_BASE   0xa0  /* 16 device nodes */
#define UMAP_AO_MINOR_BASE   0xb0  /* 16 device nodes */

#define UMAP_GRP_MINOR_BASE  0xc0  /* 32 device nodes */

#define UMAP_SYS_MINOR_BASE  0xf0  /* 1 device nodes */
#define UMAP_VB_MINOR_BASE   0xf1  /* 1 device nodes */
#define UMAP_VPP_MINOR_BASE  0xf2  /* 1 device nodes */
#define UMAP_SIO_MINOR_BASE  0xf3  /* 2 device nodes */
#define UMAP_LOG_MINOR_BASE  0xf5  /* 1 device nodes */

#define UMAP_AI_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_AI_MINOR_BASE)
#define UMAP_AO_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_AO_MINOR_BASE)

#define UMAP_SIO_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_SIO_MINOR_BASE)

#define UMAP_VI_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_VI_MINOR_BASE)
#define UMAP_VO_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_VO_MINOR_BASE)
#define UMAP_VENC_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_VENC_MINOR_BASE)
#define UMAP_VDEC_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_VDEC_MINOR_BASE)
#define UMAP_MD_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_MD_MINOR_BASE)
#define UMAP_VB_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_VB_MINOR_BASE)
#define UMAP_VPP_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_VPP_MINOR_BASE)
#define UMAP_GRP_GET_CHN(f) \
		(iminor(f->f_dentry->d_inode)-UMAP_GRP_MINOR_BASE)

#define UMAP_DEVNAME_SYSCTL     "umap/sysctl"
#define UMAP_DEVNAME_LOG_BASE   "umap/log"

#define UMAP_DEVNAME_VIDEO      "umap/video/"
#define UMAP_DEVNAME_VI_BASE    UMAP_DEVNAME_VIDEO"vi"
#define UMAP_DEVNAME_VO_BASE    UMAP_DEVNAME_VIDEO"vo"
#define UMAP_DEVNAME_VENC_BASE  UMAP_DEVNAME_VIDEO"venc"
#define UMAP_DEVNAME_VDEC_BASE  UMAP_DEVNAME_VIDEO"vdec"
#define UMAP_DEVNAME_OSD_BASE   UMAP_DEVNAME_VIDEO"osd"
#define UMAP_DEVNAME_DSU_BASE   UMAP_DEVNAME_VIDEO"dsu"
#define UMAP_DEVNAME_MD_BASE    UMAP_DEVNAME_VIDEO"md"
#define UMAP_DEVNAME_VB_BASE    UMAP_DEVNAME_VIDEO"vb"
#define UMAP_DEVNAME_VPP_BASE   UMAP_DEVNAME_VIDEO"vpp"
#define UMAP_DEVNAME_GRP_BASE   UMAP_DEVNAME_VIDEO"grp"


#define UMAP_DEVNAME_AUDIO      "umap/audio/"
#define UMAP_DEVNAME_SIO_BASE   UMAP_DEVNAME_AUDIO"sio"
#define UMAP_DEVNAME_AI_BASE    UMAP_DEVNAME_AUDIO"ai"
#define UMAP_DEVNAME_AO_BASE    UMAP_DEVNAME_AUDIO"ao"
#define UMAP_DEVNAME_AENC_BASE  UMAP_DEVNAME_AUDIO"aenc"
#define UMAP_DEVNAME_ADEC_BASE  UMAP_DEVNAME_AUDIO"adec"

typedef struct hiUMAP_DEVICE_S
{
	HI_CHAR devfs_name[64];		/* devfs */
	HI_U32  minor;
	HI_U32  users_num;

	struct file_operations *fops;
	struct class_device class_dev;	/* sysfs */
}UMAP_DEVICE_S, *PTR_UMAP_DEVICE_S;

extern HI_S32  CMPI_DevInit(HI_VOID);
extern HI_VOID CMPI_DevExit(HI_VOID);
extern HI_S32  CMPI_RegisterDevice(UMAP_DEVICE_S *umapd);
extern HI_VOID CMPI_UnRegisterDevice(UMAP_DEVICE_S *umapd);
extern HI_S32  CMPI_UserCopy(struct inode *inode, struct file *file,
				unsigned cmd, unsigned long arg,
				int (*func)(struct inode *inode, struct file *file,
						unsigned cmd, unsigned long arg));

#endif /* _DEV_EXT_H_ */
