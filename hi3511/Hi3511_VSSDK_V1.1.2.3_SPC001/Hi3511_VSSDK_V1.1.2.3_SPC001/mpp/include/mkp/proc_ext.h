/******************************************************************************
 
  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : proc_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/09
  Description   : 
  History       :
  1.Date        : 2006/12/09
    Author      : c42025
    Modification: Created file

  2.Date        : 2007/12/08
    Author      : c42025
    Modification: add a name for "group" 

  3.Date        : 2008/03/03
    Author      : c42025
    Modification: modified macros about PRO_ENTRY_XXX. 
    add support write fuction.

  4.Date        : 2008/06/18
    Author      : c42025
    Modification: add macros about PRO_ENTRY_PCIV
******************************************************************************/

#ifndef __PROC_EXT_H__
#define __PROC_EXT_H__

#include <linux/seq_file.h>
#include "hi_type.h"
#include "hi_common.h"

#define PROC_ENTRY_VIU   MPP_MOD_VIU  
#define PROC_ENTRY_VOU 	 MPP_MOD_VOU 
#define PROC_ENTRY_DSU 	 MPP_MOD_DSU 
#define PROC_ENTRY_VENC	 MPP_MOD_VENC
#define PROC_ENTRY_VDEC	 MPP_MOD_VDEC
#define PROC_ENTRY_MD  	 MPP_MOD_MD  
#define PROC_ENTRY_H264E  MPP_MOD_H264E 
#define PROC_ENTRY_H264D  MPP_MOD_H264D 
#define PROC_ENTRY_JPEGE  MPP_MOD_JPEGE 
#define PROC_ENTRY_JPEGD  MPP_MOD_JPEGD 
#define PROC_ENTRY_CHNL  MPP_MOD_CHNL 
#define PROC_ENTRY_AI   MPP_MOD_AI  
#define PROC_ENTRY_AO   MPP_MOD_AO  
#define PROC_ENTRY_VB   MPP_MOD_VB  
#define PROC_ENTRY_GRP  MPP_MOD_GRP 
#define PROC_ENTRY_VPP  MPP_MOD_VPP 
#define PROC_ENTRY_SYS  MPP_MOD_SYS 
#define PROC_ENTRY_PCIV  MPP_MOD_PCIV
#define PROC_ENTRY_MPEG4E MPP_MOD_MPEG4E


#define PROC_ENTRY_LOG  "log"

#define CMPI_PROC_SHOW CMPI_PROC_READ
typedef HI_S32 (*CMPI_PROC_READ)(struct seq_file *, HI_VOID *);	
typedef HI_S32 (*CMPI_PROC_WRITE)(struct file * file, 
    const char __user * buf, size_t count, loff_t *ppos);

typedef struct struCMPI_PROC_ITEM
{
    HI_CHAR entry_name[32];
	struct proc_dir_entry *entry;
	CMPI_PROC_READ read;
    CMPI_PROC_WRITE write;
	HI_VOID *pData;
}CMPI_PROC_ITEM_S;

extern CMPI_PROC_ITEM_S *CMPI_CreateProc(HI_CHAR *, CMPI_PROC_SHOW, HI_VOID *);
extern HI_VOID CMPI_RemoveProc(HI_CHAR *);

extern HI_VOID CMPI_ProcInit(HI_VOID);
extern HI_VOID CMPI_ProcExit(HI_VOID);

extern HI_S32 CMPI_LogInit(HI_U32);
extern HI_VOID CMPI_LogExit(HI_VOID);


#endif
