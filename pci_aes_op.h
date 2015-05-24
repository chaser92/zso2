#ifndef __PCI_AES_OP_H_
#define __PCI_AES_OP_H_

#define AES_BLOCK_SIZE 16
#define AES_PCI_KEY_OFFSET 0x10
#define AES_PCI_META_OFFSET 0x20
#define AES_PCI_DATA_OFFSET 0x30
#define AES_XFER_TASK_OFFSET 0x48

#include <linux/compiler.h>
#include "aes_dev.h"

enum aes_mode {
	ECB_ENCRYPT = 0,
	ECB_DECRYPT = 1,
 	CBC_ENCRYPT = 2,
	CBC_DECRYPT = 3,
	CFB_ENCRYPT = 4,
	CFB_DECRYPT = 5,
	OFB = 6,
	CTR = 7
};

void pci_aes_exec16(struct aes_dev* dev, void* meta, void* data);
void pci_aes_set_key(struct aes_dev* dev, void* key);
void pci_aes_set_mode(struct aes_dev* dev, int mode);

#endif