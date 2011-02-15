#ifndef __HIOS_MEMPOOL_H__
#define __HIOS_MEMPOOL_H__

#include "hios_type.h"

#define EERROR -1
#define OK      0

/*�˽ṹ���ڲ������ݣ��ɲ����ľ��庬��*/
struct mem_block{
        hios_u32 handle;
        hios_u32 addr;
        hios_u32 size;
};  

/*�����ڴ��*/
extern hios_u32 hios_mempool_create(
                        hios_char *name,    //����С��20�ֽ�
                        hios_u32 blk_sz,    //�ڴ�صĿ��С
                        hios_u32 remote_id, //�Զ�cpu id
                        hios_u32 port,      //���뷢�Ͷ���Ϣ��port��
                        hios_u32 priority   //����Ϣ�����ȼ�
                        );
                        
/*���ڴ��������ڴ��*/                        
extern hios_status hios_add_mempool(
                        hios_u32 handle,    //�ڴ��handle����hios_mempool_create�ӿڷ���
                        hios_u32 *mempool,  //�ڴ����Ϣ
                        hios_u32 count      //�ڴ��ĸ���
                        );
                        
/*�����ڴ�*/                        
extern hios_char *hios_mempool_alloc(
                        struct mem_block *mmb,  //������������ڱ�������ڴ�����Ϣ
                        hios_u32 handle,        //�ڴ��handle����hios_mempool_create�ӿڷ���
                        hios_u32 size           //�����ڴ�Ĵ�С
                        );

/*�ͷ��ڴ�*/
extern hios_void hios_mempool_free(struct mem_block mmb); //mmb�ṹ��hios_mempool_alloc�ӿ����

/*ɾ���ڴ��*/
extern hios_void hios_mempool_destory(hios_u32 handle);  //�ڴ��handle����hios_mempool_create�ӿڷ���

/*ͨ��mmb�ṹ����ȡ�����ڴ���׵�ַ*/
extern hios_char *hios_getmmb_addr(struct mem_block mmb);  //mmb�ṹ��hios_mempool_alloc�ӿ����

/*�ڴ�س�ʼ�������������ڴ��ǰ����Ҫ�ȵ��ô˽ӿ�*/
extern hios_void hios_mempool_init(hios_u32 cpu_id);  //local_cpu id


#define IOC_MEMPOOL_INIT		_IOWR('m', 0,  int)
#define IOC_GETMMB_ADDR			_IOWR('m', 1,  struct mem_block)
#define IOC_MEMPOOL_DESTORY		_IOWR('m', 2,  int)
#define IOC_MEMPOOL_FREE		_IOWR('m', 3,  struct mem_block)
#define IOC_MEMPOOL_ALLOC		_IOWR('m', 4,  struct mem_block)
#define IOC_MEMPOOL_ADD			_IOWR('m', 5,  struct mem_block)
#define IOC_MEMPOOL_CREATE		_IOWR('m', 6,  struct mem_block)

#endif
