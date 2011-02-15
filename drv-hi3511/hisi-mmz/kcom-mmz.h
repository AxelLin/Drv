
#ifndef __KCOM_MEDIA_MEM_H__
#define __KCOM_MEDIA_MEM_H__

#define UUID_MEDIA_MEMORY "media-mem-1.0.0.0"

typedef void kcom_mmz_t;
typedef unsigned long mmb_addr_t;

#define MMB_ADDR_INVALID (~0)

struct kcom_media_memory {
	struct kcom_object kcom;

	kcom_mmz_t *	(*new_zone)(const char *name, unsigned long phys_start, unsigned long size);
	void 		(*delete_zone)(kcom_mmz_t *zone);

	mmb_addr_t 	(*new_mmb)(const char *name, int size, unsigned long align, const char *zone_name);
	void 		(*delete_mmb)(mmb_addr_t addr);

	void *		(*remap_mmb)(mmb_addr_t addr);
	void *		(*remap_mmb_cached)(mmb_addr_t addr);
	void *		(*refer_mapped_mmb)(void *mapped_addr);
	int 		(*unmap_mmb)(void *mapped_addr);

	int		(*get_mmb)(mmb_addr_t addr);
	int		(*put_mmb)(mmb_addr_t addr);
};

#define kcom_to_mmz(__kcom) kcom_to_instance(__kcom, struct kcom_media_memory, kcom)


#endif

