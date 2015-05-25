#include "cyclic_buf.h"

#include <linux/errno.h>
#include <linux/slab.h>

int cbuf_init(struct cyclic_buf* cb, int length) {
	cb->buf = kmalloc(length, GFP_KERNEL);
	if (!cb->buf) {
		return -ENOMEM;
	}
	sema_init(&cb->sem_read, 1);
	sema_init(&cb->sem_write, 0);
	cb->length = length;
	cb->used = 0;
	return 0;
}

void cbuf_destroy(struct cyclic_buf* cb) {
	kfree(cb->buf);
}

int cbuf_read_nonblock(struct cyclic_buf* cb, void* buf, int length) {
	int to_read = length > cb->used ? cb->used : length;

	if (cb->used == 0) {
		return -EWOULDBLOCK;
	}
	memcpy(buf, cb->buf, to_read);
	memmove(cb->buf, cb->buf + to_read, cb->used - to_read);
	cb->used -= to_read;
	up(&cb->sem_write);
	return to_read;
}

int cbuf_write_nonblock(struct cyclic_buf* cb, void* buf, int length) {
	int rem = cb->length - cb->used;
	int to_write = length > rem ? rem : length;

	if (cb->used == cb->length) {
		return -EWOULDBLOCK;
	}
	// writing logic
	memcpy(cb->buf + cb->used, buf, to_write);
	cb->used += to_write;
	up(&cb->sem_read);
	return to_write;
}

int cbuf_read(struct cyclic_buf* cb, void* buf, int length) {
	int n;
	while ((n = cbuf_read_nonblock(cb, buf, length)) == -EWOULDBLOCK) {
		if (down_interruptible(&cb->sem_read))
			return -ERESTARTSYS;
	}
	return n;
}

int cbuf_write(struct cyclic_buf* cb, void* buf, int length) {
	int n;
	while ((n = cbuf_write_nonblock(cb, buf, length)) == -EWOULDBLOCK) {
		if (down_interruptible(&cb->sem_write))
			return -ERESTARTSYS;
	}
	return n;
}
