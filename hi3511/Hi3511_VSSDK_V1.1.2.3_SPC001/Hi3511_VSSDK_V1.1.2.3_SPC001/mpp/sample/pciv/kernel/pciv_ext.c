/******************************************************************************
 
  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : pciv_ext.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/16
  Description   : 
  History       :
  1.Date        : 2008/06/16
    Author      :  
    Modification: Created file

  2.Date        : 2008/11/20
    Author      : z44949
    Modification: Modify to support VDEC 

******************************************************************************/
#include "pciv_ext.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>

#include "hi_errno.h"
#include "hi_debug.h"
#include "mod_ext.h"
#include "proc_ext.h"
#include "pciv_ext.h"
#include "mkp_pciv.h"
#include "vpp_ext.h"
#include "pciv.h"


#define PCIV_STATE_STARTED   0
#define PCIV_STATE_STOPPING  1
#define PCIV_STATE_STOPPED   2

#define PCIV_MAX_SLOT_NUM 8

static HI_U32 s_u32PcivState = PCIV_STATE_STOPPED;
static atomic_t s_stPcivUserRef = ATOMIC_INIT(0);

static HI_S32            g_s32LocalId = -1;
static PCIV_BASEWINDOW_S g_stBaseWindow[PCIV_MAX_SLOT_NUM];

/* 0--slave , 1 -- host */
static int is_host = 0;
module_param(is_host, uint, S_IRUGO);


static DECLARE_MUTEX(g_PcivIoctlMutex);

#define PCIV_IOCTL_DWON() \
do{\
    HI_S32 s32Tmp;\
    s32Tmp = down_interruptible(&g_PcivIoctlMutex);\
    if(s32Tmp)\
    {\
        atomic_dec(&s_stPcivUserRef);\
        return s32Tmp;\
    }\
}while(0)

#define PCIV_IOCTL_UP() up(&g_PcivIoctlMutex)

static int pciv_open(struct inode * inode, struct file * file)
{
    return 0; 
}

static int pciv_close(struct inode *inode ,struct file *file)
{
    return 0;
}

static int pciv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	HI_S32 s32Ret      = -ENOIOCTLCMD;
    void __user * pArg = (void __user *)arg;
    
    if (PCIV_STATE_STARTED != s_u32PcivState)  /* 如果已经受到系统停止的通知,或者已经停止 */
    {
        return HI_ERR_PCIV_SYS_NOTREADY;
    }
    atomic_inc(&s_stPcivUserRef);

    switch(cmd)
    {
        case PCIV_CREATE_DEVICE_CTRL:
        {
            PCIV_DEVICE_S stDevice;

            PCIV_IOCTL_DWON();
            
			if(copy_from_user(&stDevice,pArg,sizeof(PCIV_DEVICE_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_Create(&stDevice);
			}
			if(HI_SUCCESS == s32Ret)
			{
			    memcpy(pArg, &stDevice, sizeof(PCIV_DEVICE_S));
			}

			PCIV_IOCTL_UP();
            break;
        }
        case PCIV_DESTROY_DEVICE_CTRL:
        {
            PCIV_DEVICE_S stDevice;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stDevice,pArg,sizeof(PCIV_DEVICE_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_Destroy(&stDevice);
			}
			
			PCIV_IOCTL_UP();
            break;
        }
        case PCIV_SETATTR_CTRL:
        {
			PCIV_DEVICE_ATTR_S stAttr;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stAttr, pArg, sizeof(PCIV_DEVICE_ATTR_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret = PCIV_SetAttr(&stAttr);
			}
			PCIV_IOCTL_UP();
			break;
        }
		case PCIV_GETATTR_CTRL:
		{
			PCIV_DEVICE_ATTR_S stAttr;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stAttr,pArg,sizeof(PCIV_DEVICE_ATTR_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret = PCIV_GetAttr(&stAttr);
				if(s32Ret == HI_SUCCESS)
				{
					copy_to_user(pArg,&stAttr,sizeof(PCIV_DEVICE_ATTR_S));
				}
			}

			PCIV_IOCTL_UP();
			break;
		}
		case PCIV_START_CTRL:
		{
            PCIV_DEVICE_S stDevice;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stDevice,pArg,sizeof(PCIV_DEVICE_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_Start(&stDevice);
			}
			PCIV_IOCTL_UP();
			break;
		}
        case PCIV_STOP_CTRL:
        {
            PCIV_DEVICE_S stDevice;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stDevice,pArg,sizeof(PCIV_DEVICE_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_Stop(&stDevice);
			}
	
            PCIV_IOCTL_UP();
			break;
        }        
        case PCIV_ENABLE_DEFLICKER_CTRL:
        {
            HI_BOOL bEnable;

            PCIV_IOCTL_DWON();

            if(copy_from_user(&bEnable,pArg,sizeof(HI_BOOL)))
			{
				s32Ret = -EFAULT; 
			}
            else
            {
                s32Ret =  PCIV_EnableDiflicker(bEnable);                
            }

			PCIV_IOCTL_UP();
			break;
        }
		case PCIV_MALLOC_CTRL:
		{
			PCIV_DEVICE_ATTR_S stAttr;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stAttr,pArg,sizeof(PCIV_DEVICE_ATTR_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret = PCIV_Malloc(&stAttr);
			}
			
			if(HI_SUCCESS == s32Ret)
			{
			    memcpy(pArg, &stAttr, sizeof(PCIV_DEVICE_ATTR_S));
			}

			PCIV_IOCTL_UP();

			break;
		}
		case PCIV_FREE_CTRL:
		{
			PCIV_DEVICE_S stDevice;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stDevice, pArg, sizeof(PCIV_DEVICE_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_Free(&stDevice);
			}

			PCIV_IOCTL_UP();
			break;
		}
		case PCIV_BIND_CTRL:
		{
			PCIV_BIND_S stBind;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stBind, pArg, sizeof(PCIV_BIND_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_Bind(&stBind);
			}
			PCIV_IOCTL_UP();
			break;
		}
		case PCIV_UNBIND_CTRL:
		{
			PCIV_BIND_S stBind;

            PCIV_IOCTL_DWON();

			if(copy_from_user(&stBind, pArg, sizeof(PCIV_BIND_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
				s32Ret =  PCIV_UnBind(&stBind);
			}
			PCIV_IOCTL_UP();
			break;
		}
		case PCIV_GETBASEWINDOW_CTRL:
		{
		    HI_S32 i;
		    PCIV_BASEWINDOW_S stBase;

            s32Ret = -EFAULT; 
			if(copy_from_user(&stBase, pArg, sizeof(PCIV_BASEWINDOW_S)))
			{
				break;
			}
            for(i=0; i<PCIV_MAX_SLOT_NUM; i++)
            {
                if(g_stBaseWindow[i].s32SlotIndex == stBase.s32SlotIndex)
                {
                    s32Ret = copy_to_user(pArg, &g_stBaseWindow[i], sizeof(PCIV_BASEWINDOW_S));
                    break;
                }
            }
		    break;
		}
		case PCIV_DMATASK_CTRL:
		{
            PCIV_DMA_TASK_S  stTask;
            PCIV_DMA_BLOCK_S stDmaBlk[PCIV_MAX_DMABLK];

			if(copy_from_user(&stTask, pArg, sizeof(PCIV_DMA_TASK_S)))
			{
				s32Ret = -EFAULT; 
			}
			else
			{
			    HI_U32 u32CpySize = sizeof(PCIV_DMA_BLOCK_S) * stTask.u32Count;
			    copy_from_user(&stDmaBlk[0], stTask.pBlock, u32CpySize);
			    stTask.pBlock = &stDmaBlk[0];
				s32Ret =  PCIV_DmaTask(&stTask);
			}
		    break;
		}
		case PCIV_GETLOCALID_CTRL:
		{
		    put_user(g_s32LocalId, (unsigned int __user *)pArg);
		    break;
		}
        default:
			s32Ret = -ENOIOCTLCMD;
        break;
    }
	atomic_dec(&s_stPcivUserRef);
    return s32Ret;
}

HI_CHAR* PRINT_PIXFORMAT(PIXEL_FORMAT_E enPixFormt)
{
    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixFormt)
        return "sp420";
    else if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixFormt)
        return "sp422";
    else if (PIXEL_FORMAT_YUV_PLANAR_420 == enPixFormt)
        return "p420";
    else if (PIXEL_FORMAT_YUV_PLANAR_422 == enPixFormt)
        return "p422";
    else if (PIXEL_FORMAT_UYVY_PACKAGE_422 == enPixFormt)
        return "uyvy422";
    else if (PIXEL_FORMAT_YUYV_PACKAGE_422 == enPixFormt)
        return "yuyv422";    
    else
        return NULL;
}

extern PCIV_CHANNEL_S g_stPcivChn[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM];
HI_S32 PcivProcRead(struct seq_file *s, HI_VOID *pArg)
{
    PCIV_CHANNEL_S *pstPcivChn;
    PCIV_PIC_ATTR_S *pstPicAttr;
    HI_S32 s32PciDev, s32Port;
    
    seq_printf(s, "\nVersion: ["MPP_VERSION"], Build Time["__DATE__", "__TIME__"]\n");
    seq_printf(s, "-----------------------------------------------------------\n"
            "infomation of PCIV attribution:\n");
    seq_printf(s, "%5s" "%8s" "%8s" "%8s" "%8s" "%8s" "%8s" "%8s" "%10s\n"
            ,"PciDev","PciPort","Width","Height","Stride","Field","PixFmt","BufCnt","PhyAddr0");    
    for (s32PciDev = 0; s32PciDev < PCIV_MAX_DEV_NUM; s32PciDev++)
    {
        for(s32Port = 0; s32Port < PCIV_MAX_PORT_NUM; s32Port++)
        {
            pstPcivChn = &g_stPcivChn[s32PciDev][s32Port];
            if (HI_FALSE == pstPcivChn->bStart)
                continue;
            pstPicAttr = &pstPcivChn->stPicAttr;
            seq_printf(s, "%5d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8s" "%8d" "%10x\n",
                s32PciDev, s32Port, 
                pstPicAttr->u32Width, 
                pstPicAttr->u32Height,
                pstPicAttr->u32Stride,
                pstPicAttr->u32Field,
                PRINT_PIXFORMAT(pstPicAttr->enPixelFormat),
                pstPcivChn->u32Count,
                pstPcivChn->au32PhyAddr[0]);
        }
    }
    seq_printf(s, "-----------------------------------------------------------\n"
            "infomation of PCIV Local bind object:\n");
    seq_printf(s, "%8s" "%8s" "%6s" "%6s" "%6s\n"
            ,"PciDev","PciPort","Type","Dev","Chn");
    for (s32PciDev = 0; s32PciDev < PCIV_MAX_DEV_NUM; s32PciDev++)
    {
        for(s32Port = 0; s32Port < PCIV_MAX_PORT_NUM; s32Port++)
        {
            PCIV_BIND_OBJ_S *pObj = NULL;
            
            pstPcivChn = &g_stPcivChn[s32PciDev][s32Port];
            if (HI_FALSE == pstPcivChn->bStart)
                continue;
            pObj = &(pstPcivChn->stBindObj);

            seq_printf(s, "%8d" "%8d", s32PciDev,s32Port);
            if(pObj->enType == PCIV_BIND_VI)
            {
                seq_printf(s, "%6s" "%6d" "%6d" "\n",
                    "VI", 
                    pObj->unAttachObj.viDevice.viDevId,
                    pObj->unAttachObj.viDevice.viChn);
            }
            else if(pObj->enType == PCIV_BIND_VO)
            {
                seq_printf(s, "%6s" "%6d" "%6d" "\n",
                    "VO", 0, pObj->unAttachObj.voDevice.voChn);
            }
            else if(pObj->enType == PCIV_BIND_VDEC)
            {
                seq_printf(s, "%6s" "%6d" "%6d" "\n",
                    "VDEC", 0, pObj->unAttachObj.vdecDevice.vdecChn);
            }
            else if(pObj->enType == PCIV_BIND_VENC)
            {
                seq_printf(s, "%6s" "%6d" "%6d" "\n",
                    "VENC", 0, pObj->unAttachObj.vencDevice.vencChn);
            }
            else if(pObj->enType == PCIV_BIND_VSTREAMREV)
            {
                seq_printf(s, "%6s" "%6d" "%6d" "\n",
                    "STREAMREV", -1, -1);
            }
            else if(pObj->enType == PCIV_BIND_VSTREAMSND)
            {
                seq_printf(s, "%6s" "%6d" "%6d" "\n",
                    "STREAMSND", -1, -1);
            }
            else
            {
                seq_printf(s, "%6s\n", "Unkown");
            }
        }
    }
        
    seq_printf(s, "-----------------------------------------------------------\n"
            "infomation of PCIV status:\n");
    seq_printf(s, "%8s" "%8s" "%8s" "%8s" "%8s" "%8s" "%9s" "%8s" "%9s\n"
            ,"PciDev","PciPort","Malloc","RecvCnt","SendCnt", "RespCnt", "RemtTarg", "RemtDev", "RemtPort");
    for (s32PciDev = 0; s32PciDev < PCIV_MAX_DEV_NUM; s32PciDev++)
    {
        for(s32Port = 0; s32Port < PCIV_MAX_PORT_NUM; s32Port++)
        {
            pstPcivChn = &g_stPcivChn[s32PciDev][s32Port];
            if (HI_FALSE == pstPcivChn->bStart)
                continue;
            seq_printf(s, "%8d" "%8d" "%8d" "%8d" "%8d" "%8d" "%9d" "%8d" "%9d\n",
                s32PciDev,s32Port, 
                pstPcivChn->bMalloc,
                pstPcivChn->u32RecvCnt,
                pstPcivChn->u32SendCnt,
                pstPcivChn->u32RespCnt,
                pstPcivChn->stRemoteObj.u32MsgTargetId,
                pstPcivChn->stRemoteObj.stTargetDev.s32PciDev,
                pstPcivChn->stRemoteObj.stTargetDev.s32Port);
        }
    }
    
    return 0;
}


static struct file_operations pciv_fops = 
{
    	.owner		= THIS_MODULE,
    	.open		= pciv_open,
	    .ioctl		= pciv_ioctl,
    	.release    = pciv_close,
};


static struct miscdevice pciv_dev = {
	MISC_DYNAMIC_MINOR,
	"pciv",
	&pciv_fops
};


HI_S32 PCIV_ExtInit(void *p)
{
	/*只要不是已停止状态就不需要初始化,直接返回成功*/
    if(s_u32PcivState != PCIV_STATE_STOPPED)
    {
        return HI_SUCCESS;
    }

    if(HI_SUCCESS != PCIV_Init())
	{
        HI_TRACE(HI_DBG_ERR,HI_ID_PCIV,"PvivInit failed\n");
    	return HI_FAILURE;
	}
	
    HI_TRACE(HI_INFO_LEVEL(255),HI_ID_PCIV,"PcivInit success\n");
    s_u32PcivState = PCIV_STATE_STARTED;

	return HI_SUCCESS;
}


HI_VOID PCIV_ExtExit(HI_VOID) 
{
	/*如果已经停止,则直接返回成功,否则调用exit函数*/
    if(s_u32PcivState == PCIV_STATE_STOPPED)
    {
        return ;
    }
	PCIV_Exit();
    s_u32PcivState = PCIV_STATE_STOPPED;
}

static HI_VOID PCIV_Notify(MOD_NOTICE_ID_E enNotice)  
{
    s_u32PcivState = PCIV_STATE_STOPPING;  /* 不再接受新的IOCT */
    /* 注意要唤醒所有的用户 */
    return;
}

static HI_VOID PCIV_QueryState(MOD_STATE_E *pstState)
{
    if (0 == atomic_read(&s_stPcivUserRef))  
    {
        *pstState = MOD_STATE_FREE;
    }
    else
    {
        *pstState = MOD_STATE_BUSY;
    }
    return;
}

static PCIV_EXPORT_FUNC_S s_stExportFuncs = 
{
    .pfnPcivSendPic  =  PCIV_ViuSendPic,
};

static UMAP_MODULE_S s_stPcivModule = 
{
    .pstOwner = THIS_MODULE,
    .enModId = HI_ID_PCIV,

    .pfnInit = PCIV_ExtInit,
    .pfnExit = PCIV_ExtExit,
    .pfnQueryState = PCIV_QueryState,
    .pfnNotify = PCIV_Notify,

    .pstExportFuncs = &s_stExportFuncs,
    .pData = HI_NULL,
};

HI_VOID  PCIV_SetBaseWindow(PCIV_BASEWINDOW_S *pBase)
{
    HI_S32 i;
    for(i=0; i<PCIV_MAX_SLOT_NUM; i++)
    {
        if(g_stBaseWindow[i].s32SlotIndex == -1)
        {
            memcpy(&g_stBaseWindow[i], pBase, sizeof(PCIV_BASEWINDOW_S));
            break;
        }
    }
}
EXPORT_SYMBOL(PCIV_SetBaseWindow);

HI_VOID  PCIV_SetLocalId(HI_S32 s32LocalId)
{
    g_s32LocalId = s32LocalId;
}
EXPORT_SYMBOL(PCIV_SetLocalId);


static int __init PCIV_ModInit(void)
{
    HI_S32 i;
    CMPI_PROC_ITEM_S *item;
	HI_S32 s32Ret = misc_register(&pciv_dev);

	if (s32Ret) 
	{
        printk("pciv register failed  error\n");
		return s32Ret;
	}

    item = CMPI_CreateProc(PROC_ENTRY_PCIV, NULL, NULL);
    if (! item)
        return -1;   
    item->read = PcivProcRead;
	
	if (CMPI_RegisterMod(&s_stPcivModule))
    {
        goto UNREGDEV;
    }
	
    memset(g_stBaseWindow, 0, sizeof(g_stBaseWindow));
    for(i=0; i<PCIV_MAX_SLOT_NUM; i++)
    {
        g_stBaseWindow[i].s32SlotIndex = -1;
    }
    return HI_SUCCESS;
	
UNREGDEV:
	misc_deregister(&pciv_dev);
    return HI_FAILURE;

}

static void __exit PCIV_ModExit(void)
{
	CMPI_RemoveProc(PROC_ENTRY_PCIV);
    CMPI_UnRegisterMod(HI_ID_PCIV);
	misc_deregister(&pciv_dev);
	
    return ;
}


module_init(PCIV_ModInit);
module_exit(PCIV_ModExit);

MODULE_AUTHOR("Hi3511 MPP GRP");
MODULE_LICENSE("Proprietary");
MODULE_VERSION(MPP_VERSION);


