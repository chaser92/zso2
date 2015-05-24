#include "pci_aes.h"

struct pci_device_id aespci_idtable[2] = {
	{
		PCI_DEVICE(0x1af4, 0x10fc)
	}, {

	}
};

int probed = 0;
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
	printk(KERN_WARNING "I was probed!\n");
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
	printk("pci_aes_probe dev = %d", dev);
	probed++;
	return 0;
}

void pci_aes_remove(struct pci_dev *dev) {
	struct aes_dev* aesdev;
	printk("pci_aes_remove dev = %d", dev);
	aesdev = pci_get_drvdata(dev);
	pci_iounmap(dev, aesdev->buf);
	printk(KERN_WARNING "Removing pci\n");
	pci_release_regions(dev);
	pci_disable_device(dev);
}

int pci_aes_init(void (*_device_created)(struct aes_dev* dev)) {
	int registered;
	device_created = _device_created;
	aespci.id_table = aespci_idtable;
	registered = pci_register_driver(&aespci);
	printk(KERN_NOTICE "PCI was initialized. %d devices found.\n", registered);
	return registered;
}

void pci_aes_deinit() {
	printk(KERN_NOTICE "Unregistering PCI driver...\n");
	pci_unregister_driver(&aespci);
}
