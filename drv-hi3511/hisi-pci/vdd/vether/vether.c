
#include <linux/config.h>

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/rtnetlink.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware/amba.h>
#include <asm/hardware/clock.h>

#include <pci_vdd.h>
#include <vdd-utils.h>

#define VETHER_DBUG_LEVEL	5
#define VETHER_DEBUG(level, s, params...) do{ if(level >= VETHER_DBUG_LEVEL)\
	                	printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
	                }while(0)

#define VETHER_MAX_SLAVE 32
#define VETHER_MSG_ID	2
#define VETHER_MSG_PORT	1

#define VETHER_HOST_ID 0


#define INVALID_TARGET_ID 0xFF
#define BROADCAST_ID	0xFE
#define MULTICAST_ID	0xFD

#if 0
static struct timeval s_old_time;
static int PCI_PrintTimeDiff(const char *file, const char *fun, const int line)
{
       struct timeval cur;
       long diff = 0;  /* unit: millisecond */  
       do_gettimeofday(&cur);
       diff = (cur.tv_usec - s_old_time.tv_usec);
       printk("\nfile:%s, function:%s, line:%d, us :%ld\n", file, fun, line, diff); 
       s_old_time = cur;
}

#define PCI_DBG_TM_INIT()  do_gettimeofday(&s_old_time)
#define PCI_DBG_TM_DIFF()  PCI_PrintTimeDiff(__FILE__, __FUNCTION__, __LINE__)

#endif

static unsigned char vether_mac[] = {0x00, 0x00, 0x4C, 0x4A, 0x44, 0x00};
static unsigned char vether_mac_broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


static int find_target_id(struct sk_buff *skb)
{
	unsigned char *mac = skb->data;
	//int i = 0;
	
	/* first check if packet is for broadcast */
	if(memcmp(mac, vether_mac_broadcast, ETH_ALEN-1) == 0)
		return BROADCAST_ID;

	/* check if packet is multicast */
	if(mac[0]%0x1)
		return MULTICAST_ID;

	/* check if packet is send to Hi35xx */
	if(memcmp(mac, vether_mac, ETH_ALEN-1)) {
		/* packet is unknown mac, dropped */
		return INVALID_TARGET_ID;
	}

	if(mac[ETH_ALEN-1] <= VETHER_MAX_SLAVE)
		return mac[ETH_ALEN-1];

	return INVALID_TARGET_ID;
}

static int find_sender_id(struct sk_buff *skb)
{
	unsigned char *mac = (unsigned char *)skb->data + ETH_ALEN;

	if(memcmp(mac, vether_mac, ETH_ALEN-1)) {
		return INVALID_TARGET_ID;
	}

	if(mac[ETH_ALEN-1] <= VETHER_MAX_SLAVE)
		return mac[ETH_ALEN-1];

	return INVALID_TARGET_ID;
}

//#define vether_rcv_get_len(x) 1600


struct vether_local {
	struct net_device_stats stats;
	int is_host;
	int is_open;
	int myid;

	void *vdd[VETHER_MAX_SLAVE];
};

static int vether_is_host(void)
{
	return vdd_is_host();
}

static int vether_get_myid(void)
{
	return vdd_get_myid();
}

static int vcom_get_target_num(void)
{
	return vdd_get_target_num();
}


struct vether_send_queue_head {
	struct sk_buff *skb;
	void *vdd;
};

#define MAX_VETHER_SENDQ_SZ 1024
static struct vether_send_queue_head *vether_send_q;
int sendq_rd, sendq_wr;

#define VETHER_SENDQ_GET()  ({ struct vether_send_queue_head *__p = NULL; \
				if(sendq_wr!=sendq_rd) __p = &vether_send_q[sendq_rd]; __p; })
#define VETHER_SENDQ_FREES() ({ int __frees = 0; \
				if(sendq_wr>=sendq_rd) \
					__frees = MAX_VETHER_SENDQ_SZ - (sendq_wr-sendq_rd) -1; \
				else __frees = sendq_rd-sendq_wr; \
				__frees; \
			})
#define VETHER_SENDQ_INQ(vdd, skb) ({ 	int __ret = -1; \
					if(VETHER_SENDQ_FREES()) { \
						struct vether_send_queue_head *p = &vether_send_q[sendq_wr++]; \
						if(sendq_wr ==MAX_VETHER_SENDQ_SZ) sendq_wr=0; \
						p->skb = skb; p->vdd = vdd; \
						__ret = sendq_wr; \
					}; __ret; \
				})
#define VETHER_SENDQ_DEQ() ({ \
				if(sendq_wr!=sendq_rd) { \
					sendq_rd++; \
					if(sendq_rd ==MAX_VETHER_SENDQ_SZ) sendq_rd=0; \
				} \
				sendq_rd; \
			})


static int vether_recv2skb(void *handle, void *head, struct net_device *dev, struct sk_buff **pp_skb)
{
	int rvlen;
	struct sk_buff* skb;
	struct vether_local *lp;
	//static int recv_flag = 0;

	*pp_skb = NULL;

	lp = netdev_priv(dev);

	rvlen = vether_rcv_get_len(head);
	VETHER_DEBUG(0, "rvlen %d", rvlen);
	if(rvlen < 0) {
		lp->stats.rx_errors++;
		lp->stats.rx_dropped++;
		lp->stats.rx_length_errors++;
		return -1;
	}

        skb = dev_alloc_skb(rvlen + 2);
        if (skb == NULL) {
		/* Again, this seems a cruel thing to do */
		printk(KERN_ERR "get skb fail, out off memory?\n");

		lp->stats.rx_errors++;
		lp->stats.rx_dropped++;
               	return -1;
        }
	skb_reserve(skb, 2);
	VETHER_DEBUG(0, "recv %d", rvlen);

	if(pci_vdd_recvfrom(handle, skb->data, rvlen, 0) != rvlen) {
		lp->stats.rx_errors++;
		lp->stats.rx_dropped++;
		dev_kfree_skb(skb);
               	return -1;
	}

	*pp_skb = skb;
	VETHER_DEBUG(1, "recv %d", rvlen);
	return rvlen;
}

static int vether_host_recv(void *handle, void *head)
{
	struct pci_vdd_opt opt;
	struct net_device *dev;
	struct sk_buff* skb;
	struct vether_local *lp;
	int rvlen;
	int targetid;

	pci_vdd_getopt(handle, &opt);
	dev = (struct net_device *)opt.data;
	lp = netdev_priv(dev);

	rvlen = vether_recv2skb(handle, head, dev, &skb);
	if(rvlen<0)
		return rvlen;

	/* simple switch */
	targetid = find_target_id(skb);
	VETHER_DEBUG(0, "targetid %d lp->myid %d", targetid,lp->myid);
	if(targetid == INVALID_TARGET_ID) {
		lp->stats.rx_dropped++;
		dev_kfree_skb(skb);
		return 0;
	}

	skb->dev = dev;
	skb_put(skb, rvlen);

	/* packet is not for host */
	if(targetid != lp->myid) {
		VETHER_DEBUG(0, "targetid %d  lp->myid %d", targetid, lp->myid);
		VETHER_DEBUG(0, "targetid %d", find_target_id(skb));
		/* if packet is broadcast or multicast, add reference and send to target */
		if(targetid == BROADCAST_ID || targetid == MULTICAST_ID)
			skb_get(skb);
		VETHER_DEBUG(0, "targetid %d", find_target_id(skb));
		if(dev->hard_start_xmit(skb, dev) != NET_XMIT_SUCCESS)
			dev_kfree_skb(skb);
	}

	VETHER_DEBUG(0, "targetid %d  lp->myid %d", targetid, lp->myid);
	/* to host rx */
	if(targetid == lp->myid || targetid == BROADCAST_ID || targetid == MULTICAST_ID) {
		skb->protocol=eth_type_trans(skb,dev);
		lp->stats.rx_packets++;
		lp->stats.rx_bytes += skb->len;

    		dev->last_rx = jiffies;
    		netif_rx(skb);
	}

	return 0;
}

static int vether_slave_recv(void *handle, void *head)
{
	struct pci_vdd_opt opt;
	struct net_device *dev;
	struct sk_buff* skb;
	struct vether_local *lp;
	int rvlen;
	int targetid;
	
	pci_vdd_getopt(handle, &opt);
	dev = (struct net_device *)opt.data;
	lp = netdev_priv(dev);

	rvlen = vether_recv2skb(handle, head, dev, &skb);
	if(rvlen<0)
		return rvlen;
	
	targetid = find_target_id(skb);
	VETHER_DEBUG(0, "targetid %d", targetid);
	if(targetid == lp->myid || targetid == BROADCAST_ID || targetid == MULTICAST_ID)
	{
       		skb->dev = dev;
	       	skb_put(skb, rvlen);
		lp->stats.rx_packets++;
		lp->stats.rx_bytes += skb->len;
		skb->protocol=eth_type_trans(skb,dev);
	    	dev->last_rx = jiffies;

    		netif_rx(skb);
	} else {
		lp->stats.rx_dropped++;
		dev_kfree_skb(skb);
	}
	VETHER_DEBUG(0, "rvlen %d", rvlen);
	return 0;
}

static int vether_host_open(struct net_device *dev)
{
	struct pci_vdd_opt opt;
	struct vether_local *lp = netdev_priv(dev);
	int i;

	VETHER_DEBUG(1, "%d", lp->is_open);

	if(lp->is_open)
		return -EBUSY;
	VETHER_DEBUG(1, "%d", vcom_get_target_num());
	for(i=0; i<vcom_get_target_num() && i<VETHER_MAX_SLAVE; i++) {
		VETHER_DEBUG(0, "i %d", i);
		lp->vdd[i] = pci_vdd_open(i+1, VETHER_MSG_ID, VETHER_MSG_PORT);
		if(lp->vdd[i] == NULL) {
			for(; i>0; i--)
				pci_vdd_close(lp->vdd[i-1]);
			return -EIO;
		}
		VETHER_DEBUG(0, "%d", i);
		pci_vdd_getopt(lp->vdd[i], &opt);
		opt.flags|=0x1;//c58721 for asyn recv
		opt.data = (unsigned long)dev;
		opt.recvfrom_notify = vether_host_recv;
		pci_vdd_setopt(lp->vdd[i], &opt);
	}
	lp->is_open = 1;

	try_module_get(THIS_MODULE);

	return 0;
}

static int vether_slave_open(struct net_device *dev)
{
	struct pci_vdd_opt opt;
	struct vether_local *lp = netdev_priv(dev);

	VETHER_DEBUG(1, "%d", 0);

	if(lp->is_open)
		return -EBUSY;

	lp->vdd[lp->myid] = pci_vdd_open(VETHER_HOST_ID, VETHER_MSG_ID, VETHER_MSG_PORT);

	if(lp->vdd[lp->myid] == NULL)
		return -EIO;

	pci_vdd_getopt(lp->vdd[lp->myid], &opt);
	opt.flags|=0x1;//c58721 for asyn recv
	opt.data = (unsigned long)dev;
	opt.recvfrom_notify = vether_slave_recv;
	pci_vdd_setopt(lp->vdd[lp->myid], &opt);

	lp->is_open = 1;

	try_module_get(THIS_MODULE);

        netif_carrier_on(dev);
	netif_start_queue(dev);

	return 0;
}

static int vether_host_close(struct net_device *dev)
{
	struct vether_local *lp = netdev_priv(dev);
	int i;

	VETHER_DEBUG(1, "%d", 0);

	netif_stop_queue(dev);
	netif_carrier_off(dev);

	for(i=0; i<vcom_get_target_num() && i<VETHER_MAX_SLAVE; i++) {
		pci_vdd_close(lp->vdd[i]);
		lp->vdd[i] = NULL;
	}

	lp->is_open = 0;

	module_put(THIS_MODULE);

	return 0;
}

static int vether_slave_close(struct net_device *dev)
{
	struct vether_local *lp = netdev_priv(dev);

	VETHER_DEBUG(1, "%d", 0);

	pci_vdd_close(lp->vdd[lp->myid]);
	lp->vdd[lp->myid] = NULL;

	lp->is_open = 0;

	module_put(THIS_MODULE);

	return 0;
}


static void vether_send_packet_work(void *data)
{
	struct vether_send_queue_head *p;
	//static int send_flag = 0;

	while( (p=VETHER_SENDQ_GET()) !=NULL) {
			
		pci_vdd_sendto(p->vdd, p->skb->data, p->skb->len, 0x5);//20071108
		
		if(VETHER_SENDQ_FREES()>=VETHER_MAX_SLAVE)
			netif_wake_queue(p->skb->dev);
		dev_kfree_skb(p->skb);
		VETHER_SENDQ_DEQ();
	}
}
static int vether_change_mtu(struct net_device *dev, int new_mtu)
{
	/* check ranges */
	if ((new_mtu < 60) || (new_mtu > 3000))
		return -EINVAL;

	/*
	 * Do anything you need, and the accept the value
	 */
	dev->mtu = new_mtu;

	VETHER_DEBUG(5, "mtu %d", dev->mtu);
	
	return 0; /* success */	
}

static DECLARE_WORK(vether_send_work, vether_send_packet_work, NULL);

static inline int vether_send_paket(struct net_device *dev, void *vdd, struct sk_buff *skb)
{
	int ret = -1;
	static int pack_num = 0;

	if(in_irq() || preempt_count()) {
		if(VETHER_SENDQ_INQ(vdd, skb) !=-1)
			ret = skb->len;
		skb->dev = dev;
		skb_get(skb);
		schedule_work(&vether_send_work);
	} else {
		pack_num++;	
		VETHER_DEBUG(1,  "pack len %d", skb->len);
		ret = pci_vdd_sendto(vdd, skb->data, skb->len, 0x5);

	
		VETHER_SENDQ_DEQ();
		VETHER_DEBUG(1, "pack_num %d", pack_num);
	}

	if(VETHER_SENDQ_FREES()>=VETHER_MAX_SLAVE)
		netif_wake_queue(dev);
	else 
		netif_stop_queue(dev);

	return ret;
}

static int vether_host_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	struct vether_local *lp = netdev_priv(dev);
	int id = find_target_id(skb);

	VETHER_DEBUG(1, "id %d", id);

	/* mac address unknown, dropped */
	if(id == INVALID_TARGET_ID) {
		lp->stats.tx_dropped++;

		return NET_XMIT_DROP;
	}
	
	VETHER_DEBUG(0, "id %d", id);
	VETHER_DEBUG(0, "VETHER_SENDQ_FREES %d", VETHER_SENDQ_FREES());
	VETHER_DEBUG(0, "lp->vdd[id - 1] %p", lp->vdd[id - 1]);
	
	/* send broadcast packet */ 
	if((id == BROADCAST_ID || id == MULTICAST_ID) && VETHER_SENDQ_FREES()>=VETHER_MAX_SLAVE) {
		int i, tx = 0;
		int sender_id = find_sender_id(skb) -1;
		VETHER_DEBUG(0, "sender_id %d", sender_id);
		VETHER_DEBUG(0, "VETHER_SENDQ_FREES() %d", VETHER_SENDQ_FREES());
		/* send to each slave */
		for(i=0, tx=0; i<VETHER_MAX_SLAVE && lp->vdd[i]; i++) {
			if(sender_id == i)
				continue;
			if(vether_send_paket(dev, lp->vdd[i], skb) != skb->len) {
				lp->stats.tx_aborted_errors++;
				lp->stats.tx_errors++;
			} else {
				tx++;
			}
		}

		if(tx) {
			lp->stats.tx_packets++;
			lp->stats.tx_bytes += skb->len;
		}

		dev_kfree_skb(skb);

		return NET_XMIT_SUCCESS;

	} 
	else if(lp->vdd[id - 1] && VETHER_SENDQ_FREES()) {
		VETHER_DEBUG(0, "id %d", id);
		/* send to certain slave */
		if(vether_send_paket(dev, lp->vdd[id - 1], skb) != skb->len) {
			lp->stats.tx_aborted_errors++;
			lp->stats.tx_errors++;
			return NET_XMIT_DROP;
		} else {
			lp->stats.tx_packets++;
			lp->stats.tx_bytes += skb->len;
		}

		dev_kfree_skb(skb);

		return NET_XMIT_SUCCESS;

	} else {
		VETHER_DEBUG(0, "id %d", id);
		/* unknown target */
		lp->stats.tx_dropped++;

		return NET_XMIT_DROP;
	}
}


static int vether_slave_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	struct vether_local *lp = netdev_priv(dev);
	VETHER_DEBUG(0, "lp->myid %d", lp->myid);
	
	if(vether_send_paket(dev, lp->vdd[lp->myid], skb) != skb->len) {
		lp->stats.tx_aborted_errors++;
		lp->stats.tx_errors++;

		return NET_XMIT_DROP;
	} else {
		lp->stats.tx_packets++;
		lp->stats.tx_bytes += skb->len;
	}

	dev_kfree_skb(skb);

	return NET_XMIT_SUCCESS;
}

static struct net_device_stats *vether_get_stats(struct net_device *dev)
{
	struct vether_local *lp = netdev_priv(dev);
	VETHER_DEBUG(0, "%d", 0);
	return &lp->stats;
}

static struct net_device *vether_dev = NULL;

static int __init vether_probe(struct device *device)
{
	int ret = 0;
	struct vether_local *lp;

	VETHER_DEBUG(1, "%d", 0);

	vether_dev = alloc_etherdev(sizeof(struct vether_local));
	if(vether_dev == NULL)
		return -ENOMEM;

	lp = netdev_priv(vether_dev);
	lp->is_host = vether_is_host();
	lp->myid = vether_get_myid();
        dev_set_drvdata(device, vether_dev);

	if(lp->is_host) {
		vether_dev->open		= vether_host_open;
		vether_dev->stop		= vether_host_close;
		vether_dev->hard_start_xmit 	= vether_host_send_packet;
	} else {
		vether_dev->open		= vether_slave_open;
		vether_dev->stop		= vether_slave_close;
		vether_dev->hard_start_xmit 	= vether_slave_send_packet;
	}

	vether_dev->watchdog_timeo	= 3*HZ;
	vether_dev->get_stats		= vether_get_stats;
	vether_dev->change_mtu		= vether_change_mtu;

	/* set mac address */
	memcpy(vether_dev->dev_addr, vether_mac, ETH_ALEN);

	vether_dev->dev_addr[ETH_ALEN-1] = (unsigned char)vether_get_myid();

	ret = register_netdev(vether_dev);
	if(ret) {
		free_netdev(vether_dev);
		return -1;
	}

	vether_send_q = vmalloc(sizeof(struct vether_send_queue_head)*MAX_VETHER_SENDQ_SZ);
	sendq_rd = 0;
	sendq_wr = 0;

	VETHER_DEBUG(1, "%d", ret);

	return ret;
}

static int __exit vether_remove(struct device *device)
{
	dev_set_drvdata(device, NULL);

	return 0;
}

void hisilicon_vether_device_release(struct device * dev)
{
}

static struct device_driver vether_driver ={
	.name = "pci-vether",
	.bus = &platform_bus_type,
	.probe = vether_probe,
	.remove = vether_remove,
};

static u64 hisilicon_vether_dma_mask = 0xffffffffUL;

static struct platform_device hisilicon_vether_device = {
	.name = "pci-vether",
	.id   = 0,
	.dev  = {
		.dma_mask = &hisilicon_vether_dma_mask,
		.coherent_dma_mask = 0xffffffffUL,
		.release = hisilicon_vether_device_release,
	},
	.num_resources = 0,
};


static int __init vether_init(void)
{
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	platform_device_register(&hisilicon_vether_device);

        driver_register(&vether_driver);

	return 0;
}

static void __exit vether_exit(void)
{
	int ret = 0;

        driver_unregister(&vether_driver);

	unregister_netdev(vether_dev);

	if(ret){
		printk("vether unregister error\n");
		return ;
	}

	free_netdev(vether_dev);
	vfree(vether_send_q); 

	platform_device_unregister(&hisilicon_vether_device);
}

module_init(vether_init);
module_exit(vether_exit);

MODULE_AUTHOR("liujiandong");
MODULE_DESCRIPTION("Hisilicon virtual ethernet over pci");
MODULE_LICENSE("GPL");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
