#ifndef __MEMPOOL_USER_H__
#define __MEMPOOL_USER_H__

#include <sys/ioctl.h>

struct mem_block{
        unsigned int handle;
        unsigned int addr;
        unsigned int size;
};  

#define IOC_MEMPOOL_INIT		_IOWR('m', 0,  int)
#define IOC_GETMMB_ADDR			_IOWR('m', 1,  struct mem_block)
#define IOC_MEMPOOL_DESTORY		_IOWR('m', 2,  int)
#define IOC_MEMPOOL_FREE		_IOWR('m', 3,  struct mem_block)
#define IOC_MEMPOOL_ALLOC		_IOWR('m', 4,  struct mem_block)
#define IOC_MEMPOOL_ADD			_IOWR('m', 5,  struct mem_block)
#define IOC_MEMPOOL_CREATE		_IOWR('m', 6,  struct mem_block)
#endif

