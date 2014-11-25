#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/param.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <net/ip.h>
#include <net/route.h>
#include <net/sock.h>
#include <net/protocol.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/inet.h>
#include <linux/wait.h>

#include "cse536_protocol.h"

#define NAME	"cse536_protocol"

static unsigned int ACK = 0;
static struct node bufHead;
static struct semaphore listSem;

atomic_t localClock = ATOMIC_INIT(0);

// Initialize the wait queue
DECLARE_WAIT_QUEUE_HEAD(cse536_wqueue);

// destination and local address variables
__be32 cse536_daddr = 0;
__be32 cse536_saddr = 0;

// Receive function for cse536 protocol
static int cse536_recv(struct sk_buff *skb)
{
        struct node *tmp = NULL;
	char *ack_data = NULL;
	int   tmpClock;

	// If the receiving packet is ACK then we store it in the list
	if ( ((struct transaction_struct *)(skb->data))->recID == 0) {

		// Wake up the waiting process
		ACK = 1;
		wake_up(&cse536_wqueue);

	        tmp = kmalloc(sizeof(struct node), GFP_KERNEL);
	        tmp->data = kmalloc(sizeof(struct transaction_struct), GFP_KERNEL);

	        memset(tmp->data, 0, sizeof(struct transaction_struct));
        	memcpy(tmp->data, skb->data, skb->len);

		down(&listSem);   		// Acquire semaphore
		list_add_tail( &(tmp->list), &(bufHead.list));
		up(&listSem);			// Release semaphore

		pr_info("%s: ACK received\n", NAME);

	} else {   // If the receiving packet is of event type
		
		// Send the ACK packet on receiving the event packet
		ack_data = kmalloc(sizeof(char)* sizeof(struct transaction_struct), GFP_KERNEL);
		memcpy(ack_data, skb->data, skb->len);
		tmpClock = tmp->data->finalClock;	

		if (tmpClock >= atomic_read(&localClock))
			atomic_set(&localClock, tmpClock + 1);
		else
			atomic_inc(&localClock);
		
	        pr_info("%s: Receviced %d bytes: %s \n", NAME, skb->len, tmp->data->msg);

		((struct transaction_struct *)ack_data)->recID = 0;
		cse536_sendmsg(ack_data, skb->len);
		kfree(ack_data);
	}

        return 0;
}

// Error handling function for cse536 protocol
static void cse536_error(struct sk_buff *skb, u32 info)
{
        pr_info("%s: Error in packet \n", NAME);
}


/* Regester protocol with  IP */
static const struct net_protocol cse536_protocol = {
        .handler        = cse536_recv,
        .err_handler    = cse536_error,
        .no_policy      = 1,
        .netns_ok       = 1,
};


static int add_cse536_proto(void)
{
        return inet_add_protocol(&cse536_protocol, IPPROTO_CSE536);
}


static int del_cse536_proto(void)
{
        return inet_del_protocol(&cse536_protocol, IPPROTO_CSE536);
}


static int __cse536_sendmsg(char *data, size_t len)
{
	struct sk_buff  *skb;
        struct iphdr    *iph;
        struct rtable   *rt;
        struct net      *net    = &init_net;
        unsigned char   *skbdata;

        // Create and setup sk_buff
        skb = alloc_skb(sizeof(struct iphdr) + 4096, GFP_ATOMIC);
        skb_reserve(skb, sizeof(struct iphdr) + 1500);
        skbdata = skb_put(skb, len);
        memcpy(skbdata, data, len);

        //setup and add ip header
        skb_push(skb, sizeof(struct iphdr));
        skb_reset_network_header(skb);
        iph = ip_hdr(skb);
        iph->version    = 4;
        iph->ihl        = 5;
        iph->tos        = 0;
        iph->frag_off   = 0;
        iph->ttl        = 64;
        iph->daddr      = cse536_daddr;
        iph->saddr      = cse536_saddr;
        iph->protocol   = IPPROTO_CSE536;
        iph->id         = htons(1);
        iph->tot_len    = htons(skb->len);

        // get the route
        rt = ip_route_output(net, cse536_daddr, cse536_saddr, 0, 0);
        skb_dst_set(skb, &rt->dst);

        return ip_local_out(skb);
}


int cse536_sendmsg(char *data, size_t len)
{
	unsigned int attempt = 0;
	struct transaction_struct *tmp = (struct transaction_struct *)data;
	int flag = 0;
	int ret;

	
	if ( ((struct transaction_struct*)data)->recID == 1) {
		pr_info("Here in sendmsg \n");


		tmp->originalClock = atomic_read(&localClock);
		atomic_inc(&localClock);
		tmp->finalClock    =  tmp->originalClock+1;			// Incremented value

		pr_info("After updateing clock\n");
		cse536_daddr = ((struct transaction_struct *)data)->destAddr;
		((struct transaction_struct *)data)->sourceAddr = cse536_saddr;
		ACK = 0;

		while( attempt != RETRY_ATTEMPTS) {
			__cse536_sendmsg(data, len);
			ret = wait_event_timeout(cse536_wqueue, ACK == 1U, WAIT_TIME_SEC*HZ);
			if (ret) {
				flag = 1;
				pr_info("%s: Message sent : %s\n", NAME, ((struct transaction_struct *)data)->msg);
				break;
			}
			attempt++;
		}

		if (!flag) {
			pr_info("%s: Message failed : %s\n", NAME, ((struct transaction_struct *)data)->msg);
			return -1;
		}
	}
	else {
                cse536_daddr = ((struct transaction_struct *)data)->sourceAddr;
		__cse536_sendmsg(data, len);
		pr_info("%s: ACK sent\n", NAME);
	}
	
	return 0;
}
EXPORT_SYMBOL(cse536_sendmsg);

// Get the message from the list
void cse536_getmsg(char *data, size_t *len)
{
	struct node *entry = NULL;
        struct list_head *tmp = NULL;

        if (list_empty( &(bufHead.list) )) {
		*len = 0;
                return;
	}
	
	down(&listSem);		// Acquire semaphore
        tmp = bufHead.list.next;
        entry = list_entry(tmp, struct node, list);
        list_del(tmp);
	up(&listSem);		// Release semaphore

	memcpy(data, entry->data, sizeof(struct transaction_struct));
	*len = sizeof(struct transaction_struct);
       
	if(entry->data)
		kfree(entry->data);
	if(entry)
		kfree(entry);
        
}
EXPORT_SYMBOL(cse536_getmsg);


// Get local ip address 
static void get_local_address(void)
{
        struct net_device *eth0 = dev_get_by_name(&init_net, "eth0");
        struct in_device *ineth0 = in_dev_get(eth0);

        for_primary_ifa(ineth0) {
                cse536_saddr = ifa->ifa_address;
        } endfor_ifa(ineth0);
}



static int __init cse536_init(void)
{
	INIT_LIST_HEAD(&bufHead.list);	
	sema_init(&listSem, 0);
	add_cse536_proto();
	get_local_address();	

	return 0;
}



static void __exit cse536_exit(void)
{
	struct node 	 *tmp = NULL;
	struct list_head *pos = NULL;
	struct list_head *q   = NULL;

	del_cse536_proto();

	// Free all the elements in the list
	list_for_each_safe(pos, q, &bufHead.list) {
		tmp = list_entry(pos, struct node, list);
		list_del(pos);
		if(tmp->data)
			kfree(tmp->data);
		kfree(tmp);
	}
}


module_init(cse536_init);
module_exit(cse536_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("shashank Karkare");
