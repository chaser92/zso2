#!/bin/sh
rmmod ./aes.ko
# sleep 1
insmod ./aes.ko
tail -n 60 -f /var/log/kern.log
