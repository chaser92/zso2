#ifndef __PCI_AES_OP_H_
#define __PCI_AES_OP_H_

#define USE_IV 0
#define SKIP_IV 1

#include <linux/compiler.h>
#include "aes_dev_context.h"

void pci_aes_exec16(struct aes_dev* dev, void* meta, void* data);
void pci_aes_set_key(struct aes_dev* dev, void* key);
void pci_aes_set_mode(struct aes_dev* dev, int mode);
int pci_aes_next_block(struct aes_dev_context* ctx, void* block, int skip_iv);
int pci_aes_lock(struct aes_dev* dev);
void pci_aes_unlock(struct aes_dev* dev);
#endif