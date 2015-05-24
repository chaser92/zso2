/*
 * scull.h -- definitions for the char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: scull.h,v 1.15 2004/11/04 17:51:18 rubini Exp $
 */

#ifndef _AES_H_
#define _AES_H_

#include <linux/device.h> /* for struct device */

/*
 * The different configurable parameters
 */
extern struct class *aes_class;
extern int aes_major;     /* main.c */
extern int aes_nr_devs;

/*
 * Prototypes for shared functions
 */

int     aes_p_init(dev_t dev);
void    aes_p_cleanup(void);

ssize_t aes_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t aes_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
long    aes_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);


#endif /* _AES_H_ */
