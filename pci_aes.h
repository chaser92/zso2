#ifndef __PCI_AES_H_
#define __PCI_AES_H_

#include <linux/pci.h>

#include "aes_dev.h"

int pci_aes_probe(struct pci_dev *dev, const struct pci_device_id *id);
void pci_aes_remove(struct pci_dev *dev);
int pci_aes_init(void (*device_created)(struct aes_dev* dev));
void pci_aes_deinit(void);

#endif