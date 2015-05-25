#ifndef __AES_DEV_CONTEXT_H_
#define __AES_DEV_CONTEXT_H_

#include <linux/semaphore.h>

#include "cyclic_buf.h"
#include "aes_dev_context.h"
#include "aes_dev.h"
#include "aesdev.h"
#include "aes.h"

struct aes_dev_context {
	struct list_head list;
	struct cyclic_buf buf;
	struct file* filp;
	struct aes_dev* dev;
	struct semaphore mutex;
	int mode;
	int block_used;
	char block[AESDEV_AES_BLOCK_SIZE];
	char key[AESDEV_AES_BLOCK_SIZE];
	char iv[AESDEV_AES_BLOCK_SIZE];
	char kbuf[CBUF_SIZE];
};

#endif