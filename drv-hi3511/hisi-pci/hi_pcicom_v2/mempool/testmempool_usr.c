#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "mempool_user.h"


/*
  �û�̬�ӿ�ʾ������
  ������ϣ���ο��ں�̬ʾ������
  �����У���˵���û�̬�ӿڵ�ʹ�÷�����
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

	/*���豸*/	
	fd = open("/dev/misc/mempool",O_RDWR);
	

	/*��ʼ��*/		
	//hios_mempool_init(0);
	handle = 0;
	/*�������Ϊ����ID����ָ�봫��*/
	ioctl(fd,IOC_MEMPOOL_INIT,&handle);
	printf("handle = %x\n",handle);
	//msleep(5000);

	/*�����ڴ�أ��������ڴ�����ԡ�
	  �ں�̬�ӿڣ�hios_mempool_create("mempool_host", 100*1024,1,128,0);
	  �����ڴ��������ڴ��ǰ���ڴ��Ϊ�գ��������ڷ����ڴ档
	  �������壺
	  1��name���ַ���������С��20
	  2���ڴ�صĿ��С��������ڴ��С��������롣���С�����4k�ֽڶ��롣
	  3���Զ�cpu_id�������У��Զ�Ϊ��cpu��idΪ1
	  4��port�š�ע�����Ӳ���ʹ����ͬ��port�š�
	  5�����ȼ������Ͷ���Ϣ�����ȼ���0�������ȼ���1�������ȼ������������жϡ�	
	*/	
	
	strcpy((char *)buf, pname);
	buf[5] = 100*1024;
	buf[6] = 0;
	buf[7] = 128;
	buf[8] = 0;
	
	/*
	  ���������ṹ
	  buf[0]~buf[4]:Ҫ�����ڴ�ص����֣���󳤶�Ϊ19�ֽڣ����'\0'
	  buf[5]:�ڴ�ؿ��С
	  buf[6]:Զ��target_id
	  buf[7]:port��
	  buf[8]:���Ͷ���Ϣ�����ȼ�
	*/
	ioctl(fd,IOC_MEMPOOL_CREATE,buf);
	/*
	  ����ֵ��
	  buf[0]:�ڴ��handle
	*/
	handle = buf[0];
	printf("create sucess! handle = %x\n",handle);
	
/*
   ����ڴ��
*/	
	memblk[0] = 0xC0000000;      //�ڴ���׵�ַ
	memblk[1] = memblk[0] + 0x100000; //�ڴ��β��ַ
	memblk[2] = 0xC2000000;           //�ڴ���׵�ַ
	memblk[3] = memblk[2] + 0x100000; //�ڴ��β��ַ
	
	/*��Զ��ڴ�����2���ڴ�顣�ܻ���buf���ƣ�һ��������10���ڴ��
	  �������壺
	  1���Զ��ڴ��handle����Ҫͨ������Ϣ���ա�����ʡ�Խ��յĲ��裬ֱ�Ӹ�ֵ��
	  2���ڴ����Ϣ�׵�ַ
	  3���ڴ��ĸ�����ע�����ڴ����Ϣbuf�ĳ��ȡ�һ������β��ַ�ԡ�����һ���ڴ�顣��������������ڴ�����Ϣ
	*/
	//i = hios_add_mempool(0x200001, memblk, 2);
	buf[0] = handle;
	buf[1] = 2;
	memcpy(&(buf[2]),memblk,2*8);
	/*
	   buf���ṹ��
	   buf[0]:�ڴ��handle
	   buf[1]:�ڴ�����
	   buf[2]~:�ڴ����ʼ������ַ��
	   ע��һ��������10���ڴ��
	   �����ʧ��ʱ��ioctl�ӿڷ��ط���ֵ��
	*/	
	if(!(ioctl(fd,IOC_MEMPOOL_ADD,buf)))
	{
		printf("IOC_MEMPOOL_ADD return error!\n");
	}	
	

	/*�����ڴ棬��������������10�顣����while(1)��Ϊ�˵ȴ���cpu����ڴ��
	  ����Ѿ�������ڴ�飬��ֱ�ӷ��䡣
	  �������壺
	  1��mmb�ṹ�����ڱ�������ڴ�����Ϣ�����ݸ�hios_mempool_alloc�ӿڣ������ͷš�
	              ����ͷŲ���������cpu�Ͻ��У���Ҫͨ������Ϣ��mmb���ݸ�����cpu
	  2���ڴ��handle�������У����ǵ�38�еõ���handle
	  3��Ҫ������ڴ��С��ʵ�ʷ���Ĵ�С�������ڴ�صĿ��С���롣
	  
	  ����ֵ��
	  addr ���ڴ����׵�ַ, �˵�ַ��ֱ�Ӵ��ݸ��Զ�ʹ�á�Ҳ�����ڶԶ�ͨ��char *hios_getmmb_addr(struct mem_block mmb);�ӿڻ�ȡ��
	         
	  NULL: ����ʧ�ܡ��������hios_type.h��
	*/
	while(1)
	{
		//addr = hios_mempool_alloc(&mmb[count], handle, 0x20000);
		buf[0] = handle;
		buf[1] = 0x20000;
		/*
		  �����ڴ棬buf���ṹ��
		  buf[0]:�ڴ��handle
		  buf[1]:�����ڴ��Ĵ�С
		  buf����Ҫ�����洢������Ϣ����СӦ��С��struct mem_block
		*/
		ioctl(fd,IOC_MEMPOOL_ALLOC,buf);
		/*
		   ����ֵ��
		   buf[0]:�����ڴ����׵�ַ
		   &buf[1]~:stuct mem_block�׵�ַ�������ͷ��ڴ档
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
	  �ͷ��ڴ�飺
	  buf���ṹ��
	  &buf[0]:struct mem_block�׵�ַ
	  �����У�mem_block�����alloc�����м̳У�û���������
	*/
	ioctl(fd,IOC_MEMPOOL_FREE,&(buf[1]));	
	
	/*
	  ɾ���ڴ�أ�
	  �������Ϊ���ڴ��handle
	  ��ָ�봫�롣
	*/
	ioctl(fd,IOC_MEMPOOL_DESTORY,&handle);
    	return 0;
}


