#include <asm/io.h>
#include "hisi_pci_hw.h"
#include "hisi_pci_data_common.h"
#include "hisi_pci_proto.h"
#include "hi3511_pci.h"

#define _MEM_ALIAGN (32)	/* 32 bytes aliagn */
#define _MEM_ALIAGN_VERS (_MEM_ALIAGN - 1) 
#define _HEAD_SIZE  (sizeof(struct hisi_pci_transfer_head))
#define _MEM_ALIAGN_CHECK (_MEM_ALIAGN - _HEAD_SIZE - _HEAD_SIZE ) /* from head to end need  */
#define _FIX_ALIAGN_DATA (0xC3) /* 11000011b */ 
const static unsigned char fixed_mem[_MEM_ALIAGN_CHECK];
struct hisi_map_shared_memory_info_slave hisi_slave_info_map;
struct hisi_map_shared_memory_info_host hisi_host_info_map[HISI_MAX_PCI_DEV];

extern struct hisi_pcidev_map hisi_map_pcidev;

#if 0   /* All function supported from hw-hal layer */
int hisi_slave_int_check(struct hisi_pcicom_dev *dev)
{
	unsigned int status;
	status=PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3;

	if(hisi_pci_interrupt_check(status)){
		if(hisi_bridge_ahb_readl(CPU_ISTATUS)&(PCIDOORBELL0|PCIDOORBELL1|PCIDOORBELL2|PCIDOORBELL3)) 
		return dev->slot_id;
	}
	return -1;
}

void hisi_slave_int_disable(void)
{
	hisi_bridge_ahb_writel(CPU_IMASK,(hisi_bridge_ahb_readl(CPU_IMASK)&0xfff0f0f0));
}

void hisi_slave_int_clear(void)
{
	//hi3511_bridge_ahb_writel(CPU_ISTATUS,0x000f0000);
	//61104
	hisi_bridge_ahb_writel(CPU_ISTATUS,0x000f0f0f);

}

void hisi_slave_int_enable(void)
{
	hisi_bridge_ahb_writel(CPU_IMASK,(hisi_bridge_ahb_readl(CPU_IMASK)|0x000f0000));
}

void hisi_host_int_disable(struct hisi_pcicom_dev *dev)
{
	unsigned long mask;
	mask = readl(dev->cfg_base_addr + PCI_IMASK);
	mask &= 0xff0fffff;
	writel(mask, dev->cfg_base_addr + PCI_IMASK);
	hisi_pci_interrupt_disable(PCIINT0);	
}

void hisi_host_int_clear(struct hisi_pcicom_dev *dev)
{
	unsigned long status;
	writel(0x00f00000, dev->cfg_base_addr + PCI_ISTATUS);
	status = readl(dev->cfg_base_addr + PCI_ISTATUS);
	hisi_pci_interrupt_clear(PCIINT0);
}

void hisi_host_int_enable(struct hisi_pcicom_dev *dev)
{
	unsigned long int_mask;
	int_mask = readl(dev->cfg_base_addr + PCI_IMASK);
	int_mask |= 0x00f00000;
	writel(int_mask, dev->cfg_base_addr + PCI_IMASK);
	hisi_pci_interrupt_enable(PCIINT0);
}
#endif

int hisi_is_priority_buffer_empty(struct hisi_pcicom_dev *dev)
{
	struct hisi_shared_memory_info *p;
        unsigned int rp, wp;

        /* slave */
        if(hisi_pci_vdd_info.id){
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
		mcc_trace(1, "slave  %d\n", 0);
                p = &pinfo->recv;
        }else{
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
		mcc_trace(1, "host %d\n", 0);
                p = &pinfo->recv;
        }
	mcc_trace(1, "rp addr 0x%8lx wp addr 0x%8lx\n",p->priority.rp_addr, p->priority.wp_addr);
	rp = readl(p->priority.rp_addr);
	wp = readl(p->priority.wp_addr);

	mcc_trace(1, "rp %u wp %u\n",rp, wp);
	return (rp == wp);
}

int hisi_is_normal_buffer_empty(struct hisi_pcicom_dev *dev)
{
	struct hisi_shared_memory_info *p;
        unsigned int rp, wp;

        /* slave */
        if(hisi_pci_vdd_info.id){
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
                p = &pinfo->recv;
        }else{
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->recv;
        }
	rp = readl(p->normal.rp_addr);
	wp = readl(p->normal.wp_addr);
	mcc_trace(1, "rp addr 0x%8lx wp addr 0x%8lx\n",p->normal.rp_addr, p->normal.wp_addr);
	mcc_trace(1, "rp %u wp %u\n",rp, wp);
	return (rp == wp);
}

static void * _wait_data_ready(struct hisi_memory_info *p)
{
	struct hisi_pci_transfer_head * head = NULL;
	unsigned int rp, wp;
	unsigned int dst;
	unsigned int len,aliagn;
	int log = 0;

        rp = readl(p->rp_addr);
        wp = readl(p->wp_addr);
	mcc_trace(1, "rp %u wp %u\n", rp, wp);
	/*
		get data from shared memory by rp and wp
	*/
	head = (struct hisi_pci_transfer_head *) (rp + p->base_addr - _HEAD_SIZE);

	mcc_trace(1, "head 0x%8lx head->check 0x%x head->magic 0x%x \n",(unsigned long)head, head->check,head->magic);
	/* waiting for head finished */
	while(head->check != HISI_HEAD_CHECK_FINISHED);

	/* when head been marked as a jump to top */
	if(head->magic == HISI_HEAD_MAGIC_JUMP){
		unsigned long flage;
		memset(head, 0xCD, _HEAD_SIZE);
		local_irq_save(flage);
		writel(0, p->rp_addr);
		local_irq_restore(flage);
		head = (struct hisi_pci_transfer_head *) (p->base_addr - _HEAD_SIZE);
		rp = 0;
		mcc_trace(1, "jump head 0x%8lx head->check 0x%x head->magic 0x%x \n",(unsigned long)head, head->check,head->magic);
		while(head->check != HISI_HEAD_CHECK_FINISHED);
	}
	
	len = head->length;
	aliagn = _MEM_ALIAGN_VERS & len;
	dst = rp + p->base_addr + len + _MEM_ALIAGN - aliagn;
	mcc_trace(1, "dst 0x%8lx len %u\n", (unsigned long)dst, len);

	/* wait for data finished */
	while(memcmp((void *)dst + _HEAD_SIZE, (void *)&fixed_mem, _MEM_ALIAGN_CHECK)){
#if (MCC_DBG_LEVEL == 2)
		if(!log){
			int i = 0;
			printk("\n dst:\n");
			for(; i < _MEM_ALIAGN_CHECK; i ++){
				if(i % 8 == 0)
					printk("\n");	
				printk("0x%x ", *((char *)dst + _HEAD_SIZE + i));
			}
			printk("\n fixed:\n");
			for(i = 0; i < _MEM_ALIAGN_CHECK; i ++){
				if(i % 8 == 0)
					printk("\n");	
				printk("0x%x ", fixed_mem[i]);
			}
			log = 1;
		}
#endif
	}
	mcc_trace(1, "out %d\n", 0);
	return head;
}

void *hisi_get_normal_head(struct hisi_pcicom_dev *dev)
{
	struct hisi_memory_info *p;
	/* slave */
	if(hisi_pci_vdd_info.id){
		struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
		p = &pinfo->recv.normal;
	}else{
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->recv.normal;
	}

	return _wait_data_ready(p);
}

void *hisi_get_priority_head(struct hisi_pcicom_dev *dev)
{
        struct hisi_memory_info *p;
        if(hisi_pci_vdd_info.id){
        	/* slave */
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
                p = &pinfo->recv.priority;
        }else{
		/* host */
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->recv.priority;
        }

        return _wait_data_ready(p);
}

static void _data_recv(struct hisi_memory_info *p, void *data, unsigned int len)
{
	unsigned int aliag = len & _MEM_ALIAGN_VERS;
	unsigned int fixlen = _MEM_ALIAGN + _MEM_ALIAGN - aliag;
	unsigned long next_rp;
	unsigned long flage;
	void *start = data;
	void *end;
	next_rp = len + (unsigned long)data - p->base_addr + fixlen;

	/* clean shared memory */
	memset(data, 0xA5, len + fixlen - _HEAD_SIZE); 
	mcc_trace(1, "data 0x%8lx len %u \n",(unsigned long)data, len + fixlen);

	/* clean head memory place */
        /* set slice head to 0xCD */
	end = data + len + fixlen;
        for(; start < end ; start += _MEM_ALIAGN){
		mcc_trace(1, "start 0x%8lx end 0x%8lx\n",(unsigned long) (start - _HEAD_SIZE), (unsigned long)end);
        	memset(start - _HEAD_SIZE, 0xCD, _HEAD_SIZE);
        }
	mcc_trace(1, "next_rp %lu p->rp_addr 0x%8lx\n",next_rp, p->rp_addr);
        local_irq_save(flage);
        writel(next_rp, p->rp_addr);
        local_irq_restore(flage);
}

void hisi_normal_data_recieved(struct hisi_pcicom_dev *dev, void *data, unsigned int len)
{
        struct hisi_memory_info *p;
        if(hisi_pci_vdd_info.id){
                /* slave */
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
                p = &pinfo->recv.normal;
        }else{
                /* host */
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->recv.normal;
        }
	_data_recv(p, data, len);
}

void hisi_priority_data_recieved(struct hisi_pcicom_dev *dev, void *data, unsigned int len)
{
        struct hisi_memory_info *p = NULL;
        if(hisi_pci_vdd_info.id){
                /* slave */
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
                p = &pinfo->recv.priority;
        }else{
                /* host */
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->recv.priority;
        }
        _data_recv(p, data, len);
}

static inline unsigned long _memcpy_shbuf(unsigned long wp,unsigned long base, 
				const void *buf, const void *head,unsigned int len)
{
	unsigned int fixlen;
	unsigned int aliagn = len & _MEM_ALIAGN_VERS;
	unsigned long next_wp;
	unsigned long addr;
	fixlen =  _MEM_ALIAGN + _MEM_ALIAGN - aliagn;
	next_wp = wp + len + fixlen;
	addr = wp + base;	

	mcc_trace(1, "addr 0x%8lx len %u \n",addr, len);

	/* bottom have enough space */
	memcpy((void *)addr, buf, len);

	mcc_trace(1, "addr 0x%8lx fixlen %u\n",addr + len, fixlen);
	memset((void *)(addr + len), _FIX_ALIAGN_DATA, fixlen);

	memcpy((void *)(addr - _HEAD_SIZE), head, _HEAD_SIZE);
#if (MCC_DBG_LEVEL >= 3)
	{
		int i;
		printk("\n send data:\n");
		for(i = 0; i < 8; i++){
			if(i%8 == 0)
				printk("\n");
			printk("0x%x ", *(unsigned char *)(addr + i));
		}
	}
#endif

	mcc_trace(1, "next_wp %lu\n",next_wp);
	return next_wp;
}

static int _sendto_shmbuf(struct hisi_memory_info *p, const void *buf, unsigned int len,
			struct hisi_pci_transfer_head *head)
{
	unsigned int aliagn = len & _MEM_ALIAGN_VERS;
	unsigned long base = p->base_addr;
        unsigned long rp, wp;
	unsigned long next_wp;
	unsigned int ret;
	aliagn = _MEM_ALIAGN - aliagn;
	atomic_inc(&p->memmutex);
        if(atomic_read(&p->memmutex) != 1){
                atomic_dec(&p->memmutex);
                return -1;
        }

        rp = readl(p->rp_addr);
        wp = readl(p->wp_addr);

	mcc_trace(1, "wp %lu  rp %lu\n",wp, rp);
	if(wp >= rp){
		/* write point larger than read point */
		if((wp + len + aliagn + _MEM_ALIAGN) < p->buf_len){
			mcc_trace(1, "wp >= rp len %u\n",len);
			next_wp = _memcpy_shbuf(wp, base, buf, head, len);

			mcc_trace(1, "writel to wp_addr 0x%8lx wp  %u\n",p->wp_addr, len);
			writel(next_wp, p->wp_addr);
			ret = len;
		}
		else{
			/* bottom do not have enough space, check the top */
			if(rp > len + aliagn + _MEM_ALIAGN){
				/* top have enough space */
				/* FIXME: market here, when received here know jump to top */
				struct hisi_pci_transfer_head mark;
				mcc_trace(1, "wp >= rp but jump len %u\n",len);
				mark = *head;
				mark.magic = HISI_HEAD_MAGIC_JUMP; 
				memcpy((void *)(wp + base - _HEAD_SIZE), &mark, _HEAD_SIZE);
				next_wp = _memcpy_shbuf(0, base, buf, head, len);
				writel(next_wp, p->wp_addr);
                        	ret = len;
			}
			else{
				/* top do not have enough space */
				ret = -1;
			}
		}
	}
	else{
		/* read point larger than write point */
		if((rp - wp) > len + aliagn + _MEM_ALIAGN){
			mcc_trace(1, "wp < rp len %u\n",len);
			/* have enough space between rp and wp */
			next_wp = _memcpy_shbuf(wp, base, buf, head, len);
			writel(next_wp, p->wp_addr);
                        ret = len;
		}else{
			/* top do not have enough space */
			ret = -1;
		}
	}
	
	mcc_trace(1, "return ret %d\n",ret);
	atomic_dec(&p->memmutex);
	return ret;
}

int hisi_send_priority_data(struct hisi_pcicom_dev *dev, const void *buf, unsigned int len, 
			struct hisi_pci_transfer_head *head)
{
        struct hisi_memory_info *p;
	int ret;

        if(hisi_pci_vdd_info.id){
        	/* slave */
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
                p = &pinfo->send.priority;
        }else{
		/* host */
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->send.priority;
        }
	ret = _sendto_shmbuf(p, buf, len, head);
        
	// send a priority package means timer need reset
	if(timer_pending(&dev->timer)){
		mcc_trace(1, "priority del timer %d\n",0);
		del_timer(&dev->timer);
	}

	/* send priority need to doorbell irq */
	dev->trigger(dev->slot_id - 1);

	return ret;
}

int hisi_send_normal_data(struct hisi_pcicom_dev *dev, const void *buf, unsigned int len, 
			struct hisi_pci_transfer_head *head)
{
        struct hisi_memory_info *p;
	int ret;

        if(hisi_pci_vdd_info.id){
        	/* slave */
                struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;
                p = &pinfo->send.normal;
        }else{
		/* host */
                struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
                p = &pinfo->send.normal;
        }
	ret = _sendto_shmbuf(p, buf, len, head);
	if(!timer_pending(&dev->timer)){
		mcc_trace(2, "normal send set timer %d\n",0);
		dev->timer.expires = jiffies + msecs_to_jiffies(10);
		dev->timer.data = (unsigned long)(dev->slot_id - 1);
		dev->timer.function = dev->trigger;
		add_timer(&dev->timer); 
	}

	return ret;
}

static void _init_meminfo(void *start, void *end , struct hisi_memory_info *p)
{
	p->base_addr = (unsigned long) start;
	p->end_addr = (unsigned long ) end;
	p->buf_len = (unsigned long) (end - start);
	p->rp_addr = (unsigned long) (start - 2 * sizeof(unsigned int) - _HEAD_SIZE);
	p->wp_addr = (unsigned long) (start - sizeof(unsigned int) - _HEAD_SIZE);
	atomic_set(&p->memmutex, 0); 
}

static void _init_mem(void * start, void * end,struct hisi_shared_memory_info *p)
{
	unsigned int size = end - start;
	void *aliag;

	/* host info id is zero */
	/* shared memory in slave */
	if(hisi_pci_vdd_info.id){
		/* set all memory to 0xA5 */
		memset(start, 0xA5, size);

		aliag = (start - _HEAD_SIZE);

		/* set slice head to 0xCD */
		for(; aliag < end; aliag += _MEM_ALIAGN){
			memset(aliag, 0xCD, _HEAD_SIZE);		
		}
	}

	_init_meminfo(start, (start + (size >> 2)), &p->priority);

	_init_meminfo((start + (size >> 2) + _MEM_ALIAGN), end, &p->normal);

	/* salve initial read and write pointer to zero */
	/* but slave only can change */
	if(hisi_pci_vdd_info.id){
	        writel(0, p->priority.rp_addr);
        	writel(0, p->priority.wp_addr);
	        writel(0, p->normal.rp_addr);
        	writel(0, p->normal.wp_addr);
	}
}

int hisi_shared_mem_init(struct hisi_pcicom_dev *dev)
{
	unsigned int sendmem_base, sendmem_end;
	unsigned int recvmem_base, recvmem_end;
 
	/* shared memory started from 256 aliagn */
	sendmem_base = dev->pf_base_addr + (_MEM_ALIAGN - (dev->pf_base_addr & _MEM_ALIAGN_VERS)) + _MEM_ALIAGN;

	sendmem_end = sendmem_base + HISI_PCI_SHARED_SENDMEM_SIZE;
	mcc_trace(1, "sendmem_base 0x%8x pf_base_addr 0x%8xl\n",(unsigned int)sendmem_base,(unsigned int)dev->pf_base_addr);
	mcc_trace(1, "sendmem_end 0x%8x pf_end_addr 0x%8xl\n",(unsigned int)sendmem_end,(unsigned int)dev->pf_end_addr);
	if((sendmem_base <= dev->pf_base_addr) || (sendmem_end >= dev->pf_end_addr)){
		printk("DATA ERROR: send mem out of pci pf memory!\n");
		return -1;
	}

	recvmem_base = sendmem_end + _MEM_ALIAGN * 2;
	recvmem_end = recvmem_base + HISI_PCI_SHARED_RECMEM_SIZE;
	if((recvmem_base <= dev->pf_base_addr) || (recvmem_end >= dev->pf_end_addr)){
		printk("DATA ERROR: send mem out of pci pf memory!\n");
		return -1;
	}
	memset((void *)fixed_mem, _FIX_ALIAGN_DATA, _MEM_ALIAGN_CHECK);
	/* host info id is zero */
	/* info map initial here */
	if(hisi_pci_vdd_info.id){
		struct hisi_map_shared_memory_info_slave *pinfo = &hisi_slave_info_map;

		/* get phy addr and virt addr */		
		pinfo->base_physics = hisi_map_pcidev.map[dev->slot_id].base_addr_phy;
		pinfo->base_virtual = hisi_map_pcidev.map[dev->slot_id].base_addr_virt;
	
		mcc_trace(1, "slave mem init send 0x%8x recv 0x%8x\n", sendmem_base,recvmem_base);

		/* init slave recv buffer */	
		_init_mem((void *)sendmem_base, (void *)sendmem_end, &pinfo->recv);
	
		/* init slave send buffer */	
		_init_mem((void *)recvmem_base, (void *)recvmem_end, &pinfo->send);
	}
	else{
		struct hisi_map_shared_memory_info_host *pinfo = &hisi_host_info_map[dev->slot_id];
	
		mcc_trace(1, "host mem init send 0x%8x recv 0x%8x\n", sendmem_base,recvmem_base);
		/* get phy addr and virt addr */		
		pinfo->base_physics = hisi_map_pcidev.map[dev->slot_id].base_addr_phy;
		pinfo->base_virtual = hisi_map_pcidev.map[dev->slot_id].base_addr_virt;

                /* init host send buffer */
                _init_mem((void *)sendmem_base, (void *)sendmem_end, &pinfo->send);

                /* init host recv buffer */
                _init_mem((void *)recvmem_base, (void *)recvmem_end, &pinfo->recv);
	}
	
	return 0;
}


