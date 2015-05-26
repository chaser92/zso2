#include "pci_aes_op.h"
#include "aes_dev_context.h"
#include "cyclic_buf.h"
#include "aesdev.h"

void pci_write16(void* __iomem dest, void* src) {
	int i = 0;
	u8* src32 = src;
	for (i = 0; i < AESDEV_AES_BLOCK_SIZE; i++) {
		iowrite8(src32[i], dest);
		dest++;
	}
}

void pci_read16(void* __iomem src, void* dest) {
	int i = 0;
	u8* dest32 = dest;
	for (i = 0; i < AESDEV_AES_BLOCK_SIZE; i++) {
		dest32[i] = ioread8(src);
		src++;
	}
}

void pci_aes_set_key(struct aes_dev* dev, void* key) {
	void* __iomem buf_key = dev->buf + AESDEV_AES_KEY(0);
	pci_write16(buf_key, key);
}

void pci_aes_exec16(struct aes_dev* dev, void* iv, void* data) {
	void* __iomem buf_iv = dev->buf + AESDEV_AES_STATE(0);
	void* __iomem buf_data = dev->buf + AESDEV_AES_DATA(0);
	if (iv) {
		pci_write16(buf_iv, iv);
	}
	pci_write16(buf_data, data);
	pci_read16(buf_data, data);
	if (iv) {
		pci_read16(buf_iv, iv);
	}
}

int pci_aes_next_block(struct aes_dev_context* ctx, void* block, int skip_iv) {
	pci_aes_exec16(ctx->dev, ctx->iv, block);
	return cbuf_write_nonblock(&ctx->buf, block, AESDEV_AES_BLOCK_SIZE);
}

void pci_aes_set_mode(struct aes_dev* dev, int mode) {
	void* __iomem buf_mode = dev->buf + AESDEV_XFER_TASK;
	iowrite32(mode, buf_mode);
}

int pci_aes_lock(struct aes_dev* dev) {
	if (down_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}
	return 0;
}

void pci_aes_unlock(struct aes_dev* dev) {
	up(&dev->mutex);
}