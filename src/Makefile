VERSION=unknown

# STAGING_DIR=/srv/preware/build/staging/i686
# CC=/srv/preware/build/toolchain/i686-unknown-linux-gnu/build/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc

STAGING_DIR=/srv/preware/build/staging/armv7
CC=/opt/PalmPDK/arm-gcc/bin/arm-none-linux-gnueabi-gcc

CPPFLAGS := -g -DVERSION=\"${VERSION}\" -I${STAGING_DIR}/usr/include/glib-2.0 -I${STAGING_DIR}/usr/lib/glib-2.0/include -I${STAGING_DIR}/usr/include
LDFLAGS  := -g -L${STAGING_DIR}/usr/lib -llunaservice -lmjson -lglib-2.0

syshealth: syshealth.o luna_service.o luna_methods.o

install: syshealth
#	- ssh root@webos killall org.webosinternals.syshealth
#	scp govnah root@webos:/var/usr/sbin/org.webosinternals.syshealth.new
#	ssh root@webos mv /var/usr/sbin/org.webosinternals.syshealth.new /var/usr/sbin/org.webosinternals.syshealth
	novacom put file://home/root/syshealth < syshealth

clobber:
	rm -rf *.o syshealth
