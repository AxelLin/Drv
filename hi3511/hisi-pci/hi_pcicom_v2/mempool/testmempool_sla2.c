#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>


#include "mempool.h"
#include "hios_type.h"

/*运行在从cpu上的示例程序*/

static int __init testmempool_sla_init(void)
{
	hios_u32 mempool[10];
	hios_s32 i,count=0;
	hios_u32 handle ,handle2;
	struct mem_block mmb[40];
	hios_u32 memblk[40];
	hios_char *addr;

	printk("##testmempool sla 1.0!\n");
	
	/*使用前必须先初始化。输入参数为local_id，用于标识不同的CPU*/
	hios_mempool_init(1);
	
	/*创建内存池，并设置内存池属性。
	  在向内存池中添加内存块前，内存池为空，不可用于分配内存。
	  参数意义：
	  1、name，字符串，长度小于20
	  2、内存池的块大小，分配的内存大小将按块对齐。块大小本身会4k字节对齐。
	  3、对端cpu_id，本例中，对端为主cpu，id为0
	  4、port号。注：主从不能使用相同的port号。
	  5、优先级。发送短消息的优先级。0：低优先级。1：高优先级，立即触发中断。	
	*/
	handle = hios_mempool_create("mempool_slave2", 10*1024,0,124,0);

	/*
	   在创建内存池后，需要对端往内存池中添加内存块，此处需要将hanlde通过短消息发送到对端。	
	*/
        /*
           发送handle到对端的操作略，请自行发送。
        */


        /*向对端内存池中添加内存块，对端内存池handle需要通过上一步发送过来。*/
	
	/*填充内存块信息*/
	msleep(500);
	memblk[0] = 0xB0000000;      //内存块首地址
	memblk[1] = memblk[0] + 0x100000; //内存块尾地址
	memblk[2] = 0xB2000000;           //内存块首地址
	memblk[3] = memblk[2] + 0x100000; //内存块尾地址
	
	/*向对端内存池添加2个内存块。受缓冲buf限制，一次最多添加10个内存块
	  参数意义：
	  1：对端内存池handle，需要通过短消息接收。本例省略接收的步骤，直接赋值。
	  2：内存块信息首地址
	  3：内存块的个数。注：非内存块信息buf的长度。一个“首尾地址对“代表一个内存块。本例填充了两个内存块的信息
	*/
	i = hios_add_mempool(0x200001, memblk, 2);
	if(i)//返回值非零，表示出错
		printk("ERROR! i= %d\n",i);	


	/*本例中的延迟，目的是为了和主cpu上的样例程序同步*/
	msleep(1000);


	/*释放内存*/
	mmb[0].handle=0x200001;
	mmb[0].addr = 0xB0000000;
	mmb[0].size=0x32000;
	
	/*
	释放对端内存
	参数意义：
	mmb结构由hios_mempool_alloc接口返回
	分配操作可参考主cpu的样例程序。
	本例中没有mmb的传送操作，直接构造了mmb结构。
	释放时，不需要关心mmb结构的具体内容。		
	*/
	hios_mempool_free(mmb[0]);
	
	mmb[0].handle=0x200001;
	mmb[0].addr = 0xB2032000;
	mmb[0].size=0x32000;
	hios_mempool_free(mmb[0]);	
	
	mmb[0].handle=0x200001;
	mmb[0].addr = 0xB2096000;
	mmb[0].size=0x32000;
	hios_mempool_free(mmb[0]);
	
	msleep(2000);
	
	
	/*
	删除内存池，可用来删除本地或者对端的内存池，通过handle区分。
	参数意义：
	内存池handle
	*/
	hios_mempool_destory(0x200001);

    	return 0;
}

static void __exit testmempool_sla_exit(void)
{
	return;
}

module_init(testmempool_sla_init);
module_exit(testmempool_sla_exit);

MODULE_LICENSE("GPL");

