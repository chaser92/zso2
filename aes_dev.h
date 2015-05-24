#ifndef __AES_DEV_H_
#define __AES_DEV_H_

#include <linux/pci.h>
#include <linux/cdev.h>

struct aes_dev {
	struct list_head list;
	void* __iomem buf;
	int udev_id;
	dev_t devno;
	struct cdev cdev;
	struct device *dev;
};

#endif