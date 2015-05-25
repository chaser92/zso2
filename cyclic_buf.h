#ifndef __CYCLIC_BUF_H_
#define __CYCLIC_BUF_H_

#include <linux/semaphore.h>

struct cyclic_buf {
	void* buf;
	int length;
	int used;
	struct semaphore sem_read;
	struct semaphore sem_write;
};

int cbuf_init(struct cyclic_buf* cb, int length);
void cbuf_destroy(struct cyclic_buf* cb);
int cbuf_has_space(struct cyclic_buf* cb, int space);
int cbuf_read_nonblock(struct cyclic_buf* cb, void* buf, int length);
int cbuf_wait_for_write(struct cyclic_buf* cb);
int cbuf_read(struct cyclic_buf* cb, void* buf, int length);
int cbuf_write_nonblock(struct cyclic_buf* cb, void* buf, int length);

#endif