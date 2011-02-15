#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <asm/page.h>
#include <assert.h>
#include "hi_comm_sys.h"
#include "tde_interface.h"
#include "hifb.h"
#include "loadbmp.h"
#include "mpi_sys.h"


#define FB_NAME_1   "/dev/fb/0"
#define SCREEN_WIDTH    720
#define SCREEN_HEIGHT   576
#define PIX_LEN   2


static HI_S32 g_s32Fd = -1;

static HI_U32 g_u32SemStart = 0;
static HI_U32 g_u32SemSize = 0;


char* Sample_hifb_mmap(int fd, int offset, size_t size)
{
    char * pszVirt = (char*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);	
    memset(pszVirt, 0, size);
    return pszVirt;
}

int Sample_hifb_munmap(void *start, size_t size)
{
    return  munmap(start, size);
}
               
HI_S32 Sample_hifb_open(HI_VOID)
{
	HIFB_ALPHA_S stAlpha;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

	struct fb_bitfield st_r32 = {10, 5, 0};
	struct fb_bitfield st_g32 = {5, 5, 0};
	struct fb_bitfield st_b32 = {0, 5, 0}; 	
	struct fb_bitfield st_a32 = {15, 1, 0};

	g_s32Fd = open(FB_NAME_1, O_RDWR);
    if(g_s32Fd < 0)
    {
       printf("open FB err!\n");
	   return HI_FAILURE;
    }

	stAlpha.bAlphaEnable = HI_TRUE;
    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.u8Alpha0 = 0xf0;
    stAlpha.u8Alpha1 = 0xf0;
	
    if (ioctl(g_s32Fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) != HI_SUCCESS)
    {
        printf("Couldn't set alpha\n");
        return  -1;
    }
    
    if (ioctl(g_s32Fd, FBIOGET_VSCREENINFO, &vinfo) != HI_SUCCESS)
    {
   	   printf("Get vscreen info err!\n");
	   return HI_FAILURE;
    }
      
    vinfo.xres = SCREEN_WIDTH;
    vinfo.yres = SCREEN_HEIGHT;
	vinfo.xres_virtual = SCREEN_WIDTH;
	vinfo.yres_virtual = SCREEN_HEIGHT*2;
    vinfo.xoffset = 0;
	vinfo.yoffset = 0;
    vinfo.red   = st_r32;
    vinfo.green = st_g32;
    vinfo.blue  = st_b32;
    vinfo.transp = st_a32;
    vinfo.bits_per_pixel = 16;
    
    vinfo.activate = FB_ACTIVATE_NOW;
    
	if (ioctl(g_s32Fd, FBIOPUT_VSCREENINFO, &vinfo) != HI_SUCCESS)
    {
   	   printf("Put vscreen info err!\n");
	   return HI_FAILURE;
    }
	
	if (ioctl(g_s32Fd, FBIOGET_FSCREENINFO, &finfo) != HI_SUCCESS)
    {
   	   printf("Get fscreen info err!\n");
	   return HI_FAILURE;
    }
 
    g_u32SemStart = finfo.smem_start; 
    g_u32SemSize = finfo.smem_len; 
  
    return HI_SUCCESS;
}

HI_VOID Sample_hifb_close()
{
    close(g_s32Fd);
    g_s32Fd = -1;

    return;
}

HI_S32 SampleLoadBmp(const char *filename, HI_U8 *pu8Virt)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
		printf("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
    
    CreateSurfaceByBitMap(filename,&Surface,pu8Virt);	 
	
    return HI_SUCCESS;
}



HI_S32 main(int argc, char *argv[])
{	

	HI_S32 s32Ret;
	HI_U32 u32Bpp;
	HI_U16 osdHeight;
    HI_U16 osdwidth;
	TDE_COLOR_FMT_E enFmt;
	
	HI_CHAR ch;
	HI_CHAR *pFilename = NULL;
	HI_CHAR *pszVirt = NULL;

    TDE_DEFLICKER_COEF_S stDefCoef ;    
    TDE_HANDLE s32Handle;
    TDE_SURFACE_S  stDst, stSrc;  
	MPP_SYS_CONF_S struSysConf;

 	HI_CHAR ping[1024] =    "\n/**************************************/\n"
							"n  --> NO TDE\n"
							"t  --> TWO TDE\n"
							"q  --> quit\n"
							"please enter order:\n";
	
	pFilename = "AVQ_SRC_TDE.bmp";

	enFmt     = TDE_COLOR_FMT_RGB1555;/*picture pix format*/
	osdwidth  = SCREEN_WIDTH;         /*picture width*/  
    osdHeight = SCREEN_HEIGHT;        /*picture height*/ 
	u32Bpp    = PIX_LEN;              /*pix length*/

    /* mpi sys init */
    HI_MPI_SYS_Exit();
	
    struSysConf.u32AlignWidth = 64;
	
	s32Ret = HI_MPI_SYS_SetConf(&struSysConf);
	if (HI_SUCCESS != s32Ret)
	{
        printf("HI_MPI_SYS_SetConf err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_SYS_Init();
	if (HI_SUCCESS != s32Ret)
	{
        printf("HI_MPI_SYS_Init err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

    Sample_hifb_open();

	/*get figure layer start virtual address*/
    pszVirt = Sample_hifb_mmap(g_s32Fd, 0, g_u32SemSize);

    stSrc.enColorFmt = enFmt;                      
    stSrc.pu8PhyAddr = (HI_U8*)g_u32SemStart;   
    stSrc.u16Height = osdHeight;                   
    stSrc.u16Width  = osdwidth;                    
    stSrc.u16Stride = SCREEN_WIDTH* u32Bpp;          
    
	stDst.enColorFmt = enFmt;                       
    stDst.pu8PhyAddr = (HI_U8*)g_u32SemStart + SCREEN_WIDTH* u32Bpp * SCREEN_HEIGHT;   
    stDst.u16Height = osdHeight;                    
    stDst.u16Width  = osdwidth;                     
    stDst.u16Stride = SCREEN_WIDTH* u32Bpp; 

	/*config deflicker coefficient*/
	memset(&stDefCoef,0,sizeof(TDE_DEFLICKER_COEF_S));
	
    stDefCoef.pu8HDfCoef = (HI_U8 *)malloc(8);
	if(NULL == stDefCoef.pu8HDfCoef)
	{
		printf("malloc deflicker coef faild!\n");
		return HI_FAILURE;
	}
	
    stDefCoef.pu8VDfCoef = (HI_U8 *)malloc(8);
	if(NULL == stDefCoef.pu8VDfCoef)
	{
		printf("malloc deflicker coef faild!\n");
		return HI_FAILURE;
	}
	
    stDefCoef.u32HDfLevel = 2;
    stDefCoef.u32VDfLevel = 2;              
    stDefCoef.pu8HDfCoef[0] = 0x80;
    stDefCoef.pu8VDfCoef[0] = 0x80;    
    
    s32Ret = HI_API_TDE_Open();
	
	printf("%s",ping);
	while((ch = getchar())!= 'q')
	{
		
		if('\n' == ch)
		{
			continue;
		}
		
		switch(ch)
		{
			case 'n':
			{
				if(SampleLoadBmp(pFilename, pszVirt) != HI_SUCCESS)/*load bitmap*/
	            {
	                printf("load bmp err!\n");

					if(NULL != stDefCoef.pu8HDfCoef)
					{
						free(stDefCoef.pu8HDfCoef);
						stDefCoef.pu8HDfCoef = NULL;
					}

					if(NULL != stDefCoef.pu8VDfCoef)
					{
						free(stDefCoef.pu8VDfCoef);
						stDefCoef.pu8VDfCoef = NULL;
					}
					
	   				return HI_FAILURE;
	            }

				break;
			}
			
			case 't':
			{
				if(SampleLoadBmp(pFilename, pszVirt) != HI_SUCCESS)
				{
					printf("load bmp err!\n");
					
					if(NULL != stDefCoef.pu8HDfCoef)
					{
						free(stDefCoef.pu8HDfCoef);
						stDefCoef.pu8HDfCoef = NULL;
					}

					if(NULL != stDefCoef.pu8VDfCoef)
					{
						free(stDefCoef.pu8VDfCoef);
						stDefCoef.pu8VDfCoef = NULL;
					}
					
	   				return HI_FAILURE;
				}   
			        
				s32Handle = HI_API_TDE_BeginJob();
				
				s32Ret = HI_API_TDE_Deflicker(s32Handle,&stSrc, &stDst, stDefCoef);
				if(s32Ret != HI_SUCCESS)
				{
					printf("HI_API_TDE_Deflicker err!\n");
					
					if(NULL != stDefCoef.pu8HDfCoef)
					{
						free(stDefCoef.pu8HDfCoef);
						stDefCoef.pu8HDfCoef = NULL;
					}

					if(NULL != stDefCoef.pu8VDfCoef)
					{
						free(stDefCoef.pu8VDfCoef);
						stDefCoef.pu8VDfCoef = NULL;
					}
					
	   				return HI_FAILURE;
				}
				
				s32Ret = HI_API_TDE_Deflicker(s32Handle,&stDst, &stSrc, stDefCoef);
				if(s32Ret != HI_SUCCESS)
				{
					printf("HI_API_TDE_Deflicker err!\n");
					
					if(NULL != stDefCoef.pu8HDfCoef)
					{
						free(stDefCoef.pu8HDfCoef);
						stDefCoef.pu8HDfCoef = NULL;
					}

					if(NULL != stDefCoef.pu8VDfCoef)
					{
						free(stDefCoef.pu8VDfCoef);
						stDefCoef.pu8VDfCoef = NULL;
					}
					
	   				return HI_FAILURE;
				}
				
				
				s32Ret = HI_API_TDE_EndJob(s32Handle, HI_TRUE, 100);
				if(s32Ret != HI_SUCCESS)
				{
					printf("HI_API_TDE_EndJob err!\n");
					
					if(NULL != stDefCoef.pu8HDfCoef)
					{
						free(stDefCoef.pu8HDfCoef);
						stDefCoef.pu8HDfCoef = NULL;
					}

					if(NULL != stDefCoef.pu8VDfCoef)
					{
						free(stDefCoef.pu8VDfCoef);
						stDefCoef.pu8VDfCoef = NULL;
					}
					
	   				return HI_FAILURE;
				}
				
				break;
			}
			
			default:
				printf("no order@@@@@!\n");
		}
		
		printf("%s",ping);
	}

    Sample_hifb_munmap(pszVirt, g_u32SemSize); 
	
    HI_API_TDE_Close();
	
    Sample_hifb_close();
	
    if(NULL != stDefCoef.pu8HDfCoef)
	{
		free(stDefCoef.pu8HDfCoef);
		stDefCoef.pu8HDfCoef = NULL;
	}

	if(NULL != stDefCoef.pu8VDfCoef)
	{
		free(stDefCoef.pu8VDfCoef);
		stDefCoef.pu8VDfCoef = NULL;
	}

	/* mpi sys init */
    HI_MPI_SYS_Exit();
	
    return HI_SUCCESS;

}




