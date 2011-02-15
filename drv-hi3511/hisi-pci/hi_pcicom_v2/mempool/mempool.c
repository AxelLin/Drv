#include <linux/module.h>
#include <linux/list.h>
#include <linux/string.h>
#include <asm/system.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include "mempool.h"
#include "hi_mcc_usrdev.h"


#define MIN_BLKSIZE 0x1000
#define MIN_FREESIZE 0x10000
#define MAX_ENTRY 50 //not large chan 64

#define DEBUG 0
#define TEST  0

//hios_mcc_handle_t *rev_handle;

struct mempool{
	hios_char name[20];
	hios_u32 id;
 	hios_u32 blk_sz;
 	hios_u32 remote_id;
 	hios_u32 port;
 	struct work_struct mmb_handle_task;
 	hios_u32 add_buf[22];
 	hios_mcc_handle_t *rev_handle;
	struct list_head mempool_list;
	struct list_head memblock_list;
};

struct memblock_info{
	hios_u32 start_addr;
	hios_u32 end_addr;
     	hios_u32 *free_mmb;
     	hios_u32 max_freesize;
     	hios_u32 head_num;
     	struct list_head mmb_list;
};

struct list_head g_mempool_listhead;
struct workqueue_struct *g_workqueue;


static unsigned int local_id = 0xFFFFFFFF;
//static unsigned int local_id = 0xF;

#if DEBUG
static void show_allentry(struct memblock_info *memblk);

static void show_freemmb(struct memblock_info *memblk)
{
	hios_u32 i,tmp;
	hios_u32 pos[MAX_ENTRY];
	hios_u32 j,count=0;
        i = memblk->head_num;
        do{
        	//printk("count=%d,i=%d\n",(int)count,(int)i);
        	pos[count]=i;
               	printk("(memblk->free_mmb)[%d]=%x,(memblk->free_mmb)[%d+1]=%x\n",(int)i,(unsigned int)(memblk->free_mmb)[i],(int)i, (unsigned int)(memblk->free_mmb)[i+1]);
               	tmp = (memblk->free_mmb)[i] & 0x3F;
               	if(tmp != 0x3F)
               	{
                       	i = tmp;
                       	count++;
                }
               	else
                       	break;
        }while(1);
        
        for(i=0;i<2*MAX_ENTRY;)
        {
        	tmp = 0;
        	for(j=0;j<=count;j++)
        	{
        		if(pos[j]==i)
        			tmp=1;
        	}
        	if(!tmp)
        	{        		
        		if((memblk->free_mmb)[i] != 0xFFFFFFFF)
        		{
        			printk("dump free_mmb! count=%d, i=%d\n",(int)count,(int)i);
        			show_allentry(memblk);
        			break;
        		}
        	}
        	i = i+2;	
        }	
}

static void show_allentry(struct memblock_info *memblk)
{
	hios_u32 j;
	printk("memblk->max_freesize=0x%x\n",(unsigned int)memblk->max_freesize);
	for(j=0;j<MAX_ENTRY;j++)
	{
		printk("%d : (memblk->free_mmb)[i]=0x%x,memblk->free_mmb)[i+1]=0x%x\n",(int)j,(unsigned int)(memblk->free_mmb)[j],(unsigned int)(memblk->free_mmb)[j+1]);
		j++;
	}
}

static void show_memblk(struct memblock_info *memblk)
{
	printk("******\nmemblk->start_addr=0x%x,memblk->end_addr=0x%x\n",(unsigned int)memblk->start_addr,(unsigned int)memblk->end_addr);
	printk("memblk->head_num=%d,memblk->max_freesize=0x%x\n",(int)memblk->head_num,(unsigned int)memblk->max_freesize);
	show_freemmb(memblk);
	printk("******\n");
}
#endif



#if TEST

static void prepare_freemmb(struct memblock_info *memblk)
{
	memset((hios_char *)memblk->free_mmb, 0xFF, MAX_ENTRY*2*sizeof(hios_u32));
#if 0	
	memblk->free_mmb[0] = 0xC020013F;
	memblk->free_mmb[1] = 0x190000;
	memblk->free_mmb[4] = 0xC0100FC0;
	memblk->free_mmb[5] = 0x7D000;
	memblk->max_freesize = 0x190000;
	memblk->head_num = 4;
#endif

#if 1	
	memblk->free_mmb[0] = 0xC0032594;
	memblk->free_mmb[1] = 0x19000*8;
	memblk->free_mmb[6] = 0xC0177216;
	memblk->free_mmb[7] = 0x19000*3;
	memblk->free_mmb[8] = 0xC020DFC6;
	memblk->free_mmb[9] = 0x19000*2;	
	memblk->free_mmb[20] = 0xC025803F;
	memblk->free_mmb[21] = 0x19000*10;	
	memblk->free_mmb[22] = 0xC03E8180;
	memblk->free_mmb[23] = 0x19000*4;			
	memblk->max_freesize = 0x19000*10;
	memblk->head_num = 8;
#endif

#if 0	
	memblk->free_mmb[0] = 0xC0032594;
	memblk->free_mmb[1] = 0x19000*4;
	memblk->free_mmb[6] = 0xC0177216;
	memblk->free_mmb[7] = 0x19000*2;
	memblk->free_mmb[8] = 0xC020DFC6;
	memblk->free_mmb[9] = 0x19000*1;	
	memblk->free_mmb[20] = 0xC025803F;
	memblk->free_mmb[21] = 0x19000*10;	
	memblk->free_mmb[22] = 0xC03E8180;
	memblk->free_mmb[23] = 0x19000*3;			
	memblk->max_freesize = 0x19000*10;
	memblk->head_num = 8;
#endif


}

#endif

static void hebing(struct memblock_info *memblk, hios_u32 i)
{
	hios_u32 j,tmp,tmp1;

head_node:
	
	if(i == memblk->head_num)
	{
#if DEBUG
		printk("i is head node!\n");
#endif	

		if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
		{
#if DEBUG
			printk("only one node!\n");
#endif			
			//only one node!
			return;
		}
		j = (memblk->free_mmb)[i] & 0x3F;
		while(1)
		{
			if(j == 0x3F)
				break;						
			if((((memblk->free_mmb)[i] & 0xFFFFF000) + (memblk->free_mmb)[i+1]) == ((memblk->free_mmb)[j] & 0xFFFFF000))
			{
#if DEBUG
				printk("hebing with %d node!,modify size!\n",(int)j);
#endif					
				if(((memblk->free_mmb)[j] & 0x3F) == 0x3F)
				{
					if(j == ((memblk->free_mmb)[i] & 0x3F))
					{
						(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];	
						(memblk->free_mmb)[j] = 0xFFFFFFFF;
						(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
						(memblk->free_mmb)[i] |= 0x3F;
						memblk->max_freesize = (memblk->free_mmb)[i+1];
						return;
					}
					
					tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
					(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0x03F;
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					memblk->max_freesize = (memblk->free_mmb)[i+1];
					
					j = (memblk->free_mmb)[i] & 0x3F;
					continue;					
				}
				//j is middle node!
				if(j == ((memblk->free_mmb)[i] & 0x3F))
				{
					tmp = (memblk->free_mmb)[j] & 0x3F;
									
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];	
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFFFC0) | tmp;
					(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i << 6);
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					j = (memblk->free_mmb)[i] & 0x3F;										
					continue;
				}
				
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
				tmp1 = (memblk->free_mmb)[j] & 0x3F;
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
				(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (tmp << 6);
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				j = (memblk->free_mmb)[i] & 0x3F;
				continue;				
			}	
			
			if(((memblk->free_mmb)[i] & 0xFFFFF000) == (((memblk->free_mmb)[j] & 0xFFFFF000) + (memblk->free_mmb)[j+1]))
			{
#if DEBUG
				printk("hebing with %d node!modify size and addr!\n",(int)j);
#endif					
				if(((memblk->free_mmb)[j] & 0x3F) == 0x3F)
				{
					if(j == ((memblk->free_mmb)[i] & 0x3F))
					{
						(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
						(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
						(memblk->free_mmb)[j] = 0xFFFFFFFF;
						(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
						(memblk->free_mmb)[i] |= 0x3F;
						memblk->max_freesize = (memblk->free_mmb)[i+1];
						return;
					}
					
					tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
					(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0x03F;
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					memblk->max_freesize = (memblk->free_mmb)[tmp+1];
					
					j = (memblk->free_mmb)[i] & 0x3F;
					continue;					
				}
				//j is middle node!
				if(j == ((memblk->free_mmb)[i] & 0x3F))
				{
					tmp = (memblk->free_mmb)[j] & 0x3F;
									
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);	
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFFFC0) | tmp;
					(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i << 6);
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					j = (memblk->free_mmb)[i] & 0x3F;										
					continue;
				}
				
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
				tmp1 = (memblk->free_mmb)[j] & 0x3F;
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
				(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (tmp << 6);
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				j = (memblk->free_mmb)[i] & 0x3F;
				continue;						
			}
			
			j = (memblk->free_mmb)[j] & 0x3F;			
		}
		return; 
	}

tail_node:	
	if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
	{
#if DEBUG
		printk("i is tail node!\n");
#endif		
		//i is the tail node
		j = memblk->head_num;
		while(1)
		{			
			if(i == j)
			{
				return;
			}
		        if((((memblk->free_mmb)[i] & 0xFFFFF000) + (memblk->free_mmb)[i+1]) == ((memblk->free_mmb)[j] & 0xFFFFF000))	
			{
#if DEBUG
				printk("hebing with %d node!, modify size!\n",(int)j);
#endif						
				if(j == memblk->head_num)
				{
					tmp = (memblk->free_mmb)[j] & 0x3F;
					if(tmp == i)
					{
						//del head node 
						(memblk->free_mmb)[i] |= 0xFFF;
						(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j+1];
						(memblk->free_mmb)[j] = 0xFFFFFFFF;
						(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
						memblk->max_freesize = (memblk->free_mmb)[i+1];
						return;
					}
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j+1];
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					memblk->max_freesize = (memblk->free_mmb)[i+1];
					(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0xFC0;
					
					j = tmp;
					continue;
				}
				
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
				tmp1 = (memblk->free_mmb)[j] & 0x3F;
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
				(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (tmp << 6);
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				j = memblk->head_num;
				continue;
			}

		        if(((memblk->free_mmb)[i] & 0xFFFFF000) == (((memblk->free_mmb)[j] & 0xFFFFF000) + (memblk->free_mmb)[j+1]))	
			{
#if DEBUG
				printk("hebing with %d node!, modify size and addr!\n",(int)j);
#endif						
				if(j == memblk->head_num)
				{
					tmp = (memblk->free_mmb)[j] & 0x3F;
					if(tmp == i)
					{
						//del head node 
						(memblk->free_mmb)[i] |= 0xFFF;
						(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j+1];
						(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
						(memblk->free_mmb)[j] = 0xFFFFFFFF;
						(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
						memblk->max_freesize = (memblk->free_mmb)[i+1];
						return;
					}
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j+1];
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					memblk->max_freesize = (memblk->free_mmb)[i+1];
					(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0xFC0;
					
					j = tmp;
					continue;
				}
				
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
				tmp1 = (memblk->free_mmb)[j] & 0x3F;
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
				(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (tmp << 6);
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				j = memblk->head_num;
				continue;
			}
			
			j = (memblk->free_mmb)[j] & 0x3F;				
		}
		
	}
	j = memblk->head_num;
	while(1)
	{
#if DEBUG
		printk("i is middle node!\n");
#endif		
		if(j == 0x3F)
			return;
		if((((memblk->free_mmb)[i] & 0xFFFFF000) + (memblk->free_mmb)[i+1]) == ((memblk->free_mmb)[j] & 0xFFFFF000))
		{
#if DEBUG
			printk("hebing with %d node!, modify size!\n",(int)j);
#endif					
			if(j == memblk->head_num)
			{
				tmp = (memblk->free_mmb)[memblk->head_num] & 0x3F;
				(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0xFC0;
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				memblk->head_num = tmp;
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				if(tmp == i)
				{
					goto head_node;
				}
				j = memblk->head_num;
				continue;
			}
			
			if(((memblk->free_mmb)[j] & 0x3F) == 0x3F)
			{
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
				if(tmp == i)
				{
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
					(memblk->free_mmb)[i] |= 0x3F;
					memblk->max_freesize = (memblk->free_mmb)[i+1];
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					goto tail_node; 
				}
				(memblk->free_mmb)[tmp] |= 0x3F;
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				memblk->max_freesize = (memblk->free_mmb)[tmp+1];
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				j = memblk->head_num;
				continue;
			}
			
			tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
			tmp1 = (memblk->free_mmb)[j] & 0x3F;	
			(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
			(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (tmp << 6);
			(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
			(memblk->free_mmb)[j] = 0xFFFFFFFF;
			(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
			j = memblk->head_num;
			continue;		
		}
		
		if(((memblk->free_mmb)[i] & 0xFFFFF000) == (((memblk->free_mmb)[j] & 0xFFFFF000) + (memblk->free_mmb)[j+1]))
		{
#if DEBUG
			printk("hebing with %d node! modify size and addr!\n",(int)j);
#endif					
			if(j == memblk->head_num)
			{
				tmp = (memblk->free_mmb)[memblk->head_num] & 0x3F;
				(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0xFC0;
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
				memblk->head_num = tmp;
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				if(tmp == i)
				{
					goto head_node;
				}
				j = memblk->head_num;
				continue;
			}
			
			if(((memblk->free_mmb)[j] & 0x3F) == 0x3F)
			{
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
				if(tmp == i)
				{
					(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
					(memblk->free_mmb)[i] |= 0x3F;
					memblk->max_freesize = (memblk->free_mmb)[i+1];
					(memblk->free_mmb)[j] = 0xFFFFFFFF;
					(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
					goto tail_node; 
				}
				(memblk->free_mmb)[tmp] |= 0x3F;
				(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
				memblk->max_freesize = (memblk->free_mmb)[tmp+1];
				(memblk->free_mmb)[j] = 0xFFFFFFFF;
				(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
				j = memblk->head_num;
				continue;
			}
			
			tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;
			tmp1 = (memblk->free_mmb)[j] & 0x3F;	
			(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
			(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (tmp << 6);
			(memblk->free_mmb)[i+1] += (memblk->free_mmb)[j + 1];
			(memblk->free_mmb)[i] = ((memblk->free_mmb)[j] & 0xFFFFF000) | ((memblk->free_mmb)[i] & 0xFFF);
			(memblk->free_mmb)[j] = 0xFFFFFFFF;
			(memblk->free_mmb)[j+1] = 0xFFFFFFFF;
			j = memblk->head_num;
			continue;		
		}		
		
		j = (memblk->free_mmb)[j] & 0x3F;		
	}
}


#if DEBUG

hios_void dump_buf(hios_u32 *tr_handle,hios_u32 *buf,hios_u32 len)
{
	hios_s32 i;
	printk("tr_handle = 0x%x\n", (unsigned int)tr_handle);
	for(i=0;i<len/4;i++)
	{
		printk("0x%x   0x%x\n",(unsigned int)buf[i],(unsigned int)buf[i+1]);
		i++;	
	}
}

#endif


void mmb_task(struct mempool *mempool)
{
	/*
	hios_status hios_add_mempool(
				hios_u32 handle,
				hios_u32 *mempool,
				hios_u32 count
				)	
				
	int hios_mcc_sendto(hios_mcc_handle_t *handle, const void *buf, unsigned int len);			
	*/	
	
	hios_status ret;

#if DEBUG
	printk("host : mmb_task!\n");
	printk("mempool = 0x%x \n",(unsigned int)mempool);
#endif	
	
	if(mempool->add_buf[1] == 0xFFFFFFFF)
	{
		//distory mempool
		hios_mempool_destory(mempool->add_buf[0]);
	}
	else	
	{
#if DEBUG
		printk("mmb_task : add_mmepool!\n");
		dump_buf(&mempool->add_buf[0], &mempool->add_buf[2], mempool->add_buf[1]*8);
#endif			
		ret = hios_add_mempool(mempool->add_buf[0],&mempool->add_buf[2],mempool->add_buf[1]);
		hios_mcc_sendto((hios_mcc_handle_t *)(mempool->rev_handle), &ret, sizeof(hios_status)); 
	}
} 


hios_s32 recvfrom_notify(void *handle, void *buf, unsigned int data_len)
{
	hios_u32 mempool_handle;
	hios_u32 id;	
	struct mempool *p_mempool;
	struct mem_block mmb;

#if DEBUG
	printk("host : rev msg!\n");
	dump_buf(handle, buf, data_len);
#endif
	
	if(data_len != sizeof(struct mem_block))
	{
		//add_mempool
		mempool_handle = *((hios_u32 *)buf);
		
		id = mempool_handle & 0x3FFF;
	
		list_for_each_entry(p_mempool, &g_mempool_listhead, mempool_list)
	        {
	        	 if(id == p_mempool->id)
				break;
		}
			
		if(id != p_mempool->id)
			return 0;

#if DEBUG
		printk("host : shedule mem_task!\n");
	//	printk("&g_workqueue = 0x%x, &(p_mempool->mmb_handle_task) = 0x%x\n",(unsigned int)&g_workqueue, (unsigned int)&(p_mempool->mmb_handle_task));		
	//	printk("mmb_handle_task info:\n");
	//	printk("func : 0x%x\n",(unsigned int)(&p_mempool->mmb_handle_task)->func);
	//	printk("data : 0x%x\n",(unsigned int)(&p_mempool->mmb_handle_task)->data);
	//	printk("wq_data : 0x%x\n",(unsigned int)(&p_mempool->mmb_handle_task)->wq_data);
	//	printk("timer : 0x%x\n",(unsigned int)&((&p_mempool->mmb_handle_task)->timer));
#endif
		
		memcpy(p_mempool->add_buf, buf, data_len);
		queue_work(g_workqueue, &p_mempool->mmb_handle_task);
		return data_len;
	}
	
	//free mmb
	memcpy(&mmb, buf, data_len);
	hios_mempool_free(mmb);
	return 	data_len;
}

hios_u32 hios_mempool_create(
			hios_char *name,
   			hios_u32 blk_sz,
                        hios_u32 remote_id,
                        hios_u32 port,
                        hios_u32 priority   			
			)
{
	hios_u32 handle;
	//struct list_head * pos;
	hios_u32 id=1;
	struct mempool *mempool = HIOS_NULL;
	struct hi_mcc_handle_attr attr;
	hios_mcc_handle_opt_t opt;

	if(local_id == 0xFFFFFFFF)
	{
		printk("ERROR! mempool has not been inited!\n");
		return 0;
	}	

	if(local_id == remote_id)
	{
		printk("ERROR! remote_id is same with local_id!\n");
		return 0;
	}

	if(port > 1023)
	{
		printk("ERROR! port is great chan  1023!\n");
		return 0;
	}

	if(sizeof(name) > 20)
	{
		printk("WARNING! mempool name '%s' is too long, \
                        may be cuted!\n",name);
	}

	if(blk_sz < MIN_BLKSIZE)
	{
		blk_sz = MIN_BLKSIZE;
		printk("WARNING! mempool '%s' has a too small block size, \
			modified to MIN_BLKSIZE=0x1000\n",name);
	}	


	if(blk_sz & (MIN_BLKSIZE-1))
	{
		blk_sz = (blk_sz+(MIN_BLKSIZE-1)) & (~(0xFFF));
#if DEBUG		
		printk("WARNING! mempool '%s' blk_sz should be align to 0xFFF.\
modified to 0x%x \n", name, (unsigned int)blk_sz);
#endif
	}

#if DEBUG
	if(!(list_empty(&g_mempool_listhead)))
        {
		printk("mempool_list: \n");
          	list_for_each_entry(mempool, &g_mempool_listhead, mempool_list)
               	{
                       	id = mempool->id;
                      	printk("%s, id=%d\n", mempool->name, (unsigned int)id);
               	}
        }
	else
		printk("mempool list is empty!\n");
#endif

	if(!(list_empty(&g_mempool_listhead)))
	{
		list_for_each_entry(mempool, &g_mempool_listhead, mempool_list)
		{
			id = mempool->id;
			if(!strncmp(mempool->name, name, 19))
			{
				printk("ERROR, mempool '%s' is exist! \n",name);
				return -EERROR;
			}
		}
		id ++;	
		mempool = HIOS_NULL;
	}

	if((id & 0xFFFFC000) != 0)
	{
		printk("ERROR! too many mempool!\n");
		return 0;
	}

	mempool = (struct mempool *)kmalloc(sizeof(struct mempool), GFP_KERNEL);	
	if(mempool == HIOS_NULL)
	{
		printk("ERROR, struct mempool malloc failed!\n");
		return -EERROR;
	}

	if(strlen(name) < 20)
	{
		strcpy(mempool->name, name);
	}
	else
	{
		strncpy(mempool->name, name, 19);
		mempool->name[19] = '\0';
	}

	mempool->blk_sz = blk_sz;
	mempool->id = id;
	mempool->remote_id = remote_id;
	mempool->port = port;
	handle = (local_id & 0xFF) << 24;
	handle = handle | (port << 14); 
	handle += mempool->id;
	INIT_LIST_HEAD(&(mempool->mempool_list));
	INIT_LIST_HEAD(&(mempool->memblock_list));
	list_add_tail(&(mempool->mempool_list), &g_mempool_listhead);
	
	attr.target_id = remote_id;
	attr.port = port;
	attr.priority = priority;
	mempool->rev_handle = hios_mcc_open(&attr);
	if(!mempool->rev_handle)
	{
		printk("ERROR! open msg slot failed! del mempool '%s' \n",name);
		list_del(&(mempool->mempool_list));
		kfree(mempool);
		return 0;
	}
	
	opt.recvfrom_notify = (int (*)(void *, void *, unsigned int))recvfrom_notify;
	hios_mcc_setopt(mempool->rev_handle, &opt);
	
	INIT_WORK(&mempool->mmb_handle_task, (void (*)(void *))mmb_task, mempool);
	
#if DEBUG	
	printk("create mempool success! name=%s,id=%d,handle=0x%x mempool = 0x%x\n\n\n\n\n",mempool->name,(unsigned int)mempool->id,(unsigned int )handle, (unsigned int )mempool);	
#endif
	return handle;
}
EXPORT_SYMBOL(hios_mempool_create);


hios_s32 add_mempool_ack(void *handle, void *buf, unsigned int data_len)
{
	hios_u32 ack;
	hios_mcc_handle_opt_t opt;
	hios_mcc_handle_t tr_handle;

#if DEBUG
	printk("add_mempool_ack!\n");
#endif
	
	tr_handle.handle = (unsigned long)handle;
	
	ack = *((hios_u32 *)buf);
	hios_mcc_getopt(&tr_handle, &opt);
	opt.data = ack+2;
	
	hios_mcc_setopt(&tr_handle, &opt);	
	return data_len;
};




hios_status hios_add_mempool(
			hios_u32 handle,
			hios_u32 *mempool,
			hios_u32 count
			)
{
	hios_u32 id;
	hios_u32 i,j;
	//hios_status ret;
	struct mempool *p_mempool;
	struct memblock_info *mmb;	

	if(local_id == 0xFFFFFFFF)
	{
		printk("ERROR! mempool has not been inited!\n");
		return -EERROR;
	}
	
	if(handle == 0 || mempool == HIOS_NULL || count ==0)
	{
		printk("ERROR! in hios_add_mempool, param is invalid!\n");
		return -EERROR;
	}
	
	if(count > 10)
	{
		printk("ERROR! too meny memblock! add 10 block to mempool one times at most!\n");
		return -EERROR;
	}
	
	if((handle >> 24) != local_id)
	{
		struct hi_mcc_handle_attr attr;
		hios_mcc_handle_t *tr_handle=HIOS_NULL;
		hios_char *buf = HIOS_NULL;
		hios_u32 *p;
		hios_s32 ret;
		hios_mcc_handle_opt_t opt;
		
		attr.target_id = (handle >> 24);
		attr.port = (handle >> 14) & 0x3FF;
		attr.priority = 0;
		
		tr_handle = hios_mcc_open(&attr);
		if(tr_handle == HIOS_NULL)
			return -EERROR;
		
		opt.recvfrom_notify = (int (*)(void *, void *, unsigned int))add_mempool_ack;
		opt.data = 0;
		hios_mcc_setopt(tr_handle, &opt);		
		
		buf = kmalloc(sizeof(hios_u32)*2 + sizeof(hios_u32)*2*count, GFP_KERNEL);
		if(!buf)
		{
			printk("ERROR! system is lack of mem!\n");
			return -EERROR;
		}
		
		p = (hios_u32 *)buf;
		*p++ = handle;
		*p++ = count;
		memcpy(p,mempool,sizeof(hios_u32)*2*count);

#if DEBUG
		printk("add memblock ! send msg to romote_cpu!\n");
		dump_buf((hios_u32 *)tr_handle, (hios_u32 *)buf,(hios_u32)(sizeof(hios_u32)*2 + sizeof(hios_u32)*2*count));
#endif		
		ret = hios_mcc_sendto(tr_handle, buf, sizeof(hios_u32)*2 + sizeof(hios_u32)*2*count);
		if(ret < 0)
		{
			printk("ERROR! send msg failed!\n");
			kfree(buf);
			return -EERROR;
		}
		
		kfree(buf);
		
		while(1)
		{
			hios_mcc_getopt(tr_handle, &opt);
			if(opt.data != 0)
			{
				if(opt.data == 1)
				{
					hios_mcc_close(tr_handle);
					return -EERROR;
				}
				else
				{
					hios_mcc_close(tr_handle);
					return OK;
				}
			}
			msleep(20);
		}
		
		//int hios_mcc_sendto(hios_mcc_handle_t *handle, const void *buf, unsigned int len);	
	}	

	id = handle & 0x3FFF;

	list_for_each_entry(p_mempool, &g_mempool_listhead, mempool_list)
        {
        	 if(id == p_mempool->id)
			break;
	}
	
#if DEBUG
	printk("add memblock to mempool id=%d p_mempool->id=%d\n",(int)id,(int)p_mempool->id);
#endif
	
	if(id != p_mempool->id)
	{
		printk("ERROR! handle is invalid!\n");
		return -EERROR;
	}

	for(i=0;i<count;i++)
	{
		if(mempool[i*2] >= mempool[i*2 +1])
		{
			printk("ERROR! mempool start_addr is big chan end_addr!\n");
			return -EERROR;
		}
		
		for(j=0;j<count;j++)
		{
			if(i==j)
				continue;
			if((mempool[j*2] >= mempool[i*2]) && (mempool[j*2] <= mempool[i*2+1]))
			{
				printk("ERROR! memblock overlap!\n");
				return -EERROR;
			}

                        if((mempool[j*2+1] >= mempool[i*2]) && (mempool[j*2+1] <= mempool[i*2+1]))
                        {
                                printk("ERROR! memblock overlap!\n");
                                return -EERROR;
                        }
		}

	}

        if(!(list_empty(&(p_mempool->memblock_list))))
        {
                list_for_each_entry(mmb, &(p_mempool->memblock_list), mmb_list)
                {
			for(i=0; i<count ;i++)
			{
	                        if((mempool[i*2] >= mmb->start_addr) && (mempool[i*2] <= mmb->end_addr))
        	                {
                	                printk("ERROR! memblock overlap!\n");
                        	        return -EERROR;
                        	}

                                if((mempool[i*2+1] >= mmb->start_addr) && (mempool[i*2+1] <= mmb->end_addr))
                                {
                                        printk("ERROR! memblock overlap!\n");
                                        return -EERROR;
                                }
			}		
                }
        }
	
#if DEBUG
	printk("param check ok!\n");
#endif
	
	mmb = HIOS_NULL;
	for(i=0;i<count;i++)
	{
		mmb = (struct memblock_info *)kmalloc(sizeof(struct memblock_info) + MAX_ENTRY*2*sizeof(hios_u32), GFP_KERNEL);
		if(!mmb)
		{
			printk("ERROR! mmb alloc failed!\n");
			return -EERROR;
		}	
		
		//mmb->start_addr = ((mempool[i*2] + (p_mempool->blk_sz -1))/p_mempool->blk_sz)*p_mempool->blk_sz;
		//mmb->end_addr = ((mempool[i*2 + 1] + (p_mempool->blk_sz -1))/p_mempool->blk_sz)*p_mempool->blk_sz;
		mmb->start_addr = mempool[i*2];
		mmb->end_addr = mempool[i*2 + 1];
		mmb->free_mmb = (hios_u32 *)((char *)mmb + sizeof(struct memblock_info));
		memset((hios_char *)mmb->free_mmb, 0xFF, MAX_ENTRY*2*sizeof(hios_u32));
		mmb->max_freesize = mmb->end_addr - mmb->start_addr;

#if DEBUG		
		printk("mmb->start_addr = 0x%x, mmb->end_addr=0x%x \n",(unsigned int)mmb->start_addr,(unsigned int)mmb->end_addr);
#endif

		if(mmb->max_freesize < p_mempool->blk_sz)
		{
			kfree(mmb);
			printk("memblock size is too small, del!\n");
			continue;
		}		
		mmb->free_mmb[0] = mmb->start_addr + 0xFFF;
		mmb->free_mmb[1] = mmb->max_freesize;
		mmb->head_num = 0;	
		INIT_LIST_HEAD(&(mmb->mmb_list));
		list_add_tail(&(mmb->mmb_list), &(p_mempool->memblock_list));
#if DEBUG		
		printk("add memblock sucess! start_addr=%x, size=%x\n",(unsigned int)mmb->start_addr,(unsigned int)mmb->max_freesize);
#endif
	}
	
#if DEBUG
        if(!(list_empty(&(p_mempool->memblock_list))))
        {
		hios_s32 i=1;
		printk("memblock list:\n");
                list_for_each_entry(mmb, &(p_mempool->memblock_list), mmb_list)
                {
			printk("%d : start_addr=0x%x, end_addr=%x,",(int)i,(unsigned int)mmb->start_addr,(unsigned int)mmb->end_addr);
			printk("free_mmb[0] = 0x%x, free_mmb[1] = 0x%x \n",(unsigned int)mmb->free_mmb[0],(unsigned int)mmb->free_mmb[1]);	
    			//if(i==1)
    			//	show_allentry(mmb);            	
                	i++;
                	
		}
        }
	else
		printk("memblock is empty!\n");
	
#endif	
	return OK;	
}
EXPORT_SYMBOL(hios_add_mempool);


hios_char *hios_mempool_alloc(
			struct mem_block *mmb,
			hios_u32 handle,
			hios_u32 size
			)
{
	hios_u32 id;
	hios_char *addr=HIOS_NULL;
	hios_u32 flags;
	hios_u32 i,tmp,tmp1,tmp2,tmp3;
        struct mempool *p_mempool;
        struct memblock_info *memblk;


	if((mmb == HIOS_NULL) || (handle == 0) || size == 0)
	{
		printk("ERROR! hios_mempool_alloc param invalid!\n");
		return HIOS_NULL;
	}	

	if((handle >> 24) != local_id)
	{
		printk("ERROR! hios_mempool_alloc handle is not match with local_id!\n");
		return HIOS_NULL;
	}

        id = handle & 0x3FFF;

        list_for_each_entry(p_mempool, &g_mempool_listhead, mempool_list)
        {
                 if(id == p_mempool->id)
                        break;
        }

        if(id != p_mempool->id)
        {
                printk("ERROR! handle is invalid!\n");
                return HIOS_NULL;
        }
#if DEBUG
	printk("(p_mempool->blk_sz)=0x%x, size=0x%x, ((size + (p_mempool->blk_sz - 1)) / (p_mempool->blk_sz))=0x%x\n",(unsigned int)(p_mempool->blk_sz),(unsigned int)size,(unsigned int)((size + (p_mempool->blk_sz - 1)) / (p_mempool->blk_sz)));
#endif
	size = ((size + (p_mempool->blk_sz - 1)) / (p_mempool->blk_sz))*(p_mempool->blk_sz);

#if DEBUG
	printk("alloc mem form mempool%d, size=0x%x\n",(int)id,(unsigned int)size);
#endif	

	list_for_each_entry(memblk, &(p_mempool->memblock_list), mmb_list)
	{

#if DEBUG
		printk("find memblock!, curmmb_maxfreesize=0x%x, size=0x%x\n",(unsigned int)memblk->max_freesize,(unsigned int)size);
#endif

		if(size > memblk->max_freesize)
			continue;
		local_irq_save(flags);
		
#if TEST
		prepare_freemmb(memblk);
#endif	
		
#if DEBUG
        	printk("before alloc free_mmb:\n");
		show_memblk(memblk);
#endif
		
		i = memblk->head_num;
		while((memblk->free_mmb)[i+1]<size){
			i = (memblk->free_mmb)[i] & 0x3F;
			if(i==0x3F)
				break;
		}
		
		if(i == 0x3F)
		{
			printk("ERROR! hios_mempool_alloc fatal error!\n");
			local_irq_restore(flags);
			return HIOS_NULL;
		}
		
#if DEBUG
		printk("(memblk->free_mmb)[i+1] - size=0x%x\n", (unsigned int)((memblk->free_mmb)[i+1]-size));
#endif
		if((memblk->free_mmb)[i+1] - size < MIN_FREESIZE)
		{
			//need to del node
			addr = (hios_char *)((memblk->free_mmb)[i] & 0xFFFFF000);
			size = (memblk->free_mmb)[i+1];
#if DEBUG
        		printk("leave space is too small!,del node\n");
			show_memblk(memblk);
#endif


			if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
			{
				//i is the trail node
				if((((memblk->free_mmb)[i] >> 6) & 0x3F) == 0x3F)
				{
					//only one node
					(memblk->free_mmb)[i] = 0xFFFFFFFF;
					(memblk->free_mmb)[i+1] = 0xFFFFFFFF;
					memblk->max_freesize = 0;
#if DEBUG
        				printk("onle one node!\n");
        				show_memblk(memblk);
#endif

				}
				else
				{
					//del trail node
					tmp = (memblk->free_mmb)[i] >> 6 & 0x3F; 
					(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0x3F;
					(memblk->free_mmb)[i] = 0xFFFFFFFF;
					(memblk->free_mmb)[i+1] = 0xFFFFFFFF;
					memblk->max_freesize = (memblk->free_mmb)[tmp + 1];
#if DEBUG
        				printk("del tail node!\n");
        				show_memblk(memblk);
#endif
				}
			}
			else if((((memblk->free_mmb)[i] >> 6) & 0x3F) == 0x3F)
			{
				//i is the head node, del head node
				tmp = (memblk->free_mmb)[i] & 0x3F;
				(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0xFC0;
				memblk->head_num = tmp;
				(memblk->free_mmb)[i] = 0xFFFFFFFF;
				(memblk->free_mmb)[i+1] = 0xFFFFFFFF;
#if DEBUG
        			printk("del head node!\n");
        			show_memblk(memblk);
#endif

			}
			else
			{
				//i is a middle node
				tmp = (memblk->free_mmb)[i] & 0x3F;  //next node;
				tmp1 = ((memblk->free_mmb)[i] >> 6) & 0x3F; //front node;
				(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | tmp;
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (tmp1<<6);
				(memblk->free_mmb)[i] = 0xFFFFFFFF;
				(memblk->free_mmb)[i+1] = 0xFFFFFFFF;
#if DEBUG
     				printk("del middle node!\n");
        			show_memblk(memblk);
#endif

			}
			local_irq_restore(flags);
			break;
		}
		else  //not need to del node
		{
			addr = (hios_char *)((memblk->free_mmb)[i] & 0xFFFFF000);
			(memblk->free_mmb)[i + 1] -= size;
			(memblk->free_mmb)[i] += size; 
#if DEBUG
        		printk("modify the node size!\n");
        		printk("addr=0x%x, size=0x%x\n",(unsigned int)addr,(unsigned int)size);
#endif

			if((((memblk->free_mmb)[i] >> 6) & 0x3F) == 0x3F)
			{
				//i is head node
#if DEBUG
        			printk("modify head node!\n");
        			show_memblk(memblk);
#endif
				if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
					memblk->max_freesize = (memblk->free_mmb)[i + 1];
				local_irq_restore(flags);
				break;	
			} //head node

			if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
			{
#if DEBUG
        			printk("modify tail node,resort the list!\n");
#endif
				//i is trail node
				tmp = memblk->head_num;
				while((memblk->free_mmb)[tmp + 1] < (memblk->free_mmb)[i + 1])
				{
					tmp = (memblk->free_mmb)[tmp] & 0x3F;
					if((tmp == 0x3F) | (tmp == i))
						break;
				}
#if DEBUG
        			printk("insert before node %d! (memblk->free_mmb)[tmp + 1]=0x%x\n",(int)tmp,(unsigned int)(memblk->free_mmb)[tmp + 1]);
#endif

				if(tmp == memblk->head_num)
				{
					//i will be the head node
					tmp = ((memblk->free_mmb)[i] >> 6) & 0x3F; 
					(memblk->free_mmb)[tmp] = (memblk->free_mmb)[tmp] | 0x3F;
					memblk->max_freesize = (memblk->free_mmb)[tmp + 1];
					(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | \
											 memblk->head_num;
					(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | 0xFC0;
					(memblk->free_mmb)[memblk->head_num] = \
							((memblk->free_mmb)[memblk->head_num] & 0xFFFFF03F) | (i<<6);
					memblk->head_num = i;
#if DEBUG
					printk("to be the head node!\n");
					show_memblk(memblk);
#endif
					
					local_irq_restore(flags);

					break;	
				}
					
				if(tmp == i)
				{
					//i still be the bigest node
					memblk->max_freesize = (memblk->free_mmb)[i+1];
#if DEBUG
				        printk("still be the tail node!\n");
				        show_memblk(memblk);
#endif
					local_irq_restore(flags);
					break;
				}
				
				tmp1 = ((memblk->free_mmb)[i] >> 6) & 0x3F;
				(memblk->free_mmb)[tmp1] = (memblk->free_mmb)[tmp1] | 0x3F;
				memblk->max_freesize = (memblk->free_mmb)[tmp1 + 1];

				tmp1 = ((memblk->free_mmb)[tmp] >> 6) & 0x3F;
				(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | i;
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | (tmp1 << 6);
				(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | tmp;
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i << 6);
				
#if DEBUG
				printk("insert middle!\n");
				show_memblk(memblk);
#endif				
				local_irq_restore(flags);
				break;
			}//trail node
			else
			{
				//i is a middle node
                                tmp = memblk->head_num;
                                while((memblk->free_mmb)[tmp + 1] < (memblk->free_mmb)[i + 1])
                                {
                                        tmp = (memblk->free_mmb)[tmp] & 0x3F;
                                        if((tmp == 0x3F) | (tmp == i))
                                                break;
                                }

                                if(tmp == i)
                                {
#if DEBUG
					printk("not need to resort list!\n");
					show_memblk(memblk);
#endif				                                	
					local_irq_restore(flags);
                                        break;
                                }

				if(tmp == memblk->head_num)
				{
					//i will be the head node
					tmp = ((memblk->free_mmb)[i] >> 6) & 0x3F; //front node
					tmp1 = (memblk->free_mmb)[i] & 0x3F;  //next node
					(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | tmp1;
					(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | \
												 (tmp << 6);
                                        (memblk->free_mmb)[i] = (memblk->free_mmb)[i] | 0xFC0;
                                        (memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFFFC0) | memblk->head_num;
                                        (memblk->free_mmb)[memblk->head_num] = \
                                                        ((memblk->free_mmb)[memblk->head_num] & 0xFFFFF03F) | (i<<6);
                                        memblk->head_num = i;
#if DEBUG
					printk("insert to head!\n");
					show_memblk(memblk);
#endif		                                        
					local_irq_restore(flags);
					break;
				}


                                tmp1 = ((memblk->free_mmb)[tmp] >> 6) & 0x3F; //front node
                                tmp2 = ((memblk->free_mmb)[i] >> 6) & 0x3F;
                                tmp3 = (memblk->free_mmb)[i] & 0x3F;
                                //del node i
                                (memblk->free_mmb)[tmp2] = ((memblk->free_mmb)[tmp2] & 0xFFFFFFC0) | tmp3;
                                (memblk->free_mmb)[tmp3] = ((memblk->free_mmb)[tmp3] & 0xFFFFF03F) | (tmp2<<6);
                                 
                        	(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | i;
                                (memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | (tmp1 << 6);
                                (memblk->free_mmb)[i] = (memblk->free_mmb)[i] | tmp;
                                (memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i << 6);
#if DEBUG
				printk("insert before %d node!\n",(int)tmp);
				show_memblk(memblk);
#endif		                                                                        
                                
				local_irq_restore(flags);
				break;

				
				
			}//middle node			
					
		}//not need to del node
		
	}//list_for_each_entry(memblk, &(p_mempool->memblock_list), mmb_list)

	if(addr == HIOS_NULL)
	{
		printk("ERROR! no mem!\n");

		return HIOS_NULL;
	}

#if DEBUG
	printk("after alloc free_mmb:\n");
	show_memblk(memblk);	
#endif 
	
	mmb->handle = handle;
	mmb->addr = (hios_u32)addr;
	mmb->size = size;	
	return addr;
}
EXPORT_SYMBOL(hios_mempool_alloc);


hios_void hios_mempool_free(struct mem_block mmb)
{
        hios_u32 id;
        hios_u32 flags;
        hios_u32 i,tmp,j,tmp1,tmp2;
        struct mempool *p_mempool;
        struct memblock_info *memblk;


	if(mmb.handle == 0 || mmb.size == 0)
	{
		printk("ERROR! hios_mempool_free param invalid!\n");
		return;
	}
	
	if((mmb.handle >> 24) != local_id)
	{
		//send_msg
		//int hios_mcc_sendto(hios_mcc_handle_t *handle, const void *buf, unsigned int len);
		struct hi_mcc_handle_attr attr;
		hios_mcc_handle_t *tr_handle;
		//hios_mcc_handle_opt_t opt;
		
		attr.target_id = (mmb.handle >> 24);
		attr.port = (mmb.handle >> 14) & 0x3FF;
		attr.priority = 1;
		
		tr_handle = hios_mcc_open(&attr);
		if(!tr_handle)
		{
			return;
		}		
		
		hios_mcc_sendto(tr_handle, &mmb, sizeof(struct mem_block));
		hios_mcc_close(tr_handle);
		return;
	}

        id = mmb.handle & 0x3FFF;

        list_for_each_entry(p_mempool, &g_mempool_listhead, mempool_list)
        {
                 if(id == p_mempool->id)
                        break;
        }

        if(id != p_mempool->id)
        {
                printk("ERROR! handle is invalid!\n");
                return;
        }

#if DEBUG
	printk("free mem form mempool%d\n",(int)id);
#endif
	
        list_for_each_entry(memblk, &(p_mempool->memblock_list), mmb_list)
	{
#if DEBUG
		printk("mmb.addr=0x%x,memblk->start_addr=0x%x,memblk->end_addr=0x%x\n",(unsigned int)mmb.addr,(unsigned int)memblk->start_addr,(unsigned int)memblk->end_addr);
#endif		
		if((mmb.addr < memblk->start_addr) || (mmb.addr >= memblk->end_addr))
			continue;

		local_irq_save(flags);
#if TEST		
		prepare_freemmb(memblk);
#endif

#if DEBUG

		printk("befor free mmb, free_mmb:\n");
		show_memblk(memblk);
#endif
	
		i = memblk->head_num;
                while(i != 0x3F){
			if(mmb.addr == (((memblk->free_mmb)[i] & 0xFFFFF000) +(memblk->free_mmb)[i+1]))
			{
				(memblk->free_mmb)[i+1] += mmb.size;
#if DEBUG
				printk("only neet to modify the %d node size!\n",(int)i);
				printk("(memblk->free_mmb)[i+1]=0x%x\n",(unsigned int)(memblk->free_mmb)[i+1]);
#endif				
				if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
				{
					memblk->max_freesize = (memblk->free_mmb)[i+1];
					hebing(memblk,i);
#if DEBUG
					printk("incorporate with the tail node!\n");
					printk("memblk->max_freesize=0x%x\n",(unsigned int)memblk->max_freesize);
					show_memblk(memblk);
#endif
					local_irq_restore(flags);
					return;
				}
				else
				{
					hebing(memblk,i);
#if DEBUG
					printk("incorporate with other node!\n");
					show_memblk(memblk);
#endif 					
					if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
					{
#if DEBUG
						printk("i is tail node!\n");
#endif 								
						local_irq_restore(flags);
						return;
					}		

				}
				

				j = i;
				while(j != 0x3F)
				{
					tmp = (memblk->free_mmb)[j] & 0x3F;
					if(tmp == 0x3F)
						break;

					if((memblk->free_mmb)[i+1] <= (memblk->free_mmb)[tmp+1])
					{
						if(j == memblk->head_num)
						{
#if DEBUG
							printk("incorporate with head node and still be the head node!\n");
							show_memblk(memblk);
#endif									
							local_irq_restore(flags);
							return;
						}
						
						if(i == memblk->head_num)
						{
							tmp1 = (memblk->free_mmb)[i] & 0x3F;
							(memblk->free_mmb)[tmp1] |= 0xFC0;
        	                                        (memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
	                                                (memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i<<6);
                                                	(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | tmp;
                                                	(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | (j<<6);
                                                	memblk->head_num = tmp1;
#if DEBUG
							printk("incorporate with the head node, and insert before %d node!\n",(int)tmp);
							show_memblk(memblk);
#endif
                                                	local_irq_restore(flags);
                                                	return;
						}
						
						if(j == i)
						{
							//not need to resort list!
#if DEBUG
							printk("not need to resort list!\n");
							show_memblk(memblk);
#endif										
							local_irq_restore(flags);
							return;								
						}
						
						tmp2 = (memblk->free_mmb)[i] & 0x3F;
						tmp1 = ((memblk->free_mmb)[i] >> 6) & 0x3F;
						(memblk->free_mmb)[tmp2] = ((memblk->free_mmb)[tmp2] & 0xFFFFF03F) | (tmp1<<6); 
						(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | tmp2;
						(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
						(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i<<6);
						(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | tmp;
						(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | (j<<6);
#if DEBUG
						printk("incorporate with the %d node, and insert before %d node!\n",(int)i,(int)tmp);
						show_memblk(memblk);
#endif						
						local_irq_restore(flags);
						return;
					}
					j = tmp;
				}
				//i will be the tail node
                                if(i == memblk->head_num)
                                {
                                	tmp1 = (memblk->free_mmb)[i] & 0x3F;
                                	
                                	(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (0x3f<<6);
                                	(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
                                	(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | (j<<6) | 0x3F;  
                                	
                                	memblk->max_freesize = (memblk->free_mmb)[i+1];
                                	memblk->head_num = tmp1;
                                	
#if DEBUG
					printk("incorporate with %d node and to be tail node!\n", (int)i);
					show_memblk(memblk);
#endif	                                
                                
                                	local_irq_restore(flags);
                                	return;                                	
                                	
                                }
                                			
                                                                				
				tmp2 = (memblk->free_mmb)[i] & 0x3F;				
                                tmp1 = ((memblk->free_mmb)[i] >> 6) & 0x3F;
				(memblk->free_mmb)[tmp2] = ((memblk->free_mmb)[tmp2] & 0xFFFFF03F) | (tmp1<<6);
                                (memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | tmp2;

				(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | (j<<6) | 0x3F;
				memblk->max_freesize = (memblk->free_mmb)[i+1];
#if DEBUG
				printk("incorporate with the %d node, and to be the tail node!\n",(int)i);
				show_memblk(memblk);
#endif										
				
                                local_irq_restore(flags);
                                return;
			}//need to modify the size
			
			if(((memblk->free_mmb)[i] & 0xFFFFF000) == (mmb.addr + mmb.size))
			{
                                (memblk->free_mmb)[i+1] += mmb.size;
				(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFF) + mmb.addr;
#if DEBUG
				printk("need to modify the size and start_addr!\n");
				printk("(memblk->free_mmb)[%d]=0x%x, (memblk->free_mmb)[i+1]=0x%x\n",(int)i,(unsigned int)(memblk->free_mmb)[i],(unsigned int)(memblk->free_mmb)[i+1]);
#endif	
				
                                if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
                                {
                                        memblk->max_freesize = (memblk->free_mmb)[i+1];
                                        hebing(memblk,i);
#if DEBUG
					printk("incorporate with the tail node!\n");
					show_memblk(memblk);
#endif	
                                        local_irq_restore(flags);
                                        return;
                                }
                                else
				{
					hebing(memblk,i);
#if DEBUG
					printk("incorporate with other node!\n");
					show_memblk(memblk);
#endif 					
					if(((memblk->free_mmb)[i] & 0x3F) == 0x3F)
					{
#if DEBUG
						printk("i is tail node!\n");
#endif 								
						local_irq_restore(flags);
						return;
					}		

				}
                                
                                j = i;
                                while(j != 0x3F)
                                {
                                        tmp = (memblk->free_mmb)[j] & 0x3F;
                                        if(tmp == 0x3F)
                                                break;

                                        if((memblk->free_mmb)[i+1] <= (memblk->free_mmb)[tmp+1])
                                        {
                                                if(j == memblk->head_num)
                                                {
#if DEBUG
							printk("incorporate with the head node!\n");
							show_memblk(memblk);
#endif	                                                	
                                                        local_irq_restore(flags);
                                                        return;
                                                }

                                                if(i == memblk->head_num)
                                                {
                                                        tmp1 = (memblk->free_mmb)[i] & 0x3F;
                                                        (memblk->free_mmb)[tmp1] |= 0xFC0;
                                                        (memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
                                                        (memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i<<6);
                                                        (memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | tmp;
                                                        (memblk->free_mmb)[i] = (memblk->free_mmb)[i] | (j<<6);
                                                        memblk->head_num = tmp1;
#if DEBUG
							printk("incorporate with head node and insert before %d node!\n", (int)tmp);
							show_memblk(memblk);
#endif	   
                                                        local_irq_restore(flags);
                                                        return;
                                                }

                                                tmp2 = (memblk->free_mmb)[i] & 0x3F;
                                                tmp1 = ((memblk->free_mmb)[i] >> 6) & 0x3F;
                                                (memblk->free_mmb)[tmp2] = ((memblk->free_mmb)[tmp2] & 0xFFFFF03F) | (tmp1<<6);
                                                (memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | tmp2;
                                                (memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
                                                (memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFF03F) | (i<<6);
                                                (memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | tmp;
                                                (memblk->free_mmb)[i] = (memblk->free_mmb)[i] | (j<<6);
#if DEBUG
						printk("incorporate with %d node and insert before %d node!\n", (int)i,(int)tmp);
						show_memblk(memblk);
#endif	   
                                                local_irq_restore(flags);
                                                return;
                                        }
                                        j = tmp;
                                }
                                //i will be the tail node
                                if(i == memblk->head_num)
                                {
                                	tmp1 = (memblk->free_mmb)[i] & 0x3F;
                                	
                                	(memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFF03F) | (0x3f<<6);
                                	(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
                                	(memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | (j<<6) | 0x3F;  
                                	
                                	memblk->max_freesize = (memblk->free_mmb)[j+1];
                                	memblk->head_num = tmp1;
                                	
#if DEBUG
					printk("incorporate with %d node and to be tail node!\n", (int)i);
					show_memblk(memblk);
#endif	                                
                                
                                	local_irq_restore(flags);
                                	return;                                	
                                	
                                }
                                
                                tmp2 = (memblk->free_mmb)[i] & 0x3F;
                                tmp1 = ((memblk->free_mmb)[i] >> 6) & 0x3F;
                                (memblk->free_mmb)[tmp2] = ((memblk->free_mmb)[tmp2] & 0xFFFFF03F) | (tmp1<<6);
                                (memblk->free_mmb)[tmp1] = ((memblk->free_mmb)[tmp1] & 0xFFFFFFC0) | tmp2;

                                (memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
                                (memblk->free_mmb)[i] = ((memblk->free_mmb)[i] & 0xFFFFF000) | (j<<6) | 0x3F;
                                memblk->max_freesize = (memblk->free_mmb)[i+1];
#if DEBUG
				printk("incorporate with %d node and to be tail node!\n", (int)i);
				show_memblk(memblk);
#endif	                                
                                
                                local_irq_restore(flags);
                                return;
			
			}//need to modify the start_addr and size
			
			i = (memblk->free_mmb)[i] & 0x3F;	
			
                }//while(i!=0x3F)

		//need to create a new node
		for(i=0;i<MAX_ENTRY;)
		{
			if((memblk->free_mmb)[i] == 0xFFFFFFFF)
				break;
			i = i+2;
		}

#if DEBUG
		printk("use new entry %d \n",(int)(i/2));
		show_memblk(memblk);	
#endif
	
		if(i == MAX_ENTRY)
		{
			printk("ERROR! too many entrys!\n");
			local_irq_restore(flags);
			return;
		}


		
		(memblk->free_mmb)[i] = mmb.addr;
		(memblk->free_mmb)[i+1] = mmb.size;
	
		j = memblk->head_num;
		
		if(((memblk->free_mmb)[j] == 0xFFFFFFFF) || (j == i))
		{
			//no entry. i will be the head
			memblk->head_num = i;
			(memblk->free_mmb)[i] += 0xFFF;
			memblk->max_freesize = (memblk->free_mmb)[i+1];
#if DEBUG
      		        printk("add head node to empty free_mmb!\n");
			show_memblk(memblk);			
#endif						
			local_irq_restore(flags);
                        return;	
		}
		
		while(1)
		{
			if((memblk->free_mmb)[i+1] <= (memblk->free_mmb)[j+1])
			{
				if(j == memblk->head_num)
				{
					(memblk->free_mmb)[i] += 0xFC0;
					(memblk->free_mmb)[i] += j;
					memblk->head_num = i;
					(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFF03F) | (i<<6);
#if DEBUG
      		          		printk("create new node! be the head node!\n");
					show_memblk(memblk);
#endif					
			                local_irq_restore(flags);
                        	        return;
				}
#if DEBUG
      		      		printk("create new node! insert before %d node!\n",(int)j); 
#endif					
				tmp = ((memblk->free_mmb)[j] >> 6) & 0x3F;				
				(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFF03F) | (i<<6);							
				(memblk->free_mmb)[tmp] = ((memblk->free_mmb)[tmp] & 0xFFFFFFC0) | i;							
				(memblk->free_mmb)[i] |= j;						
				(memblk->free_mmb)[i] |= tmp << 6;			
#if DEBUG
      		      		show_memblk(memblk);
#endif						
                                local_irq_restore(flags);
                                return;

			}
			
			tmp = (memblk->free_mmb)[j] & 0x3F; 
			if(tmp == 0x3F)
			{
				//new node will be the tail node
				(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
				(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | (j << 6);
				(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | 0x3F;
				memblk->max_freesize = (memblk->free_mmb)[i+1];
#if DEBUG
      		      		printk("create new node! be the tail node!\n");      		      		
				show_memblk(memblk);
#endif						
                                local_irq_restore(flags);
                                return;				
			}
			j = tmp;
		}//while(i != 0x3F)
#if 0
#if DEBUG
                printk("after free mmb, free_mmb:\n");
		show_memblk(memblk);
#endif
		//i will be the bigest node
		(memblk->free_mmb)[j] = ((memblk->free_mmb)[j] & 0xFFFFFFC0) | i;
		(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | j<<6;
		(memblk->free_mmb)[i] = (memblk->free_mmb)[i] | 0x3F;
		memblk->max_freesize = (memblk->free_mmb)[i+1];
		
		local_irq_restore(flags);
		return;	
#endif
	}//list_for_each_entry(memblk, &(p_mempool->memblock_list), mmb_list)	
	printk("not find match memblock!\n");
}
EXPORT_SYMBOL(hios_mempool_free);


hios_void hios_mempool_destory(hios_u32 handle)
{
        hios_u32 id;
        struct mempool *p_mempool;
        struct memblock_info *memblk;


	if(handle == 0)
	{
		printk("ERROR! hios_mempool_destory param invalid!\n");
		return;
	}

	if((handle >> 24) != local_id)
	{
		//send_msg
		struct hi_mcc_handle_attr attr;
		hios_mcc_handle_t *tr_handle;
		hios_u32 buf[2];
		
		
		attr.target_id = (handle >> 24);
		attr.port = (handle >> 14) & 0x3FF;
		attr.priority = 1;
		
		tr_handle = hios_mcc_open(&attr);
		if(!tr_handle)
			return;		
		
		buf[0] = handle;
		buf[1] = 0xFFFFFFFF;
		
		hios_mcc_sendto(tr_handle, buf, sizeof(buf));	
		hios_mcc_close(tr_handle);		
		return;
	}

        id = handle & 0x3FFF;

        list_for_each_entry(p_mempool, &g_mempool_listhead, mempool_list)
        {
                 if(id == p_mempool->id)
                        break;
        }

        if(id != p_mempool->id)
        {
                printk("ERROR! handle is invalid!\n");
                return ;
        }
	
	list_for_each_entry(memblk, &(p_mempool->memblock_list), mmb_list)
	{
		kfree(memblk);
	}
	
	list_del(&(p_mempool->mempool_list));
	hios_mcc_close(p_mempool->rev_handle);
	
	kfree(p_mempool);
}
EXPORT_SYMBOL(hios_mempool_destory);


hios_char *hios_getmmb_addr(struct mem_block mmb)
{
	return (hios_char *)mmb.addr;
}
EXPORT_SYMBOL(hios_getmmb_addr);

hios_void hios_mempool_init(hios_u32 cpu_id)
{
	local_id = cpu_id;	
}
EXPORT_SYMBOL(hios_mempool_init);

static int mempool_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mempool_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int mempool_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	//void __user *argp = (void __user *)arg;
	//int __user *p = argp;
	//int new_margin;
	hios_u32 val;
	struct mem_block mmb;
	hios_u32 buf[22];

	switch (cmd) {
		case IOC_MEMPOOL_INIT:
			if (get_user(val, (hios_u32 *)arg))
			{
				printk("ERROR! IOC_MEMPOOL_INIT get arg form user failed!\n");
				return -1;
			}
#if DEBUG					
			printk("IOC_MEMPOOL_INIT:local_id=%d\n",(int)val);
#endif
			hios_mempool_init(val);
			return 0;	

		case IOC_GETMMB_ADDR:
			copy_from_user(&mmb,(struct mem_block *)arg,sizeof(struct mem_block));			
			
			val = (hios_u32)hios_getmmb_addr(mmb);
			put_user(val,(hios_u32 *)arg);
			return 0;
			
		case IOC_MEMPOOL_DESTORY:
			get_user(val, (hios_u32 *)arg);		
			hios_mempool_destory(val);
			return 0;

		case IOC_MEMPOOL_FREE:
			copy_from_user(&mmb,(struct mem_block *)arg,sizeof(struct mem_block));
			hios_mempool_free(mmb);
			return 0;

		case IOC_MEMPOOL_ALLOC:
			copy_from_user(buf,(hios_u32 *)arg,sizeof(hios_u32)*2);
#if DEBUG			
			printk("IOC_MEMPOOL_ALLOC:buf[0]=0x%x\n",(unsigned int)buf[0]);
			printk("IOC_MEMPOOL_ALLOC:buf[1]=0x%x\n",(unsigned int)buf[1]);
#endif
			val = (hios_u32)hios_mempool_alloc(&mmb, buf[0], buf[1]);
			buf[0] = val;
			memcpy(&buf[1],&mmb,sizeof(struct mem_block));
#if DEBUG
			printk("IOC_MEMPOOL_ALLOC:buf[0]=0x%x\n",(unsigned int)buf[0]);
			printk("IOC_MEMPOOL_ALLOC:buf[1]=0x%x\n",(unsigned int)buf[1]);
			printk("IOC_MEMPOOL_ALLOC:buf[2]=0x%x\n",(unsigned int)buf[2]);
			printk("IOC_MEMPOOL_ALLOC:buf[3]=0x%x\n",(unsigned int)buf[3]);
			printk("IOC_MEMPOOL_ALLOC:buf[4]=0x%x\n",(unsigned int)buf[4]);
			printk("IOC_MEMPOOL_ALLOC:buf[5]=0x%x\n",(unsigned int)buf[5]);
#endif
			copy_to_user((hios_u32 *)arg,buf,(sizeof(struct mem_block)+sizeof(hios_u32)));
			return 0;

		case IOC_MEMPOOL_ADD:
			copy_from_user(buf,(hios_u32 *)arg,sizeof(hios_u32)*2);
#if DEBUG			
			printk("IOC_MEMPOOL_ADD:buf[0]=0x%x\n",(unsigned int)buf[0]);
			printk("IOC_MEMPOOL_ADD:buf[1]=0x%x\n",(unsigned int)buf[1]);
#endif			
			copy_from_user(&buf[2],(hios_u32 *)((hios_u32 *)arg + 2),sizeof(hios_u32)*2*buf[1]);
#if DEBUG
		        printk("IOC_MEMPOOL_ADD:buf[2]=0x%x\n",(unsigned int)buf[2]);
		        printk("IOC_MEMPOOL_ADD:buf[3]=0x%x\n",(unsigned int)buf[3]);
		        printk("IOC_MEMPOOL_ADD:buf[4]=0x%x\n",(unsigned int)buf[4]);
		        printk("IOC_MEMPOOL_ADD:buf[5]=0x%x\n",(unsigned int)buf[5]);		        
#endif
			val = (hios_u32)hios_add_mempool(buf[0],&buf[2],buf[1]);
			if(val)
				val = -1;	
#if DEBUG
			printk("IOC_MEMPOOL_ADD:ret=%d\n",(int)val);
#endif
			return val;

		case IOC_MEMPOOL_CREATE:
			copy_from_user(buf,(hios_u32 *)arg,sizeof(hios_u32)*9);
#if DEBUG
			printk("IOC_MEMPOOL_CREATE:buf[5]=0x%x\n",(unsigned int)buf[5]);
			printk("IOC_MEMPOOL_CREATE:buf[6]=0x%x\n",(unsigned int)buf[6]);
			printk("IOC_MEMPOOL_CREATE:buf[7]=0x%x\n",(unsigned int)buf[7]);
			printk("IOC_MEMPOOL_CREATE:buf[8]=0x%x\n",(unsigned int)buf[8]);				
#endif
			val = hios_mempool_create((hios_char *)buf,buf[5],buf[6],buf[7],buf[8]);
#if DEBUG
			printk("IOC_MEMPOOL_CREATE:mempool handle=%x\n",(unsigned int)val);
#endif			
			put_user(val,(hios_u32 *)arg);
			return 0;
			
		default:
			printk("ERROR! invaled cmd!\n");
			return -1;
	}

}

static struct file_operations mempool_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= mempool_ioctl,
	.open		= mempool_open,
	.release	= mempool_release,
};

static struct miscdevice mempool_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "mempool",
	.fops		= &mempool_fops,
};

static int __init mempool_init(void)
{
	hios_s32 ret;
#if 0
	if(local_id == 0xFFFFFFFF)
	{
		printk("ERROR! module should be insmod with param local_id!\n");
		return -1;
	}
#endif

	ret = misc_register(&mempool_miscdev);
	if(ret) {
		printk ("ERROR! dev register failed!\n");
		return -1;
	}

	INIT_LIST_HEAD(&g_mempool_listhead);
	//g_workqueue = create_workqueue("mempool_task");
	g_workqueue = create_singlethread_workqueue("mempool_task");
#if DEBUG
	printk("g_workqueue info:\n");
	//printk("name : %s\n",g_workqueue->name);
#endif
	printk("hios mempool1.0 init sucess!\n");
        return 0;
}

static void __exit mempool_exit(void)
{
	struct mempool *mempool = HIOS_NULL;
	struct memblock_info *memblk;

	misc_deregister(&mempool_miscdev);
//loop:	
	if(!(list_empty(&g_mempool_listhead)))
	{
		list_for_each_entry(mempool, &g_mempool_listhead, mempool_list)
		{						
			if(!(list_empty(&(mempool->memblock_list))))
			{					
				list_for_each_entry(memblk, &(mempool->memblock_list), mmb_list)
				{	
					kfree(memblk);
				}
			}
			//list_del(&(mempool->mempool_list));
			hios_mcc_close(mempool->rev_handle);					
			kfree(mempool);	
			//goto loop;			
		}		
	}
	
	destroy_workqueue(g_workqueue);
        return;
}


module_init(mempool_init);
module_exit(mempool_exit);

MODULE_LICENSE("GPL");

//module_param(local_id, uint, 0);


