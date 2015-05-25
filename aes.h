
#ifndef _AES_H_
#define _AES_H_

#define MODE_NOT_SET -1
#define CBUF_SIZE 4096
#define AES_NUM_DEVICES 16
#define AESDEV_COMMAND_GET_STATE 8

extern struct class *aes_class;
extern int aes_major;     /* main.c */
extern int aes_nr_devs;

ssize_t aes_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t aes_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
long    aes_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);


#endif /* _AES_H_ */
