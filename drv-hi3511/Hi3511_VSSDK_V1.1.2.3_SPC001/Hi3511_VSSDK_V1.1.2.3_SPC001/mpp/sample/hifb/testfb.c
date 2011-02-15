#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <asm/page.h>        /* For definition of PAGE_SIZE */
#include <assert.h>

#include "hifb.h"

static struct fb_var_screeninfo ghifb_st_def_vinfo =
{
	720,
	576,
	720,
	576,
	0,
	0, //yoffset
	16,
	0,
    {10, 5, 0},
    {5, 5, 0},
    {0, 5, 0}, 	
	{15, 1, 0},
	0,
	FB_ACTIVATE_FORCE,
	0,
	0,
	0,
	-1, //pixclock
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
};


static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
    fprintf(stderr, "Printing vinfo:\n");
    fprintf(stderr, "txres: %d\n", vinfo->xres);
    fprintf(stderr, "tyres: %d\n", vinfo->yres);
    fprintf(stderr, "txres_virtual: %d\n", vinfo->xres_virtual);
    fprintf(stderr, "tyres_virtual: %d\n", vinfo->yres_virtual);
    fprintf(stderr, "txoffset: %d\n", vinfo->xoffset);
    fprintf(stderr, "tyoffset: %d\n", vinfo->yoffset);
    fprintf(stderr, "tbits_per_pixel: %d\n", vinfo->bits_per_pixel);
    fprintf(stderr, "tgrayscale: %d\n", vinfo->grayscale);
    fprintf(stderr, "tnonstd: %d\n", vinfo->nonstd);
    fprintf(stderr, "tactivate: %d\n", vinfo->activate);
    fprintf(stderr, "theight: %d\n", vinfo->height);
    fprintf(stderr, "twidth: %d\n", vinfo->width);
    fprintf(stderr, "taccel_flags: %d\n", vinfo->accel_flags);
    fprintf(stderr, "tpixclock: %d\n", vinfo->pixclock);
    fprintf(stderr, "tleft_margin: %d\n", vinfo->left_margin);
    fprintf(stderr, "tright_margin: %d\n", vinfo->right_margin);
    fprintf(stderr, "tupper_margin: %d\n", vinfo->upper_margin);
    fprintf(stderr, "tlower_margin: %d\n", vinfo->lower_margin);
    fprintf(stderr, "thsync_len: %d\n", vinfo->hsync_len);
    fprintf(stderr, "tvsync_len: %d\n", vinfo->vsync_len);
    fprintf(stderr, "tsync: %d\n", vinfo->sync);
    fprintf(stderr, "tvmode: %d\n", vinfo->vmode);
    fprintf(stderr, "tred: %d/%d\n", vinfo->red.length, vinfo->red.offset);
    fprintf(stderr, "tgreen: %d/%d\n", vinfo->green.length, vinfo->green.offset);
    fprintf(stderr, "tblue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
    fprintf(stderr, "talpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
}

static void print_finfo(struct fb_fix_screeninfo *finfo)
{
    fprintf(stderr, "Printing finfo:\n");
    fprintf(stderr, "tsmem_start = %p\n", (char *)finfo->smem_start);
    fprintf(stderr, "tsmem_len = %d\n", finfo->smem_len);
    fprintf(stderr, "ttype = %d\n", finfo->type);
    fprintf(stderr, "ttype_aux = %d\n", finfo->type_aux);
    fprintf(stderr, "tvisual = %d\n", finfo->visual);
    fprintf(stderr, "txpanstep = %d\n", finfo->xpanstep);
    fprintf(stderr, "typanstep = %d\n", finfo->ypanstep);
    fprintf(stderr, "tywrapstep = %d\n", finfo->ywrapstep);
    fprintf(stderr, "tline_length = %d\n", finfo->line_length);
    fprintf(stderr, "tmmio_start = %p\n", (char *)finfo->mmio_start);
    fprintf(stderr, "tmmio_len = %d\n", finfo->mmio_len);
    fprintf(stderr, "taccel = %d\n", finfo->accel);
}

void* mapped_mem;
void* mapped_io;
unsigned long mapped_offset;
unsigned long mapped_memlen;
unsigned long mapped_iolen;

#define BLUE   0x001f
#define RED    0x7C00
#define GREEN  0x03e0

void DrawBox(int x, int y, int w, int h, unsigned long stride, 
         char* pmapped_mem, unsigned short color)
{
    int i, j;
    unsigned short* pvideo = (unsigned short*)pmapped_mem;

    for (i=y; i<y+h; i++)
    {
        pvideo = (unsigned short*)(pmapped_mem + i*stride);
        for (j=x; j<x+w; j++)
        {
            *(pvideo+j) = color;
        }
    }
}


int main(int argc, char* argv[])
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    HIFB_ALPHA_S stAlpha;
    int console_fd;
    int y;
    int ret = 0;
    const char* file = "/dev/fb/0";

    /* open fb device */
    if (argc >= 2)
    {
       if (*argv[1] == '1')
           file = "/dev/fb/1";
    }

    console_fd = open(file, O_RDWR, 0);
    if ( console_fd < 0 )
    {
        fprintf (stderr, "Unable to open %s\n", file);
        return (-1);
    }

    /* set color format ARGB1555, screen size: 720*576 */
    if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &ghifb_st_def_vinfo) < 0)
    {
        fprintf (stderr, "Unable to set variable screeninfo!\n");
        ret = -1;
        goto CLOSEFD;
    }

    /* Get the type of video hardware */
    if (ioctl(console_fd, FBIOGET_FSCREENINFO, &finfo) < 0 ) 
    {
        fprintf (stderr, "Couldn't get console hardware info\n");
        ret = -1;
        goto CLOSEFD;
    }

    print_finfo(&finfo);

    mapped_memlen = finfo.smem_len + mapped_offset;
    mapped_mem = mmap(NULL, mapped_memlen,
                      PROT_READ|PROT_WRITE, MAP_SHARED, console_fd, 0);
    if ( mapped_mem == (char *)-1 ) 
    {
        fprintf (stderr, "Unable to memory map the video hardware\n");
        mapped_mem = NULL;
        ret = -1;
        goto CLOSEFD;
    }

    /* Determine the current screen depth */
    if (ioctl(console_fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ) 
    {
        fprintf (stderr, "Couldn't get console pixel format\n");
        ret = -1;
        goto UNMAP;
    } 

    print_vinfo(&vinfo);

    /* set alpha */
    stAlpha.bAlphaEnable = HI_TRUE;
    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.u8Alpha0 = 0xff;
    stAlpha.u8Alpha1 = 0xff;
    if (ioctl(console_fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
    {
        fprintf (stderr, "Couldn't set alpha\n");
        ret = -1;
        goto UNMAP;
    }
    
    memset(mapped_mem, 0x00, finfo.smem_len);

    printf("fill box\n");

    y = 0;
    DrawBox(100, y, 100, 100, finfo.line_length, mapped_mem, BLUE);
    y +=110;
    DrawBox(100, y, 100, 100, finfo.line_length, mapped_mem, RED);
    y +=110;
    DrawBox(100, y, 100, 100, finfo.line_length, mapped_mem, GREEN);
    y +=110;
    DrawBox(100, y, 100, 100, finfo.line_length, mapped_mem, 0x0f0f);
    y +=110;
    DrawBox(100, y, 100, 100, finfo.line_length, mapped_mem, -1);
    sleep(10);

UNMAP:  
    munmap(mapped_mem, mapped_memlen);
CLOSEFD:
    close(console_fd);
    return ret;
}

