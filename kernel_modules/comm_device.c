/*
 *  This program creates a character device /dev/cse536
 *  which can be used to send and receive message over IP.
 */
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

#define DEVICE_NAME	"cse536"
#define MAX_BUF_SIZE	10
#define MAX_MSG_SIZE	256

/* Structure for buffer linklist */
struct node {
	struct list_head list;
	char	*data;
};


/* Structure per-device */
struct comm_device {
	char		dev_name[20];
	struct cdev	cdev;
}*comm_devp;


static dev_t comm_dev_number;	// Alloted device number //
struct class *comm_dev_class; 	// Tie with device model //

///////////////////////////////////////////////////////////////////
//		Protocol Part
///////////////////////////////////////////////////////////////////
/*
static int cse536_recv(struct sk_buff *skb)
{
	struct node *tmp = NULL;

	tmp = kmalloc(sizeof(struct node), GFP_KERNEL);
	tmp->data = kmalloc(sizeof(char)* MAX_MSG_SIZE, GFP_KERNEL);

	memset(tmp->data, 0, MAX_MSG_SIZE);
	memcpy(tmp->data, skb->data, skb->len);

	list_add_tail( &(tmp->list), &(comm_devp->bufHead));

	pr_info("%s: Receviced %d bytes: %s \n", DEVICE_NAME, 
			skb->len, tmp->data);

	return 0;
}


static void cse536_error(struct sk_buff *skb, u32 info)
{
	pr_info("%s: Error in packet \n", DEVICE_NAME);
}

// Regester protocol with  IP 
static const struct net_protocol cse536_protocol = {
	.handler	= cse536_recv,
	.err_handler	= cse536_error,
	.no_policy	= 1,
	.netns_ok	= 1,
};


static int add_my_proto(void)
{
	return inet_add_protocol(&cse536_protocol, IPPROTO_CSE536);
}

static int del_my_proto(void)
{
	return inet_del_protocol(&cse536_protocol, IPPROTO_CSE536);
}

static int cse536_sendmsg(char *data, size_t len)
{
	struct sk_buff 	*skb;
	struct iphdr	*iph;
	struct rtable	*rt;
	struct net 	*net	= &init_net;
	unsigned char 	*skbdata;

	// Create and setup sk_buff
	skb = alloc_skb(sizeof(struct iphdr) + 4096, GFP_ATOMIC);
	skb_reserve(skb, sizeof(struct iphdr) + 1500);
	skbdata = skb_put(skb, len);
	memcpy(skbdata, data, len);

	//setup and add ip header
	skb_push(skb, sizeof(struct iphdr));
	skb_reset_network_header(skb);
	iph = ip_hdr(skb);
	iph->version 	= 4;
	iph->ihl 	= 5;
	iph->tos	= 0;
	iph->frag_off	= 0;
	iph->ttl	= 64;
	iph->daddr	= cse536_daddr;
	iph->saddr	= cse536_saddr;
	iph->protocol	= IPPROTO_CSE536;
	iph->id		= htons(1);
	iph->tot_len	= htons(skb->len);

	// get the route
	rt = ip_route_output(net, cse536_daddr, cse536_saddr, 0, 0);
	skb_dst_set(skb, &rt->dst);

	return ip_local_out(skb);
}


// Get local ip address 
static void getLocalAddress(void)
{
	struct net_device *eth0 = dev_get_by_name(&init_net, "eth0");
	struct in_device *ineth0 = in_dev_get(eth0);

	for_primary_ifa(ineth0) {
		cse536_saddr = ifa->ifa_address;
	} endfor_ifa(ineth0);
}

*/

///////////////////////////////////////////////////////////////////
// 	Device part
///////////////////////////////////////////////////////////////////

/*
 * Open comm device
 */
int comm_open(struct inode *inode, struct file *file)
{
	printk("%s: Device opening\n", DEVICE_NAME);
	return 0;
}

/*
 * Release comm device
 */ 
int comm_release(struct inode *inode, struct file *file)
{
	printk("%s: Device closing\n",DEVICE_NAME);
	return 0;
}


/*
 * Reads message from the buffer linklist
 */
ssize_t comm_read(struct file *file, char __user *buf , size_t count,
		loff_t *ppos)
{
	int ret = 0;
	struct node *entry = NULL;
	struct list_head *tmp = NULL;

	if (list_empty(&comm_devp->bufHead))
		return 0;
	
	tmp = comm_devp->bufHead.next;
	entry = list_entry(tmp, struct node, list);
	list_del(tmp);
	
	if(copy_to_user( (void __user*)buf, (const void *)entry->data,
		strlen(entry->data)) != 0) {
		pr_info("%s: Error copying data \n",DEVICE_NAME);
		ret = -1;
	}
	else {
		ret = strlen(entry->data);
		if(entry->data)
			kfree(entry->data);
		if(entry)
			kfree(entry);
	}

	return ret;
}



/*
 *  sends the message over the IP 
 */
ssize_t comm_write(struct file *file, const char *buf, size_t count,
		loff_t *ppos)
{
	char *tmp_data = NULL;
	tmp_data = kzalloc(sizeof(char)* MAX_MSG_SIZE, GFP_KERNEL);
	if(!tmp_data) {
		pr_info("%s: Insufficient memory\n", DEVICE_NAME);
		return -ENOMEM;
	}

	if (copy_from_user((void *)tmp_data, 
		(const void __user *)buf, count) != 0) {
		pr_info("%s: Error copying data from user buf\n", DEVICE_NAME);
		kfree(tmp_data);
		return -1;
	}
	tmp_data[strlen(tmp_data)] = '\0';

	if (tmp_data[0] == '1') {
		// set the destination address
		cse536_daddr = in_aton(tmp_data+1);
	}
	else {
		cse536_sendmsg(tmp_data+1, count);
	}
	pr_info("%s: data written = %s : %d \n",DEVICE_NAME, tmp_data, (unsigned int)count);

	if(tmp_data)
		kfree(tmp_data);

	return count;
}


/* File operations structure. */
static struct file_operations comm_fops = {
    .owner   = THIS_MODULE,           /* Owner          */
    .open    = comm_open,             /* Open method    */
    .release = comm_release,          /* Release method */
    .read    = comm_read,             /* Read method    */
    .write   = comm_write,            /* Write method   */
};



static int __init comm_init(void)
{
	int ret;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&comm_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "%s: Can't register device\n", DEVICE_NAME); 
		return -1;
	}

	comm_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	
	// Allocate memory for per-device structure
	comm_devp = kmalloc(sizeof(struct comm_device), GFP_KERNEL);
	if (!comm_devp) {
		pr_info("%s : Insufficient memory !!\n",DEVICE_NAME);
		unregister_chrdev_region((comm_dev_number), 1);
		return -ENOMEM;
	}


	// Add new protocol
	if( add_cse536_proto() == -1) {
		pr_info("%s: Error registering protocol \n", DEVICE_NAME);
		kfree(comm_devp);
		unregister_chrdev_region((comm_dev_number), 1);
		return -1;
	}

	/* Connect the file operations with the cdev */
	cdev_init(&comm_devp->cdev, &comm_fops);
	comm_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&comm_devp->cdev, (comm_dev_number), 1);
	if (ret) {
		printk("%s: Bad cdev\n", DEVICE_NAME);
		del_cse536_proto();
		kfree(comm_devp);
		unregister_chrdev_region((comm_dev_number), 1);
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(comm_dev_class, NULL, MKDEV(MAJOR(comm_dev_number), 0),
			NULL, "%s",DEVICE_NAME);	

	pr_info("%s: Device Initialized\n", DEVICE_NAME);
	return 0;
}


static void __exit comm_exit(void)
{
	/* Destroy device */
	device_destroy(comm_dev_class, MKDEV(MAJOR(comm_dev_number), 0));
	cdev_del(&comm_devp->cdev);

	del_cse536_proto();

	if(!comm_devp->buf_size)
		kfree(comm_devp);
	else {
		// Free all the buffer nodes in list
	}

	/* Destroy driver_class */
	class_destroy(comm_dev_class);

	/* Release the major number */
	unregister_chrdev_region((comm_dev_number), 1);	
}


module_init(comm_init);
module_exit(comm_exit);
MODULE_AUTHOR("Shashank Karkare");
MODULE_DESCRIPTION("Communication over IP");
MODULE_LICENSE("GPL");
