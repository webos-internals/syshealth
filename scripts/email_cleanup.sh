#!/bin/sh
#
# Clean up email and attachments as webOS leaves them lying around
#

/usr/bin/find /var/luna/data/emails -type f -mtime +7 -exec rm -f {} \;
/usr/bin/find /var/luna/data/attachments -type f -mtime +7 -exec rm -f {} \;

