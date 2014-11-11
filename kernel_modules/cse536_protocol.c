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

#include "cse536_protocol.h"

static struct node bufHead;

// destination and local address variables
__be32 cse536_daddr = 0;
__be32 cse536_saddr = 0;



// Receive function for cse536 protocol
static int cse536_recv(struct sk_buff *skb)
{
        struct node *tmp = NULL;

        tmp = kmalloc(sizeof(struct node), GFP_KERNEL);
        tmp->data = kmalloc(sizeof(char)* MAX_MSG_SIZE, GFP_KERNEL);

        memset(tmp->data, 0, MAX_MSG_SIZE);
        memcpy(tmp->data, skb->data, skb->len);
	tmp->len = skb->len;

        list_add_tail( &(tmp->list), &(bufHead.list));

        pr_info("%s: Receviced %d bytes: %s \n", __FILE__,
                        skb->len, tmp->data);

        return 0;
}

// Error handling function for cse536 protocol
static void cse536_error(struct sk_buff *skb, u32 info)
{
        pr_info("%s: Error in packet \n", __FILE__);
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


int cse536_sendmsg(char *data, size_t len)
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
EXPORT_SYMBOL(cse536_sendmsg);

// Set destination address
void cse536_set_addr(char *addr) 
{
	 cse536_daddr = in_aton(addr);
}
EXPORT_SYMBOL(cse536_set_addr);

// Get the message from the list
void cse536_getmsg(char *data, size_t *len)
{
	struct node *entry = NULL;
        struct list_head *tmp = NULL;

        if (list_empty( &(bufHead.list) )) {
		*len = 0;
                return;
	}

        tmp = bufHead.list.next;
        entry = list_entry(tmp, struct node, list);
        list_del(tmp);

	memcpy(data, entry->data, entry->len);
	*len = entry->len;
       
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
