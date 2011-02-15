#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "mempool.h"
#include "hios_type.h"


/*
  主cpu上的示例程序
*/


static int __init testmempool_init(void)
{
	hios_u32 mempool[10];
	hios_s32 i,count=0;
	hios_u32 handle ,handle2;
	struct mem_block mmb[40];
	hios_char *addr=HIOS_NULL;

	printk("##testmempool host 1.0!\n");
	
	/*使用前必须先初始化。输入参数为local_id，用于标识不同的CPU*/	
	hios_mempool_init(0);
	
	//msleep(5000);
	
	/*创建内存池，并设置内存池属性。
	  在向内存池中添加内存块前，内存池为空，不可用于分配内存。
	  参数意义：
	  1、name，字符串，长度小于20
	  2、内存池的块大小，分配的内存大小将按块对齐。块大小本身会4k字节对齐。
	  3、对端cpu_id，本例中，对端为从cpu，id为1
	  4、port号。注：主从不能使用相同的port号。
	  5、优先级。发送短消息的优先级。0：低优先级。1：高优先级，立即触发中断。	
	*/	
	handle = hios_mempool_create("mempool_host", 100*1024,1,128,0);
	
	printk("create sucess! handle = %x\n",handle);

	/*分配内存，本例中连续分配10块。做出while(1)是为了等待从cpu添加内存块
	  如果已经添加了内存块，可直接分配。
	  参数意义：
	  1、mmb结构，用于保存分配内存块的信息，传递给hios_mempool_alloc接口，用于释放。
	              如果释放操作在其它cpu上进行，需要通过短消息将mmb传递给其它cpu
	  2、内存池handle，本例中，就是低38行得到的handle
	  3、要分配的内存大小。实际分配的大小将按照内存池的块大小对齐。
	  
	  返回值：
	  addr ：内存块的首地址, 此地址可直接传递给对端使用。也可以在对端通过hios_char *hios_getmmb_addr(struct mem_block mmb);接口获取。
	         
	  HIOS_NULL: 分配失败。请包含”hios_type.h“
	*/
	while(1)
	{
		addr = hios_mempool_alloc(&mmb[count], handle, 0x20000);
		if(addr)
		{
			printk("alloc sucess! addr = 0x%x\n",(unsigned int)addr);
			printk("mmb.handle=0x%x\n",mmb[count].handle);
			printk("mmb.addr=0x%x\n",mmb[count].addr);
			printk("mmb.size=0x%x\n",mmb[count].size);
			count ++;
		}
		else
		{
			msleep(50);
			continue;
		}
		
		if(count >9 )
			break;
	}
	/*本例中的delay操作是为了与从cpu上的样例进行同步*/
	msleep(2000);
	
	printk("\n\n\n\n\n\n");
	
	
	/*此时，从cpu已经释放了几个内存块，重新分配，以验证释放操作是否成功。*/
	count = 0;
	addr = hios_mempool_alloc(&mmb[count], handle, 0x32000);
	if(addr)
	{
		printk("alloc sucess! addr = 0x%x\n",(unsigned int)addr);
		printk("mmb.handle=0x%x\n",mmb[count].handle);
		printk("mmb.addr=0x%x\n",mmb[count].addr);
		printk("mmb.size=0x%x\n",mmb[count].size);
		count ++;
	}	

	addr = hios_mempool_alloc(&mmb[count], handle, 0x32000*2);
	if(addr)
	{
		printk("alloc sucess! addr = 0x%x\n",(unsigned int)addr);
		printk("mmb.handle=0x%x\n",mmb[count].handle);
		printk("mmb.addr=0x%x\n",mmb[count].addr);
		printk("mmb.size=0x%x\n",mmb[count].size);
		count ++;
	}	
	
	msleep(2000);
	
	
	/*此时，从cpu已经删除了内存池
	  执行分配操作，以验证内存池是否删除成功。
	*/
	hios_mempool_alloc(&mmb[count], handle, 0x32000);
	
    	return 0;
}

static void __exit testmempool_exit(void)
{
	return;
}

module_init(testmempool_init);
module_exit(testmempool_exit);

MODULE_LICENSE("GPL");

