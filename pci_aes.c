#include "pci_aes.h"
#include "aesdev.h"

struct pci_device_id aespci_idtable[2] = {
	{
		PCI_DEVICE(AESDEV_VENDOR_ID, AESDEV_DEVICE_ID)
	}, {

	}
};

void (*device_created)(struct aes_dev* dev);

struct pci_driver aespci = {
	.name = "aes",
	.probe = pci_aes_probe,
	.remove = pci_aes_remove
};

int pci_aes_probe(struct pci_dev *dev, const struct pci_device_id *id) {
	int retval;
	struct aes_dev* aesdev = kmalloc(sizeof(struct aes_dev), GFP_KERNEL);
	if (!aesdev) {
		printk(KERN_WARNING "Unable to allocate memory for aesdev.\n");
		return -ENOMEM;
	}

	if ((retval = pci_enable_device(dev))) {
		printk(KERN_WARNING "Unable to enable device.\n");
		return retval;
	}
	if ((retval = pci_request_regions(dev, "aes"))) {
		printk(KERN_WARNING "Unable to request regions.\n");
		return retval;
	}
	aesdev->buf = pci_iomap(dev, 0, 0);
	device_created(aesdev);
	pci_set_drvdata(dev, aesdev);

	return 0;
}

void pci_aes_remove(struct pci_dev *dev) {
	struct aes_dev* aesdev;
	aesdev = pci_get_drvdata(dev);
	pci_iounmap(dev, aesdev->buf);
	pci_release_regions(dev);
	pci_disable_device(dev);
}

int pci_aes_init(void (*_device_created)(struct aes_dev* dev)) {
	int err;
	device_created = _device_created;
	aespci.id_table = aespci_idtable;
	if ((err = pci_register_driver(&aespci))) {
		printk(KERN_WARNING "Unable to initialize PCI.\n");	
	}
	
	return err;
}

void pci_aes_deinit() {
	pci_unregister_driver(&aespci);
}
