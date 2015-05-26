// Autor: Mariusz Kierski

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
#include "pci_aes_op.h"
#include "aesdev_ioctl.h"
#include "aesdev.h"
#include "aes_dev_context.h"
#include "cyclic_buf.h"
/*
 * Our parameters which can be set at load time.
 */

#define AES_MAJOR 0

int aes_major =   AES_MAJOR;
int aes_minor =   0;
int aes_nr_devs = 0;	/* number of bare aes devices */

MODULE_AUTHOR("Mariusz Kierski");
MODULE_LICENSE("GPL");

struct class *aes_class;

LIST_HEAD(aes_devices);

/*
 * Open and close
 */

int aes_open(struct inode *inode, struct file *filp) {
	int err;
	struct aes_dev *dev;
	struct aes_dev_context* ctx = kmalloc(sizeof(struct aes_dev_context), GFP_KERNEL);
	ctx->mode = MODE_NOT_SET;
	ctx->block_used = 0;
	sema_init(&ctx->mutex, 1);
	if (!ctx) {
		return -ENOMEM;
	}
	ctx->filp = filp;
	if ((err = cbuf_init(&ctx->buf, CBUF_SIZE))) {
		return err;
	}

    dev = container_of(inode->i_cdev, struct aes_dev, cdev);
	ctx->dev = dev;
	filp->private_data = ctx;
	return 0;          /* success */
}

int aes_release(struct inode *inode, struct file *filp) {
	struct aes_dev_context* ctx = filp->private_data;
	cbuf_destroy(&ctx->buf);
	kfree(ctx);
	return 0;
}

ssize_t aes_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	int lock_err;
	int result;
	int nonblock = filp->f_flags & O_NONBLOCK;
	int read;
	struct aes_dev_context* ctx = filp->private_data;
	char* kbuf = ctx->kbuf;

	while (true) {
		if ((lock_err = down_interruptible(&ctx->mutex))) {
			return lock_err;	
		}
		if (ctx->mode == MODE_NOT_SET) {
			up(&ctx->mutex);
			return -EINVAL;
		}
			
		read = cbuf_read_nonblock(&ctx->buf, kbuf, count > CBUF_SIZE ? CBUF_SIZE : count);
		if (read == -EWOULDBLOCK) {
			up(&ctx->mutex);
			if (nonblock) {
				return -EAGAIN;
			} else {
				if ((lock_err = cbuf_wait_for_read(&ctx->buf))) {
					return lock_err;
				}
			}
		} else {
			break;
		}
	}
		
	if (read >= 0 && copy_to_user(buf, kbuf, read)) {
		result = -ENOMEM;
	} else {
		result = read;
	}
	up(&ctx->mutex);
	return result;
}

ssize_t aes_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos) {
	int nonblock = filp->f_flags & O_NONBLOCK;
	int written = 0;
	int lock_err;
	struct aes_dev_context* ctx = filp->private_data;
	char* kbuf = ctx->kbuf;

	// this loop is pure beauty
	while (true) {
		if ((lock_err = down_interruptible(&ctx->mutex))) {
			return lock_err;	
		}
		if (ctx->mode == MODE_NOT_SET) {
			up(&ctx->mutex);
			return -EINVAL;
		}
		if (!cbuf_has_space(&ctx->buf, AESDEV_AES_BLOCK_SIZE)) {
			up(&ctx->mutex);
			if (nonblock) {
				return -EAGAIN;
			} else {
				if ((lock_err = cbuf_wait_for_write(&ctx->buf))) {
					return lock_err;
				}
			}
		} else {
			break;
		}
	}

	count = count > CBUF_SIZE ? CBUF_SIZE : count;
	if (copy_from_user(kbuf, buf, count)) {
		up(&ctx->mutex);
		return -ENOMEM;
	}

	// first copy at most AESDEV_AES_BLOCK_SIZE bytes to small block buffer
	written = count > AESDEV_AES_BLOCK_SIZE - ctx->block_used ? AESDEV_AES_BLOCK_SIZE - ctx->block_used : count;
	memcpy(ctx->block + ctx->block_used, kbuf, written);
	ctx->block_used += written;

	if (ctx->block_used < AESDEV_AES_BLOCK_SIZE) {
		up(&ctx->mutex);
		return written;
	}

	if ((lock_err = pci_aes_lock(ctx->dev))) {
		up(&ctx->mutex);
		return lock_err;
	}
	pci_aes_set_mode(ctx->dev, ctx->mode);
	pci_aes_set_key(ctx->dev, ctx->key);
	pci_aes_next_block(ctx, ctx->block, USE_IV);
	kbuf += written;
	count -= written;
	ctx->block_used = 0;
	

	// Next process all remaining blocks. No blocking here is allowed -
	// if we encounter possible blocking, we exit.
	while (count >= AESDEV_AES_BLOCK_SIZE) {
		if (!cbuf_has_space(&ctx->buf, AESDEV_AES_BLOCK_SIZE)) {
			pci_aes_unlock(ctx->dev);
			up(&ctx->mutex);
			return written;
		}
		pci_aes_next_block(ctx, kbuf, SKIP_IV);
		count -= AESDEV_AES_BLOCK_SIZE;
		kbuf += AESDEV_AES_BLOCK_SIZE;
		written += AESDEV_AES_BLOCK_SIZE;
	}
	pci_aes_unlock(ctx->dev);

	// finally, copy the remainder to small block buffer
	memcpy(ctx->block, kbuf, count);
	ctx->block_used = count;
	written += count;
	up(&ctx->mutex);
	return written;
}

/*
 * The ioctl() implementation
 */

int ctlcmd_to_mode_update_keys(struct aes_dev_context* ctx, unsigned int cmd, unsigned long arg) {
	struct aesdev_ioctl_set_ecb *ecb = (struct aesdev_ioctl_set_ecb *)arg;
	struct aesdev_ioctl_set_iv *iv = (struct aesdev_ioctl_set_iv *)arg;
	struct aesdev_ioctl_get_state *state = (struct aesdev_ioctl_get_state *)arg;

	switch (cmd) {
		case AESDEV_IOCTL_SET_ECB_ENCRYPT:
			return copy_from_user(ctx->key, ecb->key, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_ECB_ENCRYPT;
		case AESDEV_IOCTL_SET_ECB_DECRYPT:
			return copy_from_user(ctx->key, ecb->key, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_ECB_DECRYPT;
		case AESDEV_IOCTL_SET_CBC_ENCRYPT:
			return copy_from_user(ctx->key, iv->key, AESDEV_AES_BLOCK_SIZE) ||
				   copy_from_user(ctx->iv, iv->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_CBC_ENCRYPT;
		case AESDEV_IOCTL_SET_CBC_DECRYPT:
			return copy_from_user(ctx->key, iv->key, AESDEV_AES_BLOCK_SIZE) ||
				   copy_from_user(ctx->iv, iv->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_CBC_DECRYPT;
		case AESDEV_IOCTL_SET_CFB_ENCRYPT:
			return copy_from_user(ctx->key, iv->key, AESDEV_AES_BLOCK_SIZE) ||
				   copy_from_user(ctx->iv, iv->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_CFB_ENCRYPT;
		case AESDEV_IOCTL_SET_CFB_DECRYPT:
			return copy_from_user(ctx->key, iv->key, AESDEV_AES_BLOCK_SIZE) ||
				   copy_from_user(ctx->iv, iv->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_CFB_DECRYPT;
		case AESDEV_IOCTL_SET_OFB:
			return copy_from_user(ctx->key, iv->key, AESDEV_AES_BLOCK_SIZE) ||
				   copy_from_user(ctx->iv, iv->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_OFB;
		case AESDEV_IOCTL_SET_CTR:
			return copy_from_user(ctx->key, iv->key, AESDEV_AES_BLOCK_SIZE) ||
				   copy_from_user(ctx->iv, iv->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_MODE_CTR;
		case AESDEV_IOCTL_GET_STATE:
			return copy_to_user(state, ctx->iv, AESDEV_AES_BLOCK_SIZE) ? -EINVAL :
				AESDEV_COMMAND_GET_STATE;
		default:
			return -ENOTTY;
	}
}

long aes_ioctl_unsafe(struct file* filp, 
	unsigned int cmd, unsigned long arg) {
	int lock_err;
	long result;
	struct aes_dev_context* ctx = filp->private_data;

	if ((lock_err = down_interruptible(&ctx->mutex))) {
		return lock_err;	
	}
	result = aes_ioctl(filp, cmd, arg);
	up(&ctx->mutex);
	return result;
}

long aes_ioctl(struct file *filp,
                 unsigned int cmd, unsigned long arg) {
	struct aes_dev_context* ctx = filp->private_data;
	int new_mode = ctlcmd_to_mode_update_keys(ctx, cmd, arg);
	if (new_mode < 0) {
		return new_mode;
	}
	if (new_mode != AESDEV_IOCTL_GET_STATE) {
		ctx->mode = new_mode;
	}
	return 0;
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


void aes_cleanup_module(void) {
	struct list_head* entry;
	struct list_head* next;
	struct aes_dev* dev;
	dev_t devno = MKDEV(aes_major, aes_minor);

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
	unregister_chrdev_region(devno, AES_NUM_DEVICES); 

	class_destroy(aes_class);
}


/*
 * Set up the char_dev structure for this device.
 */
static int aes_setup_cdev(struct aes_dev *dev, int index) {
	int err;
       
	dev->devno = MKDEV(aes_major, aes_minor + index);
	cdev_init(&dev->cdev, &aes_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aes_fops;
	err = cdev_add(&dev->cdev, dev->devno, 1);
	/* Fail gracefully if need be */
	if (err) {
		printk(KERN_NOTICE "Error %d adding aes%d\n", err, index);
		return err;
	}

	dev->dev = device_create(aes_class, 0, dev->devno, 0, "aes%d", index);
	if (IS_ERR(dev->dev)) {
		printk(KERN_NOTICE "Error %ld adding aes%d to sysfs\n", PTR_ERR(dev->dev), index);
		dev->dev = NULL;
		return 1;
	} else {
		return 0;
	}
}

int aes_initm_get_major(void) {
	int result;
	dev_t dev = MKDEV(AES_MAJOR, 0);
	if ((result = alloc_chrdev_region(&dev, 0, AES_NUM_DEVICES, "aes"))) {
		printk(KERN_WARNING "aes: can't get major %d\n", aes_major);
		return result;
	}
	aes_major = MAJOR(dev);
	return 0;
}

int aes_initm_create_class(void) {
	aes_class = class_create(THIS_MODULE, "aes");
	if (IS_ERR(aes_class)) {
		return PTR_ERR(aes_class);
	}
	return 0;
}

void aes_create_device(struct aes_dev* dev) {
	if (aes_setup_cdev(dev, aes_nr_devs++)) {
		return;
	}
	sema_init(&dev->mutex, 1);
	dev->list.next = 0;
	dev->list.prev = 0;
	list_add_tail(&dev->list, &aes_devices);
}

int aes_init_module(void) {

	int result;
	
	if ((result = aes_initm_get_major() ||
				  aes_initm_create_class() ||
				  pci_aes_init(aes_create_device))) {
		printk(KERN_WARNING "AES was not initialized.");
		aes_cleanup_module();
		return result;
	}

	return 0; /* succeed */
}

module_init(aes_init_module);
module_exit(aes_cleanup_module);
