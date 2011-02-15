#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "mempool_user.h"


/*
  用户态接口示例程序
  主从配合，请参考内核态示例程序
  被例中，仅说明用户态接口的使用方法。
*/


int main()
{
	unsigned int mempool[10];
	int i,count=0;
	unsigned int handle ,handle2;
	struct mem_block *mmb;
	char *addr=NULL;
	int fd;
	//char name[20];
	char *pname = "usemempool";
	unsigned int buf[22];
	unsigned int memblk[40];

	/*打开设备*/	
	fd = open("/dev/misc/mempool",O_RDWR);
	

	/*初始化*/		
	//hios_mempool_init(0);
	handle = 0;
	/*传入参数为本地ID，用指针传入*/
	ioctl(fd,IOC_MEMPOOL_INIT,&handle);
	printf("handle = %x\n",handle);
	//msleep(5000);

	/*创建内存池，并设置内存池属性。
	  内核态接口：hios_mempool_create("mempool_host", 100*1024,1,128,0);
	  在向内存池中添加内存块前，内存池为空，不可用于分配内存。
	  参数意义：
	  1、name，字符串，长度小于20
	  2、内存池的块大小，分配的内存大小将按块对齐。块大小本身会4k字节对齐。
	  3、对端cpu_id，本例中，对端为从cpu，id为1
	  4、port号。注：主从不能使用相同的port号。
	  5、优先级。发送短消息的优先级。0：低优先级。1：高优先级，立即触发中断。	
	*/	
	
	strcpy((char *)buf, pname);
	buf[5] = 100*1024;
	buf[6] = 0;
	buf[7] = 128;
	buf[8] = 0;
	
	/*
	  缓冲区填充结构
	  buf[0]~buf[4]:要创建内存池的名字，最大长度为19字节，添加'\0'
	  buf[5]:内存池块大小
	  buf[6]:远端target_id
	  buf[7]:port号
	  buf[8]:发送短消息的优先级
	*/
	ioctl(fd,IOC_MEMPOOL_CREATE,buf);
	/*
	  返回值：
	  buf[0]:内存池handle
	*/
	handle = buf[0];
	printf("create sucess! handle = %x\n",handle);
	
/*
   添加内存块
*/	
	memblk[0] = 0xC0000000;      //内存块首地址
	memblk[1] = memblk[0] + 0x100000; //内存块尾地址
	memblk[2] = 0xC2000000;           //内存块首地址
	memblk[3] = memblk[2] + 0x100000; //内存块尾地址
	
	/*向对端内存池添加2个内存块。受缓冲buf限制，一次最多添加10个内存块
	  参数意义：
	  1：对端内存池handle，需要通过短消息接收。本例省略接收的步骤，直接赋值。
	  2：内存块信息首地址
	  3：内存块的个数。注：非内存块信息buf的长度。一个“首尾地址对“代表一个内存块。本例填充了两个内存块的信息
	*/
	//i = hios_add_mempool(0x200001, memblk, 2);
	buf[0] = handle;
	buf[1] = 2;
	memcpy(&(buf[2]),memblk,2*8);
	/*
	   buf填充结构：
	   buf[0]:内存池handle
	   buf[1]:内存块个数
	   buf[2]~:内存块起始结束地址对
	   注：一次最多添加10个内存块
	   当添加失败时，ioctl接口返回非零值。
	*/	
	if(!(ioctl(fd,IOC_MEMPOOL_ADD,buf)))
	{
		printf("IOC_MEMPOOL_ADD return error!\n");
	}	
	

	/*分配内存，本例中连续分配10块。做出while(1)是为了等待从cpu添加内存块
	  如果已经添加了内存块，可直接分配。
	  参数意义：
	  1、mmb结构，用于保存分配内存块的信息，传递给hios_mempool_alloc接口，用于释放。
	              如果释放操作在其它cpu上进行，需要通过短消息将mmb传递给其它cpu
	  2、内存池handle，本例中，就是低38行得到的handle
	  3、要分配的内存大小。实际分配的大小将按照内存池的块大小对齐。
	  
	  返回值：
	  addr ：内存块的首地址, 此地址可直接传递给对端使用。也可以在对端通过char *hios_getmmb_addr(struct mem_block mmb);接口获取。
	         
	  NULL: 分配失败。请包含”hios_type.h“
	*/
	while(1)
	{
		//addr = hios_mempool_alloc(&mmb[count], handle, 0x20000);
		buf[0] = handle;
		buf[1] = 0x20000;
		/*
		  分配内存，buf填充结构：
		  buf[0]:内存池handle
		  buf[1]:分配内存块的大小
		  buf还需要用来存储返回信息，大小应不小于struct mem_block
		*/
		ioctl(fd,IOC_MEMPOOL_ALLOC,buf);
		/*
		   返回值：
		   buf[0]:分配内存块的首地址
		   &buf[1]~:stuct mem_block首地址，用于释放内存。
		*/
		if(buf[0])
		{
			mmb = (struct mem_block *)(&(buf[1]));
			printf("alloc sucess! addr = 0x%x\n",(unsigned int)buf[0]);
			printf("mmb.handle=0x%x\n",mmb->handle);
			printf("mmb.addr=0x%x\n",mmb->addr);
			printf("mmb.size=0x%x\n",mmb->size);
			count ++;
		}
		else
		{
			//msleep(50);
			continue;
		}
		
		if(count >9 )
			break;
	}
		return 0;
	
	/*
	  释放内存块：
	  buf填充结构：
	  &buf[0]:struct mem_block首地址
	  本例中，mem_block结果从alloc测试中继承，没有重新填充
	*/
	ioctl(fd,IOC_MEMPOOL_FREE,&(buf[1]));	
	
	/*
	  删除内存池：
	  传入参数为：内存池handle
	  用指针传入。
	*/
	ioctl(fd,IOC_MEMPOOL_DESTORY,&handle);
    	return 0;
}


