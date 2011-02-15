#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include "hi_comm_vb.h"
#include "hi_comm_sys.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vdec.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vo.h"
#include "mpi_vo.h"


#define Printf(fmt...)  \
do{\
    printf("\n [%s]: %d ", __FUNCTION__, __LINE__);\
    printf(fmt);\
} while(0)


/*send h264 nalu to vdec pthread*/
void* proc_sendh264stream(void* p)
{
    #define MAX_READ_LEN 1024
    
    VDEC_STREAM_S stStream;
    FILE* file = NULL;
    HI_S32 s32ret;
    char buf[MAX_READ_LEN];
    char *filename;

    filename = (char*)p;
    /*open the stream file*/
    file = fopen(filename, "r");
    if (!file)
    {
        printf("open file %s err\n",filename);
        return NULL;
    }
    
    while(1)
    {
        s32ret = fread(buf,1,MAX_READ_LEN,file);
        if(s32ret <=0)
        {
            break;
        }

        stStream.pu8Addr = buf;
        stStream.u32Len = s32ret;
        stStream.u64PTS = 0;

        s32ret = HI_MPI_VDEC_SendStream(0, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {
            Printf("send to vdec 0x %x \n", s32ret);
            break;
        }
    }
    
    fclose(file);
    return NULL;
}

/*
 * description:
 *    There are two mode to displsy decoded picture in VO:
 *    (1) bind mode.
 *    (2) user-send mode.
 *
 *    To use bind mode, call HI_MPI_VDEC_BindOutput(). It is recommended.
 *    To use user-send mode, do as proc_recvPic() did. It is not recommended.
 *
 * remark: 
 *    How to send pictures to VO is totally left to user in user-send mode. 
 *    So it is more complicated, but also more flexible. User can do some 
 *    special control in user-send mode.
 */
void* proc_recvPic(void* p)
{
    HI_S32 s32ret;
    
    while(1)
    {
        VDEC_DATA_S stVdecData;
        s32ret = HI_MPI_VDEC_GetData(0, &stVdecData, HI_IO_BLOCK);
        if (HI_SUCCESS == s32ret)
        {
            if(stVdecData.stFrameInfo.bValid)
            {
                HI_MPI_VO_SendFrame(0, &stVdecData.stFrameInfo.stVideoFrameInfo);
                usleep(40000);                    
            }
            
            HI_MPI_VDEC_ReleaseData(0, &stVdecData);
        }
    }
    
    return NULL;
}


HI_S32 SampleSysInit(HI_VOID)
{
    HI_S32 s32Ret;
    MPP_SYS_CONF_S stSysConf = {0};
    VB_CONF_S stVbConf ={0};
    
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    
    stVbConf.u32MaxPoolCnt = 64;
    stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
    stVbConf.astCommPool[0].u32BlkCnt = 20; 
    stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
    stVbConf.astCommPool[1].u32BlkCnt = 40; 

    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VB_SetConf failed 0x%x!\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VB_Init();
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VB_Init failed 0x%x!\n",s32Ret);
        return HI_FAILURE;
    }

    stSysConf.u32AlignWidth = 64;

    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != s32Ret)
    {
        HI_MPI_VB_Exit();
        printf("conf : system config failed 0x%x!\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_SYS_Init();
    if(HI_SUCCESS != s32Ret)
    {
        HI_MPI_VB_Exit();
        printf("HI_MPI_SYS_Init err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


HI_VOID SampleSysExit(HI_VOID)
{   
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
}


/*
* init system
*/
static HI_S32 SampleVoInit(void)
{
    HI_S32 s32ret;
    VO_PUB_ATTR_S VoAttr;

    VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;  /*p制显示模式*/
    VoAttr.u32BgColor = 0x0 /*10*/;

    s32ret = HI_MPI_VO_SetPubAttr(&VoAttr);
    if (HI_SUCCESS != s32ret)
    {
        Printf("HI_MPI_VO_SetPubAttr errno: 0x%x!\n", s32ret);
        return s32ret;
    }

    s32ret = HI_MPI_VO_Enable();
    if (HI_SUCCESS != s32ret)
    {
        Printf("HI_MPI_VO_Enable errno: 0x%x!\n", s32ret);
        return s32ret;
    }
    
    return HI_SUCCESS;
}

/*
* Exit System
*/
static HI_VOID SampleVoExit(void)
{   
    HI_S32 s32Ret;
    s32Ret = HI_MPI_VO_Disable();
    if (HI_SUCCESS != s32Ret)
    {
        Printf("HI_MPI_VO_Disable errno: 0x%x!\n", s32Ret);
    }
}


HI_S32 SampleVoInitChn(VO_CHN VoChn, VO_CHN_ATTR_S *pstVoChnAttr)
{
    if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(VoChn, pstVoChnAttr))
    {
        Printf("set VO 0 Chn attribute failed !\n");
        return HI_FAILURE;
    }

    if(HI_SUCCESS!=HI_MPI_VO_EnableChn(VoChn))
    {
        Printf("enable vo channel failed !\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 SampleVoExitChn(VO_CHN VoChn)
{
    if(HI_SUCCESS!=HI_MPI_VO_DisableChn(VoChn))
    {
        Printf("diaable vo channel failed !\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


/*
* Create vdec chn
*/
static HI_S32 SampleCreateVdecchn(HI_S32 s32ChnID, PAYLOAD_TYPE_E enType, void* pstAttr)
{
    VDEC_CHN_ATTR_S     stAttr;
    HI_S32 s32ret;
    
    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    
    switch(enType)
    {
        case PT_H264:
        {
            VDEC_ATTR_H264_S stH264Attr;

            memcpy(&stH264Attr, pstAttr, sizeof(VDEC_ATTR_H264_S));
        
            stAttr.enType = PT_H264;
            stAttr.u32BufSize = (((stH264Attr.u32PicWidth) * (stH264Attr.u32PicHeight))<<1);
            stAttr.pValue = (void*)&stH264Attr;
        }
        break;
       
        case PT_JPEG:
        {
            VDEC_ATTR_JPEG_S stJpegAttr;

            memcpy(&stJpegAttr, pstAttr, sizeof(VDEC_ATTR_JPEG_S));
        
            stAttr.enType = PT_JPEG;
            stAttr.u32BufSize = ((stJpegAttr.u32PicWidth*stJpegAttr.u32PicHeight) * 1.5);
            stAttr.pValue = (void*)&stJpegAttr;
        }
        break;
        default:
        Printf("err type \n");
        return HI_FAILURE;    
    }
    
    s32ret = HI_MPI_VDEC_CreateChn(s32ChnID, &stAttr, NULL);
    if (HI_SUCCESS != s32ret)
    {
        Printf("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32ret);
        return s32ret;
    }
    
    s32ret = HI_MPI_VDEC_StartRecvStream(s32ChnID);
    if (HI_SUCCESS != s32ret)
    {
        Printf("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32ret);
        return s32ret;
    }
    
    return HI_SUCCESS;
}


/*强行停止解码并销毁视频编码*/
void SampleForceDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32Ret;
    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        Printf("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
    }
    
    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        Printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
    }
}

static HI_VOID H264D_SendEos(VDEC_CHN VdChn)
{
	VDEC_STREAM_S stStreamEos;
	HI_U32 au32EOS[2] = {0x01000000,0x0100000b};
    	
	stStreamEos.pu8Addr = (HI_U8 *)au32EOS;
	stStreamEos.u32Len  = sizeof(au32EOS); 
	stStreamEos.u64PTS  = 0;

	(void)HI_MPI_VDEC_SendStream(VdChn, &stStreamEos, HI_IO_BLOCK);
	
	return;
}

/*等待解码完成并销毁视频编码*/
void SampleWaitDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32ret;
    VDEC_CHN_STAT_S stStat;
    
    memset(&stStat,0,sizeof(VDEC_CHN_STAT_S));

    /* send EOS(End Of Stream) to ensure decoding the last frame. */
    H264D_SendEos(s32ChnID);
        
    s32ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if(s32ret != HI_SUCCESS)
    {
        Printf("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32ret);
        return;
    }
    
    while(1)
    {
        usleep(40000);
        s32ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
        if(s32ret != HI_SUCCESS)
        {
            Printf("HI_MPI_VDEC_Query failed errno 0x%x \n", s32ret);
            return;
        }
        
        if(stStat.u32LeftPics == 0 && stStat.u32LeftStreamBytes == 0 )
        {
            printf("had no stream and pic left\n");
            break;
        }
    }

    s32ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if(s32ret != HI_SUCCESS)
    {
        Printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32ret);
        return;
    }
}
void printhelp(void)
{
    printf("1--h264d to vo,vo bind h264d\n" );
    printf("2--h264d to vo,user get pic from vdec\n" );
    printf("3-one d1 h264 decoder to vo, stream and vo frame rate is 6\n" );        
    printf("4-one cif h264 decoder to vo, stream frame rate is full,vo frame rate is 12\n" );       
    printf("5-one cif h264 decoder to vo, stream frame rate is full,vo frame rate is 50)\n" );      
}

int main(int argc,char* argv[] )
{
    HI_S32 S32Index;
    
    if (argc != 2)
    {
        printf("usage :%s 1|2|3|4|5\n",argv[0]);
        printhelp();
        return HI_FAILURE;
    }
    
    S32Index = atoi(argv[1]);

    if(S32Index < 1 || S32Index > 5)
    {
        printhelp();
        return HI_FAILURE;
    }
    /*init system*/
    if ( HI_SUCCESS != SampleSysInit())
    {
        Printf("vdec_init failed \n");
        return HI_FAILURE;
    }
    
    /*init vo*/
    if ( HI_SUCCESS != SampleVoInit())
    {
        Printf("vdec_init failed \n");
        SampleSysExit();
    }
    
    switch(S32Index)
    {
        case 1: /*one h264 decoder to vo, d1*/
        {
            pthread_t   pidSendStream;
            VDEC_ATTR_H264_S stH264Attr;
            VO_CHN_ATTR_S   stVoChnAttr;
            HI_CHAR filename[100] = "sample_d1.h264";
            
            stVoChnAttr.u32Priority = 0;
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 0;
            stVoChnAttr.stRect.u32Width = 720;
            stVoChnAttr.stRect.u32Height = 576;
            stVoChnAttr.bZoomEnable = HI_TRUE;
            
            stH264Attr.u32Priority = 0;
            stH264Attr.u32PicHeight = 576;
            stH264Attr.u32PicWidth = 720;
            stH264Attr.u32RefFrameNum = 2;
            stH264Attr.enMode = H264D_MODE_STREAM;

            /*init vo channel*/
            if(HI_SUCCESS != SampleVoInitChn(0, &stVoChnAttr) )
            {
                Printf("init vo failed\n");
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            /*create vdec chn and vo chn*/
            if (HI_SUCCESS != SampleCreateVdecchn(0, PT_H264, &stH264Attr) )
            {
                Printf("create_vdecchn failed\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            /*bind vdec to vo*/
            if(HI_SUCCESS != HI_MPI_VDEC_BindOutput(0, 0))
            {
                Printf("bind vdec to vo failed\n");
                SampleForceDestroyVdecChn(0);
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }

            pthread_create(&pidSendStream, NULL, proc_sendh264stream, filename);
            pthread_join(pidSendStream, 0);
        }
        break;
        case 2: /*get user data from h264 stream*/
            /*to do*/
        {
            pthread_t   pidSendStream;
            pthread_t   pidRecvPic;
            
            VDEC_ATTR_H264_S stH264Attr;
            VO_CHN_ATTR_S   stVoChnAttr;
            HI_CHAR filename[100] = "sample_d1.h264";
            
            stVoChnAttr.u32Priority = 0;
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 0;
            stVoChnAttr.stRect.u32Width = 720;
            stVoChnAttr.stRect.u32Height = 576;
            stVoChnAttr.bZoomEnable = HI_TRUE;
            
            stH264Attr.u32Priority = 0;
            stH264Attr.u32PicHeight = 576;
            stH264Attr.u32PicWidth = 720;
            stH264Attr.u32RefFrameNum = 2;
            stH264Attr.enMode = H264D_MODE_STREAM;

            /*init vo channel*/
            if(HI_SUCCESS != SampleVoInitChn(0, &stVoChnAttr) )
            {
                Printf("init vo failed\n");
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            /*create vdec chn and vo chn*/
            if (HI_SUCCESS != SampleCreateVdecchn(0, PT_H264, &stH264Attr) )
            {
                Printf("create_vdecchn failed\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            
            pthread_create(&pidSendStream, NULL, proc_sendh264stream, filename);
            pthread_create(&pidRecvPic, NULL, proc_recvPic, NULL);
            pthread_join(pidSendStream, 0);
            break;
        }   
        case 3: /*one d1 h264 decoder to vo, stream and vo frame rate is 6*/
        {
            pthread_t   pidSendStream;
            VDEC_ATTR_H264_S stH264Attr;
            VO_CHN_ATTR_S   stVoChnAttr;
            HI_CHAR filename[100] = "sample_d1_6.h264";
            
            stVoChnAttr.u32Priority = 0;
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 0;
            stVoChnAttr.stRect.u32Width = 720;
            stVoChnAttr.stRect.u32Height = 576;
            stVoChnAttr.bZoomEnable = HI_TRUE;
            
            stH264Attr.u32Priority = 0;
            stH264Attr.u32PicHeight = 576;
            stH264Attr.u32PicWidth = 720;
            stH264Attr.u32RefFrameNum = 2;
            stH264Attr.enMode = H264D_MODE_STREAM;

            /*init vo channel*/
            if(HI_SUCCESS != SampleVoInitChn(0, &stVoChnAttr) )
            {
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }

            if(HI_SUCCESS != HI_MPI_VO_SetFrameRate(0, 6) )
            {
                Printf("set vo framerate ok\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }           
            
            /*create vdec chn and vo chn*/
            if (HI_SUCCESS != SampleCreateVdecchn(0, PT_H264, &stH264Attr) )
            {
                Printf("create_vdecchn failed\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            /*bind vdec to vo*/
            if(HI_SUCCESS != HI_MPI_VDEC_BindOutput(0, 0))
            {
                Printf("bind vdec to vo failed\n");
                SampleForceDestroyVdecChn(0);
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            
            pthread_create(&pidSendStream, NULL, proc_sendh264stream, filename);
            pthread_join(pidSendStream, 0);
            break;          
        }
        case 4: /*one cif h264 decoder to vo, stream frame rate is full,vo frame rate is 12*/
        {
            pthread_t   pidSendStream;
            VDEC_ATTR_H264_S stH264Attr;
            VO_CHN_ATTR_S   stVoChnAttr;
            HI_CHAR filename[100] = "sample_cif.h264";
            
            stVoChnAttr.u32Priority = 0;
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 0;
            stVoChnAttr.stRect.u32Width = 720;
            stVoChnAttr.stRect.u32Height = 576;
            stVoChnAttr.bZoomEnable = HI_TRUE;
            
            stH264Attr.u32Priority = 0;
            stH264Attr.u32PicHeight = 576;
            stH264Attr.u32PicWidth = 720;
            stH264Attr.u32RefFrameNum = 2;
            stH264Attr.enMode = H264D_MODE_STREAM;

            /*init vo channel*/
            if(HI_SUCCESS != SampleVoInitChn(0, &stVoChnAttr) )
            {
                Printf("init vo failed\n");
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }

            if(HI_SUCCESS != HI_MPI_VO_SetFrameRate(0, 12) )
            {
                Printf("set vo framerate ok\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }           
            
            /*create vdec chn and vo chn*/
            if (HI_SUCCESS != SampleCreateVdecchn(0, PT_H264, &stH264Attr) )
            {
                Printf("create_vdecchn failed\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;          }
            /*bind vdec to vo*/
            if(HI_SUCCESS != HI_MPI_VDEC_BindOutput(0, 0))
            {
                Printf("bind vdec to vo failed\n");
                SampleForceDestroyVdecChn(0);
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            
            pthread_create(&pidSendStream, NULL, proc_sendh264stream, filename);
            pthread_join(pidSendStream, 0);
            break;          
        }   
        case 5: /*one cif h264 decoder to vo, stream frame rate is full,vo frame rate is 50*/
        {
            pthread_t   pidSendStream;
            VDEC_ATTR_H264_S stH264Attr;
            VO_CHN_ATTR_S   stVoChnAttr;
            HI_CHAR filename[100] = "sample_cif.h264";
            
            stVoChnAttr.u32Priority = 0;
            stVoChnAttr.stRect.s32X = 0;
            stVoChnAttr.stRect.s32Y = 0;
            stVoChnAttr.stRect.u32Width = 720;
            stVoChnAttr.stRect.u32Height = 576;
            stVoChnAttr.bZoomEnable = HI_TRUE;
            
            stH264Attr.u32Priority = 0;
            stH264Attr.u32PicHeight = 576;
            stH264Attr.u32PicWidth = 720;
            stH264Attr.u32RefFrameNum = 2;
            stH264Attr.enMode = H264D_MODE_STREAM;

            /*init vo channel*/
            if(HI_SUCCESS != SampleVoInitChn(0, &stVoChnAttr) )
            {
                Printf("init vo failed\n");
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }

            if(HI_SUCCESS != HI_MPI_VO_SetFrameRate(0, 50) )
            {
                Printf("set vo framerate ok\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }           
            
            /*create vdec chn and vo chn*/
            if (HI_SUCCESS != SampleCreateVdecchn(0, PT_H264, &stH264Attr) )
            {
                Printf("create_vdecchn failed\n");
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            /*bind vdec to vo*/
            if(HI_SUCCESS != HI_MPI_VDEC_BindOutput(0, 0))
            {
                Printf("bind vdec to vo failed\n");
                SampleForceDestroyVdecChn(0);
                SampleVoExitChn(0);
                SampleVoExit();
                SampleSysExit();
                return HI_FAILURE;
            }
            
            pthread_create(&pidSendStream, NULL, proc_sendh264stream, filename);
            pthread_join(pidSendStream, 0);
            
            break;          
        }       
        default:
            Printf("err arg \n");
        break;
    }

    /*等待销毁解码通道*/
    SampleWaitDestroyVdecChn(0);
    
    SampleVoExitChn(0);
    SampleVoExit();
    SampleSysExit();
    return HI_SUCCESS;
}
