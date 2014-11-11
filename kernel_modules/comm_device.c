/*
 *  This program creates a character device /dev/cse536
 *  which can be used to send and receive message over IP.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/device.h>

#include "cse536_protocol.h"

#define DEVICE_NAME	"cse536"
#define MAX_MSG_SIZE	256
#define NUM_DEVICE	1


/* Structure per-device */
struct comm_device {
	char		name[20];
	struct cdev	cdev;
	unsigned int	devid;
}*comm_devp[NUM_DEVICE];


static dev_t comm_dev_number;	// Alloted device number //
struct class *comm_dev_class; 	// Tie with device model //

///////////////////////////////////////////////////////////////////
// 	Device part
///////////////////////////////////////////////////////////////////

/*
 * Open comm device
 */
int comm_open(struct inode *inode, struct file *file)
{
	struct comm_device *devtmp;

	devtmp = container_of(inode->i_cdev, struct comm_device,cdev);

	file->private_data = devtmp;
	printk("%s: Device opening\n",devtmp->name);

	return 0;
}

/*
 * Release comm device
 */ 
int comm_release(struct inode *inode, struct file *file)
{
	struct comm_device  *devtmp;

	devtmp = file->private_data;
	printk("%s: Device closing\n",devtmp->name);

	return 0;
}


/*
 * Reads message from the buffer linklist
 */
ssize_t comm_read(struct file *file, char __user *buf , size_t count,
		loff_t *ppos)
{

	int ret = 0;
	char *data = NULL;
	struct comm_device *tmpdev;

	tmpdev = file->private_data;

	data = kzalloc(sizeof(char)*MAX_MSG_SIZE, GFP_KERNEL);
	if (!data) {
		pr_info("%s: Insufficient memory\n", tmpdev->name);
		return -ENOMEM;
	}

	cse536_getmsg(data, &ret);
	
	if(copy_to_user( (void __user*)buf, (const void *)data, ret) != 0) {
		pr_info("%s: Error copying data \n", tmpdev->name);
		ret = -1;
	}

	kfree(data);
	return ret;
}



/*
 *  sends the message over the IP 
 */
ssize_t comm_write(struct file *file, const char *buf, size_t count,
		loff_t *ppos)
{
	int ret = 0;
	struct comm_dev *tmpdev = NULL;
	char *tmp_data = NULL;

	tmpdev = file->private_data;

	tmp_data = kzalloc(sizeof(char)* MAX_MSG_SIZE, GFP_KERNEL);
	if(!tmp_data) {
		pr_info("%s: Insufficient memory\n", tmpedv->name);
		return -ENOMEM;
	}

	if (copy_from_user((void *)tmp_data, 
		(const void __user *)buf, count) != 0) {
		pr_info("%s: Error copying data from user buf\n", tmpdev->name);
		kfree(tmp_data);
		return -1;
	}
	tmp_data[strlen(tmp_data)] = '\0';

	if (tmp_data[0] == '1') {
		// set the destination address
		cse536_set_addr(tmp_data+1);
	}
	else {
		cse536_sendmsg(tmp_data+1, count);
	}
	pr_info("%s: data written = %s : %d \n",tmpdev->name, tmp_data, (unsigned int)count);

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
	int i;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&comm_dev_number, 0, NUM_DEVICE, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "%s: Can't register device\n", DEVICE_NAME); 
		return -1;
	}

	comm_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	
	for (i = 0; i < NUM_DEVICE; i++)
	{
		// Allocate memory for per-device structure
		comm_devp[i] = kmalloc(sizeof(struct comm_device), GFP_KERNEL);
		if ( !(comm_devp[i])) {
			pr_info("%s : Insufficient memory !!\n",DEVICE_NAME);
			unregister_chrdev_region((comm_dev_number), NUM_DEVICE);
			return -ENOMEM;
		}

		sprintf(comm_devp[i]->name, "%s-%d", DEVICE_NAME, i);

		/* Connect the file operations with the cdev */
		cdev_init(&(comm_devp[i])->cdev, &comm_fops);
		(comm_devp[i])->cdev.owner = THIS_MODULE;

		/* Connect the major/minor number to the cdev */
		ret = cdev_add(&comm_devp[i]->cdev, (comm_dev_number), NUM_DEVICE);
		if (ret) {
			printk("%s: Bad cdev\n", DEVICE_NAME);
			kfree(comm_devp);
			unregister_chrdev_region((comm_dev_number), 1);
			return ret;
		}

		/* Send uevents to udev, so it'll create /dev nodes */
		device_create(comm_dev_class, NULL, MKDEV(MAJOR(comm_dev_number), i),
				NULL, "%s-%d",DEVICE_NAME,i);	
	}

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
