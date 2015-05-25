#ifndef AESDEV_H
#define AESDEV_H

#define AESDEV_VENDOR_ID		0x1af4
#define AESDEV_DEVICE_ID		0x10fc

#define AESDEV_ENABLE			0x000
#define AESDEV_ENABLE_XFER_DATA		0x00000001
#define AESDEV_ENABLE_FETCH_CMD		0x00000002
#define AESDEV_STATUS			0x004
#define AESDEV_STATUS_XFER_DATA		0x00000001
#define AESDEV_STATUS_FETCH_CMD		0x00000002
#define AESDEV_INTR			0x008
#define AESDEV_INTR_ENABLE		0x00c

#define AESDEV_AES_KEY(i)		(0x010 + (i))
#define AESDEV_AES_DATA(i)		(0x020 + (i))
#define AESDEV_AES_STATE(i)		(0x030 + (i))

#define AESDEV_XFER_IN_PTR		0x040
#define AESDEV_XFER_OUT_PTR		0x044
#define AESDEV_XFER_STATE_PTR		0x048
#define AESDEV_XFER_TASK		0x04c

#define AESDEV_TASK(count, intr, save, mode)		((count) << 12 | (intr) << 4 | (save) << 3 | (mode))
#define AESDEV_TASK_MODE(task)          ((task) & 7)
#define AESDEV_TASK_SAVE(task)          ((task) >> 3 & 1)
#define AESDEV_TASK_INTR(task)          ((task) >> 4 & 0xff)
#define AESDEV_TASK_COUNT(task)         ((task) >> 12 & 0xfffff)
#define AESDEV_TASK_ACTIVE(task)        (!!((task) & ~0x7))

#define AESDEV_MODE_ECB_ENCRYPT		0
#define AESDEV_MODE_ECB_DECRYPT		1
#define AESDEV_MODE_CBC_ENCRYPT		2
#define AESDEV_MODE_CBC_DECRYPT		3
#define AESDEV_MODE_CFB_ENCRYPT		4
#define AESDEV_MODE_CFB_DECRYPT		5
#define AESDEV_MODE_OFB			6
#define AESDEV_MODE_CTR			7

#define AESDEV_CMD_BEGIN_PTR		0x050
#define AESDEV_CMD_END_PTR		0x054
#define AESDEV_CMD_READ_PTR		0x058
#define AESDEV_CMD_WRITE_PTR		0x05c

#define AESDEV_AES_KEY_SIZE		0x10
#define AESDEV_AES_BLOCK_SIZE		0x10

#endif
