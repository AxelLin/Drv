/* media-mem.c
*
* Copyright (c) 2006 Hisilicon Co., Ltd. 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/

#include <linux/config.h>
#include <linux/kernel.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include <linux/string.h>
#include <linux/list.h>

#include <linux/time.h>

#include <media-mem.h>


#define MMZ_DBG_LEVEL 0x0
#define mmz_trace(level, s, params...) do{ if(level & MMZ_DBG_LEVEL)\
		printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
		}while(0)

#define mmz_trace_func() mmz_trace(0x02,"%s", __FILE__)



#define MMZ_GRAIN PAGE_SIZE
#define mmz_bitmap_size(p) (mmz_align2(mmz_length2grain((p)->nbytes),8)/8)

#define mmz_get_bit(p,n) (((p)->bitmap[(n)/8]>>((n)&0x7))&0x1)
#define mmz_set_bit(p,n) (p)->bitmap[(n)/8] |= 1<<((n)&0x7)
#define mmz_clr_bit(p,n) (p)->bitmap[(n)/8] &= ~(1<<((n)&0x7))

#define mmz_pos2phy_addr(p,n) ((p)->phys_start+(n)*MMZ_GRAIN) 
#define mmz_phy_addr2pos(p,a) (((a)-(p)->phys_start)/MMZ_GRAIN) 

#define mmz_align2(x,g) ((((x)+(g)-1)/(g))*(g))
#define mmz_grain_align(x) mmz_align2(x,MMZ_GRAIN)
#define mmz_length2grain(len) (mmz_grain_align(len)/MMZ_GRAIN)


#define begin_list_for_each_mmz(p,gfp,mmz_name) list_for_each_entry(p,&mmz_list, list) {\
		if(gfp==0 ? 0:(p)->gfp!=(gfp))\
			continue;\
		if(mmz_name!=NULL) { \
			if(*mmz_name!='\0' && strcmp(mmz_name, p->name)) \
				continue; \
		} \
		if((mmz_name==NULL) && (anony==1)){ \
			if(strcmp("anonymous", p->name)) \
				continue; \
		} \				
		mmz_trace(1, HIL_MMZ_FMT_S, hil_mmz_fmt_arg(p));

#define end_list_for_each_mmz() }


static unsigned long _strtoul_ex(const char *s, char **ep, unsigned int base)
{
	char *__end_p;
	unsigned long __value;
	
	__value = simple_strtoul(s,&__end_p,base); 

	switch(*__end_p) { 
	case 'm': 
	case 'M':
		__value <<= 10; 
	case 'k': 
	case 'K': 
		__value <<= 10; 
		if(ep)
			(*ep) = __end_p + 1;
	default: 
		break; 
	} 

	return __value;
}


static LIST_HEAD(mmz_list);
static DECLARE_MUTEX(mmz_lock);

static int anony = 0;
module_param(anony, int, S_IRUGO);

hil_mmz_t *hil_mmz_create(const char *name, unsigned long gfp, unsigned long phys_start, unsigned long nbytes)
{
	hil_mmz_t *p;

	mmz_trace_func();

	if(name == NULL) {
		printk(KERN_ERR "%s: 'name' can not be zero!", __FUNCTION__);
		return NULL;
	}

	p = kmalloc(sizeof(hil_mmz_t), GFP_KERNEL);

	memset(p, 0, sizeof(hil_mmz_t));
	strlcpy(p->name, name, HIL_MMZ_NAME_LEN);
	p->gfp = gfp;
	p->phys_start = phys_start;
	p->nbytes = nbytes;

	INIT_LIST_HEAD(&p->list);
	INIT_LIST_HEAD(&p->mmb_list);

	p->destructor = kfree;

	return p;
}

int hil_mmz_destroy(hil_mmz_t *zone)
{
	if(zone == NULL)
		return -1;
	if(zone->destructor)
		zone->destructor(zone);
	return 0;
}

static int _check_mmz(hil_mmz_t *zone)
{
	hil_mmz_t *p;

	unsigned long new_start=zone->phys_start;
	unsigned long new_end=zone->phys_start+zone->nbytes;

	if(zone->nbytes == 0)
		return -1;

	if(!((new_start>=__pa(high_memory)) || (new_start<PHYS_OFFSET && new_end<=PHYS_OFFSET))) {
		printk(KERN_ERR "ERROR: Conflict MMZ:\n");
		printk(KERN_ERR HIL_MMZ_FMT_S "\n", hil_mmz_fmt_arg(zone));
		printk(KERN_ERR "MMZ conflict to kernel memory (0x%08lX, 0x%08lX)\n", PHYS_OFFSET, __pa(high_memory)-1);

		return -1;
	}

	list_for_each_entry(p,&mmz_list, list) {
		unsigned long start,end;

		start = p->phys_start;
		end   = p->phys_start + p->nbytes;

		if(new_start >= end) {
			continue;
		} else if(new_start<start && new_end<=start) {
			continue;
		} else {}

		printk(KERN_ERR "ERROR: Conflict MMZ:\n");
		printk(KERN_ERR "MMZ new:   " HIL_MMZ_FMT_S "\n", hil_mmz_fmt_arg(zone));
		printk(KERN_ERR "MMZ exist: " HIL_MMZ_FMT_S "\n", hil_mmz_fmt_arg(p));
		printk(KERN_ERR "Add new MMZ failed!\n");
		return -1;
	}

	return 0;
}

int hil_mmz_register(hil_mmz_t *zone)
{
	int ret = 0;

	mmz_trace(1, HIL_MMZ_FMT_S, hil_mmz_fmt_arg(zone));

	if(zone == NULL)
		return -1;

	down(&mmz_lock);

	ret = _check_mmz(zone);
	if(ret) {
		up(&mmz_lock);
		return ret;
	}

	zone->bitmap = kmalloc(mmz_bitmap_size(zone), GFP_KERNEL);
	memset(zone->bitmap, 0, mmz_bitmap_size(zone));

	INIT_LIST_HEAD(&zone->mmb_list);

	list_add(&zone->list, &mmz_list);

	up(&mmz_lock);

	return 0;
}

int hil_mmz_unregister(hil_mmz_t *zone)
{
	int losts = 0;
	hil_mmb_t *p;

	if(zone == NULL)
		return -1;

	mmz_trace_func();

	down(&mmz_lock);
	list_for_each_entry(p,&zone->mmb_list, list) {
		printk(KERN_WARNING "          MB Lost: " HIL_MMB_FMT_S "\n", hil_mmb_fmt_arg(p));
		losts++;
	}

	if(losts) {
		printk(KERN_ERR "%d mmbs not free, mmz<%s> can not be deregistered!\n", losts, zone->name);
		up(&mmz_lock);
		return -1;
	}

	list_del(&zone->list);

	kfree(zone->bitmap);
	zone->bitmap = NULL;
	hil_mmz_destroy(zone);

	up(&mmz_lock);

	return 0;
}

static unsigned long _find_fixed_region(unsigned long *region_len, hil_mmz_t *mmz,
		unsigned long size, unsigned long align)
{
	int i;
	unsigned long fixed_start=0;
	unsigned long fixed_len=~1;

	mmz_trace_func();

	/* align to phys address first! */
	i = mmz_phy_addr2pos(mmz, mmz_align2(mmz->phys_start, align));
	align = mmz_grain_align(align)/MMZ_GRAIN;

	for(; i<mmz_length2grain(mmz->nbytes); i+=align) {
		unsigned long start;
		unsigned long len;

		if(mmz_get_bit(mmz,i))
			continue;

		len = 0;
		start = mmz_pos2phy_addr(mmz,i);
		for(; i<mmz_length2grain(mmz->nbytes); i++) {
			if(mmz_get_bit(mmz,i)) {
				i = mmz_align2(i,align);
				break;
			}

			len += MMZ_GRAIN;
		}

		mmz_trace(1,"      region: start=0x%08lX, len=%luKB", start, len/SZ_1K);

		if((len<fixed_len) && (len>=size)) {
			fixed_len = len;
			fixed_start = start;
			mmz_trace(1,"fixed_region: start=0x%08lX, len=%luKB", fixed_start, fixed_len/SZ_1K);
		}
	}

	*region_len = fixed_len;

	return fixed_start;
}

static int _do_mmb_alloc(hil_mmb_t *mmb)
{
	int i;
	hil_mmb_t *p=NULL;

	mmz_trace_func();

	for(i=0; i<mmz_length2grain(mmb->length); i++)
		mmz_set_bit(mmb->zone,i+mmz_phy_addr2pos(mmb->zone,mmb->phys_addr));

	/* add mmb sorted */
	list_for_each_entry(p,&mmb->zone->mmb_list, list) {
		if(mmb->phys_addr < p->phys_addr)
			break;
		if(mmb->phys_addr == p->phys_addr){
			printk(KERN_ERR "ERROR: media-mem allocator bad in %s! (%s, %d)",
					mmb->zone->name,  __FUNCTION__, __LINE__);
		}
	}
	list_add(&mmb->list, p->list.prev);

	mmz_trace(1,HIL_MMB_FMT_S,hil_mmb_fmt_arg(mmb));

	return 0;
}

static hil_mmb_t *__mmb_alloc(const char *name, unsigned long size, unsigned long align, 
		unsigned long gfp, const char *mmz_name, hil_mmz_t *_user_mmz)
{
	hil_mmz_t *mmz;
	hil_mmb_t *mmb;

	unsigned long orig_size = size;

	unsigned long start;
	unsigned long region_len;

	unsigned long fixed_start=0;
	unsigned long fixed_len=~1;
	hil_mmz_t *fixed_mmz=NULL;

	mmz_trace_func();

	if(size == 0 || size > 0x40000000UL)
		return NULL;
	if(align == 0)
		align = 1;

	size = mmz_grain_align(size);

	mmz_trace(1,"size=%luKB, align=%lu", size/SZ_1K, align);

	begin_list_for_each_mmz(mmz, gfp, mmz_name)
		if(_user_mmz!=NULL && _user_mmz!=mmz)
			continue;
		start = _find_fixed_region(&region_len, mmz, size, align);
		if( (fixed_len > region_len) && (start!=0)) {
			fixed_len = region_len;
			fixed_start = start;
			fixed_mmz = mmz;
		}
	end_list_for_each_mmz()

	if(fixed_mmz == NULL) {
		return NULL;
	}

	mmb = kmalloc(sizeof(hil_mmb_t), GFP_KERNEL);

	memset(mmb, 0, sizeof(hil_mmb_t));
	mmb->zone = fixed_mmz;
	mmb->phys_addr = fixed_start;
	mmb->length = orig_size;
	if(name)
		strlcpy(mmb->name, name, HIL_MMB_NAME_LEN);
	else strcpy(mmb->name, "<null>");

	if(_do_mmb_alloc(mmb)) {
		kfree(mmb);
		mmb = NULL;
	}

	return mmb;
}

hil_mmb_t *hil_mmb_alloc(const char *name, unsigned long size, unsigned long align, 
		unsigned long gfp, const char *mmz_name)
{
	hil_mmb_t *mmb;

	down(&mmz_lock);
	mmb = __mmb_alloc(name, size, align, gfp, mmz_name, NULL);
	up(&mmz_lock);

	return mmb;
}

hil_mmb_t *hil_mmb_alloc_in(const char *name, unsigned long size, unsigned long align, 
		hil_mmz_t *_user_mmz)
{
	hil_mmb_t *mmb;

	if(_user_mmz==NULL)
		return NULL;

	down(&mmz_lock);
	mmb = __mmb_alloc(name, size, align, _user_mmz->gfp, _user_mmz->name, _user_mmz);
	up(&mmz_lock);

	return mmb;
}

static void *_mmb_map2kern(hil_mmb_t *mmb, int cached)
{
	if(mmb->flags & HIL_MMB_MAP2KERN) {
		if((cached*HIL_MMB_MAP2KERN_CACHED) != (mmb->flags&HIL_MMB_MAP2KERN_CACHED)) {
			printk(KERN_ERR "mmb<%s> already kernel-mapped %s, can not be re-mapped as %s.",
					mmb->name,
					(mmb->flags&HIL_MMB_MAP2KERN_CACHED) ? "cached" : "non-cached",
					(cached) ? "cached" : "non-cached" );
			return NULL;
		}

		mmb->map_ref++;

		return mmb->kvirt;
	}

	if(cached) {
		mmb->flags |= HIL_MMB_MAP2KERN_CACHED;
		mmb->kvirt = ioremap_cached(mmb->phys_addr, mmb->length);
	} else {
		mmb->flags &= ~HIL_MMB_MAP2KERN_CACHED;
		mmb->kvirt = ioremap_nocache(mmb->phys_addr, mmb->length);
	}

	if(mmb->kvirt) {
	       	mmb->flags |= HIL_MMB_MAP2KERN;
		mmb->map_ref++;
	}

	return mmb->kvirt;
}

void *hil_mmb_map2kern(hil_mmb_t *mmb)
{
	void *p;
	 
	if(mmb == NULL)
		return NULL;

	down(&mmz_lock);
	p =  _mmb_map2kern(mmb, 0);
	up(&mmz_lock);

	return p;
}

void *hil_mmb_map2kern_cached(hil_mmb_t *mmb)
{
	void *p;

	if(mmb == NULL)
		return NULL;

	down(&mmz_lock);
	p = _mmb_map2kern(mmb, 1);
	up(&mmz_lock);

	return p;
}

static int _mmb_free(hil_mmb_t *mmb);

int hil_mmb_unmap(hil_mmb_t *mmb)
{
	int ref;

	if(mmb == NULL)
		return -1;

	down(&mmz_lock);

	if(mmb->flags & HIL_MMB_MAP2KERN) {
		ref = --mmb->map_ref;
		if(mmb->map_ref !=0) {
			up(&mmz_lock);
			return ref;
		}

		iounmap(mmb->kvirt);
	}

	mmb->kvirt = NULL;
	mmb->flags &= ~HIL_MMB_MAP2KERN;
	mmb->flags &= ~HIL_MMB_MAP2KERN_CACHED;

	if((mmb->flags & HIL_MMB_RELEASED) && mmb->phy_ref ==0) {
		_mmb_free(mmb);
	}

	up(&mmz_lock);

	return 0;
}

int hil_mmb_get(hil_mmb_t *mmb)
{
	int ref;

	if(mmb == NULL)
		return -1;

	down(&mmz_lock);

	if(mmb->flags & HIL_MMB_RELEASED)
		printk(KERN_WARNING "hil_mmb_get: amazing, mmb<%s> is released!\n", mmb->name);
	ref = ++mmb->phy_ref;

	up(&mmz_lock);

	return ref;
}

static int _mmb_free(hil_mmb_t *mmb)
{
	int i;

	for(i=0; i<mmz_length2grain(mmb->length); i++)
		mmz_clr_bit(mmb->zone,i+mmz_phy_addr2pos(mmb->zone,mmb->phys_addr));

	list_del(&mmb->list);
	kfree(mmb);

	return 0;
}

int hil_mmb_put(hil_mmb_t *mmb)
{
	int ref;

	if(mmb == NULL)
		return -1;

	down(&mmz_lock);

	if(mmb->flags & HIL_MMB_RELEASED)
		printk(KERN_WARNING "hil_mmb_put: amazing, mmb<%s> is released!\n", mmb->name);

	ref = --mmb->phy_ref;
	
	if((mmb->flags & HIL_MMB_RELEASED) && mmb->phy_ref ==0 && mmb->map_ref ==0) {
		_mmb_free(mmb);
	}

	up(&mmz_lock);

	return ref;
}

int hil_mmb_free(hil_mmb_t *mmb)
{
	mmz_trace_func();

	if(mmb == NULL)
		return -1;

	mmz_trace(1,HIL_MMB_FMT_S,hil_mmb_fmt_arg(mmb));

	down(&mmz_lock);

	if(mmb->flags & HIL_MMB_RELEASED) {
		printk(KERN_WARNING "hil_mmb_free: amazing, mmb<%s> is released before, but still used!\n", mmb->name);

		up(&mmz_lock);

		return 0;
	}

	if(mmb->phy_ref >0) {
		printk(KERN_WARNING "hil_mmb_free: free mmb<%s> delayed for which ref-count is %d!\n",
				mmb->name, mmb->map_ref);
		mmb->flags |= HIL_MMB_RELEASED;
		up(&mmz_lock);

		return 0;
	}

	if(mmb->flags & HIL_MMB_MAP2KERN) {
		printk(KERN_WARNING "hil_mmb_free: free mmb<%s> delayed for which is kernel-mapped to 0x%p with map_ref %d!\n",
				mmb->name, mmb->kvirt, mmb->map_ref);
		mmb->flags |= HIL_MMB_RELEASED;
		up(&mmz_lock);

		return 0;
	}

	_mmb_free(mmb);

	up(&mmz_lock);

	return 0;
}

#define MACH_MMB(p, val, member) do{\
					hil_mmz_t *__mach_mmb_zone__; \
					(p) = NULL;\
					list_for_each_entry(__mach_mmb_zone__,&mmz_list, list) { \
						hil_mmb_t *__mach_mmb__;\
						list_for_each_entry(__mach_mmb__,&__mach_mmb_zone__->mmb_list, list) { \
							if(__mach_mmb__->member == (val)){ \
								(p) = __mach_mmb__; \
								break;\
							} \
						} \
						if(p)break;\
					} \
				}while(0)

hil_mmb_t *hil_mmb_getby_phys(unsigned long addr)
{
	hil_mmb_t *p;

	down(&mmz_lock);
	MACH_MMB(p, addr, phys_addr);
	up(&mmz_lock);

	return p;
}

hil_mmb_t *hil_mmb_getby_kvirt(void *virt)
{
	hil_mmb_t *p;

	if(virt == NULL)
		return NULL;

	down(&mmz_lock);
	MACH_MMB(p, virt, kvirt);
	up(&mmz_lock);

	return p;
}


hil_mmz_t *hil_mmz_find(unsigned long gfp, const char *mmz_name)
{
	hil_mmz_t *p;

	down(&mmz_lock);
	begin_list_for_each_mmz(p, gfp, mmz_name)
		up(&mmz_lock);
		return p;
	end_list_for_each_mmz()
	up(&mmz_lock);

	return NULL;
}

/*
 * name,gfp,phys_start,nbytes;...
 * All param in hex mode, except name.
 */
static int media_mem_parse_cmdline(char *s)
{
	hil_mmz_t *zone = NULL;
	char *line;

	while( (line = strsep(&s,":")) !=NULL) {
		int i;
		char *argv[4];

		/*
		 * FIXME: We got 4 args in "line", formated "argv[0],argv[1],argv[2],argv[3]".
		 * eg: "<mmz_name>,<gfp>,<phys_start_addr>,<size>"
		 * For more convenient, "hard code" are used such as "arg[0]", i.e.
		 */
		for(i=0; (argv[i] = strsep(&line,",")) != NULL;)
			if(++i == ARRAY_SIZE(argv)) break;
		if(i < ARRAY_SIZE(argv))
			continue;

		zone = hil_mmz_create("null",0,0,0);

		strlcpy(zone->name, argv[0], HIL_MMZ_NAME_LEN);
		zone->gfp = _strtoul_ex(argv[1], NULL, 0);
		zone->phys_start = _strtoul_ex(argv[2], NULL, 0);
		zone->nbytes = _strtoul_ex(argv[3], NULL, 0);

		if(hil_mmz_register(zone)) {
			printk(KERN_WARNING "Add MMZ failed: " HIL_MMZ_FMT_S "\n", hil_mmz_fmt_arg(zone));
			hil_mmz_destroy(zone);
		}
		zone = NULL;
	}

	return 0;
}


#define MEDIA_MEM_NAME  "media-mem"

#ifdef CONFIG_PROC_FS

static int mmz_read_proc(char *page, char **start, off_t off,
		                          int count, int *eof, void *data)
{
	hil_mmz_t *p;
	unsigned int pos=0;
	int len=0;

	mmz_trace_func();

	down(&mmz_lock);
	list_for_each_entry(p,&mmz_list, list) {
		hil_mmb_t *mmb;

		if(pos == off) {
			len = snprintf(page, count, "+---ZONE: " HIL_MMZ_FMT_S "\n", hil_mmz_fmt_arg(p));
			goto mid_exit;
		}
		pos++;

		list_for_each_entry(mmb,&p->mmb_list, list) {
			if(pos == off) {
				len = snprintf(page, count, "   |-MMB: " HIL_MMB_FMT_S "\n", hil_mmb_fmt_arg(mmb));
				goto mid_exit;
			}
			pos++;
		}
	}

	*eof = 1;

mid_exit:
	if(*eof == 0) {
		if(count<len) 
			len = count-1;
		*start = (char*)1;
	}
	up(&mmz_lock);

	return len;
}

static int mmz_write_proc(struct file *file, const char __user *buffer,
		                           unsigned long count, void *data)
{
	char buf[256];

	if(count >= sizeof(buf)) {
		printk(KERN_ERR "MMZ: your parameter string is too long!\n");
		return -EIO;
	}

	memset(buf, 0, sizeof(buf));
	copy_from_user(buf, buffer, count);
	media_mem_parse_cmdline(buf);

	return count;
}


static int __init media_mem_proc_init(void)
{
	struct proc_dir_entry *p;

	p = create_proc_entry(MEDIA_MEM_NAME, 0644, &proc_root);
	if(p == NULL)
		return -1;
	p->read_proc = mmz_read_proc;
	p->write_proc = mmz_write_proc;

        return 0;
}

static void __exit media_mem_proc_exit(void)
{
	remove_proc_entry(MEDIA_MEM_NAME, &proc_root);
}

#else
static int __init media_mem_proc_init(void){ return 0; }
static void __exit media_mem_proc_exit(void){ }

#endif /* CONFIG_PROC_FS */

#define MMZ_SETUP_CMDLINE_LEN 256

#ifndef MODULE

static char __initdata setup_zones[MMZ_SETUP_CMDLINE_LEN] = CONFIG_HISILICON_MMZ_DEFAULT;
static int __init parse_kern_cmdline(char *line)
{
	strlcpy(setup_zones, line, sizeof(setup_zones));

	return 1;
}
__setup("mmz=", parse_kern_cmdline);

#else
static char __initdata setup_zones[MMZ_SETUP_CMDLINE_LEN]={'\0'};
module_param_string(mmz, setup_zones, MMZ_SETUP_CMDLINE_LEN, 0600);
MODULE_PARM_DESC(mmz,"mmz=name,0,start,size:[others]");
#endif

static int __init media_mem_init(void)
{
	printk(KERN_INFO "Hisilicon Media Memory Zone Manager\n");

	media_mem_proc_init();

	media_mem_parse_cmdline(setup_zones);

	kcom_mmz_register();

	mmz_userdev_init();

	return 0;
}

#ifdef MODULE

static void __exit mmz_exit_check(void)
{
	hil_mmz_t *p;

	mmz_trace_func();

	for(p=hil_mmz_find(0,NULL); p!=NULL; p=hil_mmz_find(0,NULL)) {
		printk(KERN_WARNING "MMZ force removed: " HIL_MMZ_FMT_S "\n", hil_mmz_fmt_arg(p));
		hil_mmz_unregister(p);
	}
}

static void __exit media_mem_exit(void)
{
	mmz_userdev_exit();

	kcom_mmz_unregister();

	mmz_exit_check();

	media_mem_proc_exit();
}

module_init(media_mem_init);
module_exit(media_mem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liu jiandong");

#else

subsys_initcall(media_mem_init);

#endif

