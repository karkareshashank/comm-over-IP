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

#define DEVICE_NAME	"cse536"
#define IPPROTO_CSE536	234
#define MAX_BUF_SIZE	10

/* Structure for buffer linklist */
struct node {
	struct list_head link;
	char	*data;
};


/* Structure per-device */
struct comm_device {
	struct list_head 	bufHead;
	unsigned int 		buf_size;
	struct cdev		cdev;
}*comm_devp;


static dev_t comm_dev_number;	// Alloted device number //
struct class *comm_dev_class; 	// Tie with device model //


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
	return 0;
}



/*
 *  sends the message over the IP 
 */
ssize_t comm_write(struct file *file, const char *buf, size_t count,
		loff_t *ppos)
{
	return 0;
}


/* File operations structure. */
static struct file_operations comm_fops = {
    .owner   = THIS_MODULE,           /* Owner          */
    .open    = comm_open,             /* Open method    */
    .release = comm_release,          /* Release method */
    .read    = comm_read,             /* Read method    */
    .write   = comm_write,            /* Write method   */
};



int __init comm_init(void)
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

	// Initialize the buffer list 
	INIT_LIST_HEAD(&comm_devp->bufHead);
	comm_devp->buf_size = 0;

	/* Connect the file operations with the cdev */
	cdev_init(&comm_devp->cdev, &comm_fops);
	comm_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&comm_devp->cdev, (comm_dev_number), 1);
	if (ret) {
		printk("%s: Bad cdev\n", DEVICE_NAME);
		kfree(comm_devp);
		unregister_chrdev_region((comm_dev_number), 1);
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(comm_dev_class, NULL, MKDEV(MAJOR(comm_dev_number), 0), NULL, "%s",DEVICE_NAME);	

	pr_info("%s: Device Initialized\n", DEVICE_NAME);
	return 0;
}


void __exit comm_exit(void)
{
	/* Destroy device */
	device_destroy(comm_dev_class, MKDEV(MAJOR(comm_dev_number), 0));
	cdev_del(&comm_devp->cdev);

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
