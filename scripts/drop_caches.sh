#!/bin/sh
#
# Writing to this will cause the kernel to drop clean caches, 
# dentries and inodes from memory, causing that memory to become free.
#

sync
echo 3 > /proc/sys/vm/drop_caches

