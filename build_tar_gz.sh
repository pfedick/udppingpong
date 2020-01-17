#!/bin/sh
VERSION=${VERSION:=$BUILD_NUMBER}
VERSION=${VERSION:=trunk}
RPM_RELEASE=${RPM_RELEASE:=1}
SOURCE=`pwd`
BUILD=`pwd`/build

create_dir ()
{
	mkdir -p $1
	if [ $? -ne 0 ] ; then
		echo "ERROR: could not create: $1"
		exit 1
	fi
}

rm -rf distfiles build/dist
create_dir distfiles
create_dir build/dist
create_dir build/dist/udppingpong-$VERSION-$RPM_RELEASE

find *.m4 autoconf include/sender.h \
	include/sensor.h include/udpecho.h include/config.h.in src *.TXT configure Makefile.in README.md \
	ppl7/src ppl7/Makefile.in ppl7/*.TXT \
	ppl7/include/*.in ppl7/include/ppl7.h ppl7/include/compat_ppl7.h \
	ppl7/include/ppl7-algorithms.h ppl7/include/ppl7-crypto.h \
	ppl7/include/ppl7-exceptions.h ppl7/include/ppl7-inet.h ppl7/include/ppl7-types.h \
	ppl7/include/prolog_ppl7.h ppl7/include/socket_ppl7.h ppl7/include/threads_ppl7.h \
	| cpio -pdmv ${SOURCE}/build/dist/udppingpong-$VERSION-$RPM_RELEASE

cd ${SOURCE}/build/dist
tar -czf ${SOURCE}/distfiles/udppingpong-$VERSION-$RPM_RELEASE.tar.gz udppingpong-$VERSION-$RPM_RELEASE
	


