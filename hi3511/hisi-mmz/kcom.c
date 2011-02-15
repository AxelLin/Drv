/* kcom.c
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

#include <linux/kcom.h>

#include <kcom-mmz.h>
#include <media-mem.h>


static kcom_mmz_t *new_zone(const char *name, unsigned long phys_start, unsigned long size)
{
	hil_mmz_t *mmz;
	
	mmz = hil_mmz_create(name, 0, phys_start, size);
	if(mmz ==NULL)
		return NULL;
	if(hil_mmz_register(mmz) !=0) {
		hil_mmz_destroy(mmz);
		return NULL;
	}

	return (kcom_mmz_t *)mmz;
}

static void delete_zone(kcom_mmz_t *zone)
{
	hil_mmz_unregister((kcom_mmz_t *)zone);
	hil_mmz_destroy((kcom_mmz_t *)zone);
}

static mmb_addr_t new_mmb(const char *name, int size, unsigned long align, const char *zone_name)
{
	hil_mmb_t *mmb;
	
	mmb = hil_mmb_alloc(name, size, align, 0, zone_name);
	if(mmb ==NULL)
		return MMB_ADDR_INVALID;

	return (mmb_addr_t)hil_mmb_phys(mmb);
}

static void delete_mmb(mmb_addr_t addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_phys((unsigned long)addr);
	if(mmb ==NULL)
		return;

	hil_mmb_free(mmb);
}

static void *remap_mmb(mmb_addr_t addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_phys((unsigned long)addr);
	if(mmb ==NULL)
		return NULL;

	return hil_mmb_map2kern(mmb);
}

static void *remap_mmb_cached(mmb_addr_t addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_phys((unsigned long)addr);
	if(mmb ==NULL)
		return NULL;

	return hil_mmb_map2kern_cached(mmb);
}

void *	refer_mapped_mmb(void *mapped_addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_kvirt(mapped_addr);
	if(mmb ==NULL)
		return NULL;

	if(mmb->flags & HIL_MMB_MAP2KERN_CACHED)
		return hil_mmb_map2kern_cached(mmb);
	else
		return hil_mmb_map2kern(mmb);
}

static int unmap_mmb(void *mapped_addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_kvirt(mapped_addr);
	if(mmb ==NULL)
		return -1;

	return hil_mmb_unmap(mmb);
}

#if 0
static mmb_addr_t mapped_to_mmb(void *mapped_addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_kvirt(mapped_addr);
	if(mmb ==NULL)
		return MMB_ADDR_INVALID;

	return hil_mmb_phys(mmb);
}
#endif

static int get_mmb(mmb_addr_t addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_phys((unsigned long)addr);
	if(mmb ==NULL)
		return -1;

	return hil_mmb_get(mmb);
}

static int put_mmb(mmb_addr_t addr)
{
	hil_mmb_t *mmb;

	mmb = hil_mmb_getby_phys((unsigned long)addr);
	if(mmb ==NULL)
		return -1;

	return hil_mmb_put(mmb);
}

static struct kcom_media_memory kcom_mmz = {
	.kcom = KCOM_OBJ_INIT(UUID_MEDIA_MEMORY, UUID_MEDIA_MEMORY, NULL, THIS_MODULE, KCOM_TYPE_OBJECT, NULL),

	.new_zone = new_zone,
	.delete_zone = delete_zone,

	.new_mmb = new_mmb,
	.delete_mmb = delete_mmb,

	.remap_mmb = remap_mmb,
	.remap_mmb_cached = remap_mmb_cached,
	.refer_mapped_mmb = refer_mapped_mmb,
	.unmap_mmb = unmap_mmb,

	.get_mmb = get_mmb,
	.put_mmb = put_mmb,
};

int kcom_mmz_register(void)
{
	return kcom_register(&kcom_mmz.kcom);
}

void kcom_mmz_unregister(void)
{
	kcom_unregister(&kcom_mmz.kcom);
}

