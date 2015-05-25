#ifndef __AES_DEV_CONTEXT_H_
#define __AES_DEV_CONTEXT_H_

// #include <linux/pci.h>

#include "cyclic_buf.h"
#include "aes_dev_context.h"
#include "aes_dev.h"

struct aes_dev_context {
	struct list_head list;
	struct cyclic_buf buf;
	struct file* filp;
	struct aes_dev* dev;
	int mode;
	int block_used;
	char block[16];
	char key[16];
	char iv[16];
	char kbuf[4096];
};

#endif