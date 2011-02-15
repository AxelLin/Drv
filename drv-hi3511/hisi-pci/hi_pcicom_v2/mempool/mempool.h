#ifndef __HIOS_MEMPOOL_H__
#define __HIOS_MEMPOOL_H__

#include "hios_type.h"

#define EERROR -1
#define OK      0

/*此结构用于参数传递，可不关心具体含义*/
struct mem_block{
        hios_u32 handle;
        hios_u32 addr;
        hios_u32 size;
};  

/*创建内存池*/
extern hios_u32 hios_mempool_create(
                        hios_char *name,    //长度小于20字节
                        hios_u32 blk_sz,    //内存池的块大小
                        hios_u32 remote_id, //对端cpu id
                        hios_u32 port,      //用与发送短消息的port号
                        hios_u32 priority   //短消息的优先级
                        );
                        
/*向内存池中添加内存块*/                        
extern hios_status hios_add_mempool(
                        hios_u32 handle,    //内存池handle，由hios_mempool_create接口返回
                        hios_u32 *mempool,  //内存块信息
                        hios_u32 count      //内存块的个数
                        );
                        
/*分配内存*/                        
extern hios_char *hios_mempool_alloc(
                        struct mem_block *mmb,  //输出参数，用于保存分配内存块的信息
                        hios_u32 handle,        //内存池handle，由hios_mempool_create接口返回
                        hios_u32 size           //分配内存的大小
                        );

/*释放内存*/
extern hios_void hios_mempool_free(struct mem_block mmb); //mmb结构由hios_mempool_alloc接口输出

/*删除内存池*/
extern hios_void hios_mempool_destory(hios_u32 handle);  //内存池handle，由hios_mempool_create接口返回

/*通过mmb结构，获取分配内存的首地址*/
extern hios_char *hios_getmmb_addr(struct mem_block mmb);  //mmb结构由hios_mempool_alloc接口输出

/*内存池初始化函数，创建内存池前，需要先调用此接口*/
extern hios_void hios_mempool_init(hios_u32 cpu_id);  //local_cpu id


#define IOC_MEMPOOL_INIT		_IOWR('m', 0,  int)
#define IOC_GETMMB_ADDR			_IOWR('m', 1,  struct mem_block)
#define IOC_MEMPOOL_DESTORY		_IOWR('m', 2,  int)
#define IOC_MEMPOOL_FREE		_IOWR('m', 3,  struct mem_block)
#define IOC_MEMPOOL_ALLOC		_IOWR('m', 4,  struct mem_block)
#define IOC_MEMPOOL_ADD			_IOWR('m', 5,  struct mem_block)
#define IOC_MEMPOOL_CREATE		_IOWR('m', 6,  struct mem_block)

#endif
