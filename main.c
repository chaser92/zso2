/*
 * main.c -- the bare aes char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>	/* copy_*_user */

#include "aes.h"		/* local definitions */
#include "pci_aes.h"
#include "aes_dev_context.h"
#include "cyclic_buf.h"
/*
 * Our parameters which can be set at load time.
 */

#define AES_MAJOR 0

int aes_major =   AES_MAJOR;
int aes_minor =   0;
int aes_nr_devs = 0;	/* number of bare aes devices */

//module_param(aes_major, int, S_IRUGO);
//module_param(aes_minor, int, S_IRUGO);
//module_param(aes_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct class *aes_class;

LIST_HEAD(aes_devices);
LIST_HEAD(aes_contexts);

/*
 * Open and close
 */

int aes_open(struct inode *inode, struct file *filp)
{
	int err;
	//struct aes_dev *dev;
	struct aes_dev_context* ctx = kmalloc(sizeof(struct aes_dev_context), GFP_KERNEL);
	printk(KERN_NOTICE "Opening\n");
	if (!ctx) {
		return -ENOMEM;
	}
	ctx->filp = filp;
	if ((err = cbuf_init(&ctx->buf, 4096))) {
		return err;
	}
    dev = container_of(inode->i_cdev, struct aes_dev, cdev);
	ctx->dev = dev;
	filp->private_data = ctx;
	return 0;          /* success */
}

int aes_release(struct inode *inode, struct file *filp)
{
	struct aes_dev_context* ctx = filp->private_data;
	cbuf_destroy(&ctx->buf);
	kfree(ctx);
	return 0;
}

ssize_t aes_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	// struct aes_dev_context* ctx = find_context_by_filp(filp);
	
	return 0;
}

ssize_t aes_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	return 0;
}

/*
 * The ioctl() implementation
 */

long aes_ioctl(struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}


struct file_operations aes_fops = {
	.owner =    		THIS_MODULE,
	.read =     		aes_read,
	.write =    		aes_write,
	.unlocked_ioctl =  	aes_ioctl,
	.compat_ioctl =    	aes_ioctl,
	.open =     		aes_open,
	.release =  		aes_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void aes_cleanup_module(void)
{
	struct list_head* entry;
	struct list_head* next;
	struct aes_dev* dev;
	dev_t devno = MKDEV(aes_major, aes_minor);

	printk(KERN_NOTICE "Cleaning up...\n");

	pci_aes_deinit();

	/* Get rid of our char dev entries */

	list_for_each_safe(entry, next, &aes_devices) {
		dev = list_entry(entry, struct aes_dev, list);
		device_destroy(aes_class, dev->devno);
		cdev_del(&dev->cdev);
		list_del(entry);
		kfree(dev);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 16); // TODO

	class_destroy(aes_class);
}


/*
 * Set up the char_dev structure for this device.
 */
static int aes_setup_cdev(struct aes_dev *dev, int index)
{
	int err;
       
	dev->devno = MKDEV(aes_major, aes_minor + index);
	cdev_init(&dev->cdev, &aes_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aes_fops;
	err = cdev_add(&dev->cdev, dev->devno, 1);
	/* Fail gracefully if need be */
	if (err) {
		printk(KERN_NOTICE "Error %d adding aes%d", err, index);
		return err;
	}

	dev->dev = device_create(aes_class, 0, dev->devno, 0, "myaes%d", index);
	if (IS_ERR(dev->dev)) {
		printk(KERN_NOTICE "Error %ld adding aes%d to sysfs\n", PTR_ERR(dev->dev), index);
		dev->dev = 0;
		return 1;
	} else {
		printk(KERN_NOTICE "Device successfully added!!!!\n");
		return 0;
	}
}

int aes_initm_get_major(void) {
	int result;
	dev_t dev = MKDEV(AES_MAJOR, 0);
	if ((result = alloc_chrdev_region(&dev, 0, 16, "myaes"))) {
		printk(KERN_WARNING "aes: can't get major %d\n", aes_major);
		return result;
	}
	aes_major = MAJOR(dev);
	return 0;
}

int aes_initm_create_class(void) {
	aes_class = class_create(THIS_MODULE, "myaes");
	if (IS_ERR(aes_class)) {
		return PTR_ERR(aes_class);
	}
	return 0;
}

void aes_create_device(struct aes_dev* dev) {
	if (aes_setup_cdev(dev, aes_nr_devs++)) {
		return;
	}
	dev->list.next = 0;
	dev->list.prev = 0;
	list_add_tail(&dev->list, &aes_devices);
}

int aes_init_module(void)
{

	int result;
	
	printk(KERN_WARNING "AES IS UP!\n");
	if ((result = aes_initm_get_major() ||
				  aes_initm_create_class())) {
		printk(KERN_WARNING "AES was not initialized.");
		aes_cleanup_module();
		return result;
	}
	pci_aes_init(aes_create_device);

	return 0; /* succeed */
}

module_init(aes_init_module);
module_exit(aes_cleanup_module);
