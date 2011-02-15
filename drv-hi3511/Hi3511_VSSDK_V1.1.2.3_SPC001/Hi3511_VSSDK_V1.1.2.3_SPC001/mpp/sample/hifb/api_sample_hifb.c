#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "hifb.h"

#define IMAGE_WIDTH     640
#define IMAGE_HEIGHT    352
#define IMAGE_SIZE      (640*352*2)
#define IMAGE_NUM       14
#define IMAGE_PATH		"./res/%d.bits"

static struct fb_bitfield g_r16 = {10, 5, 0};
static struct fb_bitfield g_g16 = {5, 5, 0};
static struct fb_bitfield g_b16 = {0, 5, 0}; 	
static struct fb_bitfield g_a16 = {15, 1, 0};

int main()
{
    int fd;
    int i;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    unsigned char *pShowScreen;
    unsigned char *pHideScreen;
	HIFB_ALPHA_S stAlpha;
    HIFB_POINT_S stPoint = {40, 112};
    FILE *fp;
    char image_name[128];

    /* 1. open framebuffer device overlay 0 */
    fd = open("/dev/fb/0", O_RDWR);
    if(fd < 0)
    {
        printf("open fb0 failed!\n");
        return -1;
    }

    /* 2. set the screen original position */
    if (ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        printf("set screen original show position failed!\n");
        close(fd);
        return -1;
    }

	/* 3.set alpha */
	stAlpha.bAlphaEnable = HI_TRUE;
	stAlpha.bAlphaChannel = HI_FALSE;
	stAlpha.u8Alpha0 = 0xff;
	stAlpha.u8Alpha1 = 0x8f;
	if (ioctl(fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
	{
	    printf("Set alpha failed!\n");
        close(fd);
        return -1;
	}

    /* 4. get the variable screen info */
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
   	    printf("Get variable screen info failed!\n");
        close(fd);
        return -1;
    }

    /* 5. modify the variable screen info
          the screen size: IMAGE_WIDTH*IMAGE_HEIGHT 
          the virtual screen size: IMAGE_WIDTH*(IMAGE_HEIGHT*2) 
          the pixel format: ARGB21555
    */
    var.xres = var.xres_virtual = IMAGE_WIDTH;
    var.yres = IMAGE_HEIGHT;
    var.yres_virtual = IMAGE_HEIGHT*2;
    
    var.transp= g_a16;
    var.red = g_r16;
    var.green = g_g16;
    var.blue = g_b16;
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_FORCE;  
    /* 6. set the variable screeninfo */
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &var) < 0)
    {
   	    printf("Put variable screen info failed!\n");
        close(fd);
        return -1;
    }
    
    /* 7. get the fix screen info */
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
   	    printf("Get fix screen info failed!\n");
        close(fd);
        return -1;
    }

    /* 8. map the physical video memory for user use */
    pShowScreen = mmap(NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
   	    printf("mmap framebuffer failed!\n");
        close(fd);
        return -1;
    }
    
    pHideScreen = pShowScreen + IMAGE_SIZE;
    memset(pShowScreen, 0, IMAGE_SIZE);

    /* 9. load the bitmaps from file to hide screen and set pan display the hide screen */
    for(i = 0; i < IMAGE_NUM; i++)
    {
        sprintf(image_name, IMAGE_PATH, i);
        fp = fopen(image_name, "rb");
        if(NULL == fp)
        {
            printf("Load %s failed!\n", image_name);
            munmap(pShowScreen, fix.smem_len);
            close(fd);
            return -1;
        }
    
        fread(pHideScreen, 1, IMAGE_SIZE, fp);
        fclose(fp);
        usleep(1000);
        if(i%2)
        {
            var.yoffset = 0;
            pHideScreen = pShowScreen + IMAGE_SIZE;
        }
        else
        {
            var.yoffset = IMAGE_HEIGHT;
            pHideScreen = pShowScreen;
        }
        
        if (ioctl(fd, FBIOPAN_DISPLAY, &var) < 0)
        {
            printf("FBIOPAN_DISPLAY failed!\n");
            munmap(pShowScreen, fix.smem_len);
            close(fd);
            return -1;
        }
    }

    printf("Enter to quit!\n");    
    getchar();

    /* 10.unmap the physical memory */
    munmap(pShowScreen, fix.smem_len);
    
    /* 11. close the framebuffer device */
    close(fd);

    return 0;
}

