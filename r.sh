#!/bin/sh
rmmod ./aesdev.ko
# sleep 1
insmod ./aesdev.ko
tail -n 60 -f /var/log/kern.log
