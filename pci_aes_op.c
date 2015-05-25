#include "pci_aes_op.h"
#include "aes_dev_context.h"
#include "cyclic_buf.h"

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

void pci_aes_set_meta(struct aes_dev* dev, void* meta) {
	void* __iomem buf_meta = dev->buf + AES_PCI_META_OFFSET;
	pci_write16(buf_meta, meta);
}

void pci_aes_exec16(struct aes_dev* dev, void* data) {
	void* __iomem buf_data = dev->buf + AES_PCI_DATA_OFFSET;
	pci_write16(buf_data, data);
	pci_read16(buf_data, data);
}

int pci_aes_next_block(struct aes_dev_context* ctx, void* block, int nonblock) {
	pci_aes_exec16(ctx->dev, block);
	if (nonblock) {
		return cbuf_write_nonblock(&ctx->buf, block, 16);
	} else {
		return cbuf_write(&ctx->buf, block, 16);
	}
}

void pci_aes_set_mode(struct aes_dev* dev, int mode) {
	void* __iomem buf_mode = dev->buf + AES_XFER_TASK_OFFSET;
	iowrite32(mode, buf_mode);
}
