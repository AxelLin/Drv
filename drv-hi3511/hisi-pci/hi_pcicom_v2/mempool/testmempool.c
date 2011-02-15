#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "mempool.h"
#include "hios_type.h"


/*
  ��cpu�ϵ�ʾ������
*/


static int __init testmempool_init(void)
{
	hios_u32 mempool[10];
	hios_s32 i,count=0;
	hios_u32 handle ,handle2;
	struct mem_block mmb[40];
	hios_char *addr=HIOS_NULL;

	printk("##testmempool host 1.0!\n");
	
	/*ʹ��ǰ�����ȳ�ʼ�����������Ϊlocal_id�����ڱ�ʶ��ͬ��CPU*/	
	hios_mempool_init(0);
	
	//msleep(5000);
	
	/*�����ڴ�أ��������ڴ�����ԡ�
	  �����ڴ��������ڴ��ǰ���ڴ��Ϊ�գ��������ڷ����ڴ档
	  �������壺
	  1��name���ַ���������С��20
	  2���ڴ�صĿ��С��������ڴ��С��������롣���С�����4k�ֽڶ��롣
	  3���Զ�cpu_id�������У��Զ�Ϊ��cpu��idΪ1
	  4��port�š�ע�����Ӳ���ʹ����ͬ��port�š�
	  5�����ȼ������Ͷ���Ϣ�����ȼ���0�������ȼ���1�������ȼ������������жϡ�	
	*/	
	handle = hios_mempool_create("mempool_host", 100*1024,1,128,0);
	
	printk("create sucess! handle = %x\n",handle);

	/*�����ڴ棬��������������10�顣����while(1)��Ϊ�˵ȴ���cpu����ڴ��
	  ����Ѿ�������ڴ�飬��ֱ�ӷ��䡣
	  �������壺
	  1��mmb�ṹ�����ڱ�������ڴ�����Ϣ�����ݸ�hios_mempool_alloc�ӿڣ������ͷš�
	              ����ͷŲ���������cpu�Ͻ��У���Ҫͨ������Ϣ��mmb���ݸ�����cpu
	  2���ڴ��handle�������У����ǵ�38�еõ���handle
	  3��Ҫ������ڴ��С��ʵ�ʷ���Ĵ�С�������ڴ�صĿ��С���롣
	  
	  ����ֵ��
	  addr ���ڴ����׵�ַ, �˵�ַ��ֱ�Ӵ��ݸ��Զ�ʹ�á�Ҳ�����ڶԶ�ͨ��hios_char *hios_getmmb_addr(struct mem_block mmb);�ӿڻ�ȡ��
	         
	  HIOS_NULL: ����ʧ�ܡ��������hios_type.h��
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
	/*�����е�delay������Ϊ�����cpu�ϵ���������ͬ��*/
	msleep(2000);
	
	printk("\n\n\n\n\n\n");
	
	
	/*��ʱ����cpu�Ѿ��ͷ��˼����ڴ�飬���·��䣬����֤�ͷŲ����Ƿ�ɹ���*/
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
	
	
	/*��ʱ����cpu�Ѿ�ɾ�����ڴ��
	  ִ�з������������֤�ڴ���Ƿ�ɾ���ɹ���
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

