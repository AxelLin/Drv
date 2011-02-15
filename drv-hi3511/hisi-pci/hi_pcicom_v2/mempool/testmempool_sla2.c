#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>


#include "mempool.h"
#include "hios_type.h"

/*�����ڴ�cpu�ϵ�ʾ������*/

static int __init testmempool_sla_init(void)
{
	hios_u32 mempool[10];
	hios_s32 i,count=0;
	hios_u32 handle ,handle2;
	struct mem_block mmb[40];
	hios_u32 memblk[40];
	hios_char *addr;

	printk("##testmempool sla 1.0!\n");
	
	/*ʹ��ǰ�����ȳ�ʼ�����������Ϊlocal_id�����ڱ�ʶ��ͬ��CPU*/
	hios_mempool_init(1);
	
	/*�����ڴ�أ��������ڴ�����ԡ�
	  �����ڴ��������ڴ��ǰ���ڴ��Ϊ�գ��������ڷ����ڴ档
	  �������壺
	  1��name���ַ���������С��20
	  2���ڴ�صĿ��С��������ڴ��С��������롣���С�����4k�ֽڶ��롣
	  3���Զ�cpu_id�������У��Զ�Ϊ��cpu��idΪ0
	  4��port�š�ע�����Ӳ���ʹ����ͬ��port�š�
	  5�����ȼ������Ͷ���Ϣ�����ȼ���0�������ȼ���1�������ȼ������������жϡ�	
	*/
	handle = hios_mempool_create("mempool_slave2", 10*1024,0,124,0);

	/*
	   �ڴ����ڴ�غ���Ҫ�Զ����ڴ��������ڴ�飬�˴���Ҫ��hanldeͨ������Ϣ���͵��Զˡ�	
	*/
        /*
           ����handle���Զ˵Ĳ����ԣ������з��͡�
        */


        /*��Զ��ڴ��������ڴ�飬�Զ��ڴ��handle��Ҫͨ����һ�����͹�����*/
	
	/*����ڴ����Ϣ*/
	msleep(500);
	memblk[0] = 0xB0000000;      //�ڴ���׵�ַ
	memblk[1] = memblk[0] + 0x100000; //�ڴ��β��ַ
	memblk[2] = 0xB2000000;           //�ڴ���׵�ַ
	memblk[3] = memblk[2] + 0x100000; //�ڴ��β��ַ
	
	/*��Զ��ڴ�����2���ڴ�顣�ܻ���buf���ƣ�һ��������10���ڴ��
	  �������壺
	  1���Զ��ڴ��handle����Ҫͨ������Ϣ���ա�����ʡ�Խ��յĲ��裬ֱ�Ӹ�ֵ��
	  2���ڴ����Ϣ�׵�ַ
	  3���ڴ��ĸ�����ע�����ڴ����Ϣbuf�ĳ��ȡ�һ������β��ַ�ԡ�����һ���ڴ�顣��������������ڴ�����Ϣ
	*/
	i = hios_add_mempool(0x200001, memblk, 2);
	if(i)//����ֵ���㣬��ʾ����
		printk("ERROR! i= %d\n",i);	


	/*�����е��ӳ٣�Ŀ����Ϊ�˺���cpu�ϵ���������ͬ��*/
	msleep(1000);


	/*�ͷ��ڴ�*/
	mmb[0].handle=0x200001;
	mmb[0].addr = 0xB0000000;
	mmb[0].size=0x32000;
	
	/*
	�ͷŶԶ��ڴ�
	�������壺
	mmb�ṹ��hios_mempool_alloc�ӿڷ���
	��������ɲο���cpu����������
	������û��mmb�Ĵ��Ͳ�����ֱ�ӹ�����mmb�ṹ��
	�ͷ�ʱ������Ҫ����mmb�ṹ�ľ������ݡ�		
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
	ɾ���ڴ�أ�������ɾ�����ػ��߶Զ˵��ڴ�أ�ͨ��handle���֡�
	�������壺
	�ڴ��handle
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

