#include "pci_aes_op.h"
#include "aes_dev.h"

void pci_write16(void* __iomem dest, void* src) {
	int i = 0;
	u32* src32 = src;
	for (i=0; i<4; i++) {
		iowrite32(*src32, dest);
		src32++;
		dest += 4;
	}
}

void pci_read16(void* __iomem src, void* dest) {
	int i = 0;
	uint32_t* dest32 = dest;
	for (i=0; i<4; i++) {
		dest32[i] = ioread32(&src);
		src += 4;
	}
}

void pci_aes_set_key(struct aes_dev* dev, void* key) {
	void* __iomem buf_key = dev->buf + AES_PCI_KEY_OFFSET;
	pci_write16(buf_key, key);
}

void pci_aes_exec16(struct aes_dev* dev, void* meta, void* data) {
	void* __iomem buf_meta = dev->buf + AES_PCI_META_OFFSET;
	void* __iomem buf_data = dev->buf + AES_PCI_DATA_OFFSET;
	pci_write16(buf_meta, meta);
	pci_write16(buf_data, data);
	pci_read16(buf_data, data);
}

void pci_aes_set_mode(struct aes_dev* dev, int mode) {
	void* __iomem buf_mode = dev->buf + AES_XFER_TASK_OFFSET;
	iowrite32(mode, buf_mode);
}
