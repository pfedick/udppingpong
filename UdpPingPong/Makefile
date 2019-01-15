EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../CommonLib/include -I../ppl7/include

CFLAGS		=  -ggdb -O2 -Wall  $(INCLUDE)  -I/usr/local/include -DHAVE_CONFIG_H  -pthread \
	-I/usr/include      \
	$(EXTRA_CFLAGS)


LIBS = ../ppl7/release/libppl7.a  -lssl -lcrypto -lresolv -lresolv -lresolv -lresolv -lresolv -lresolv -lresolv -lm  -pthread  \
	-L/usr/lib -lz -lbz2 -lpcre   -lrt -lstdc++

LIBDEP = ../ppl7/release/libppl7.a



UNAME := $(shell uname)
ifeq "$(UNAME)" "FreeBSD"
LIBS += -lkvm
endif


all: pingpong_sender pingpong_bouncer

pingpong_sender: build/sender.o Makefile $(LIBDEP) ../CommonLib/build/libdnsperftest.a
	$(CXX) -O -o pingpong_sender $(CFLAGS) build/sender.o  ../CommonLib/build/libdnsperftest.a $(LIBS)

pingpong_bouncer: build/bouncer.o Makefile $(LIBDEP) ../CommonLib/build/libdnsperftest.a
	$(CXX) -O -o pingpong_bouncer $(CFLAGS) build/bouncer.o  ../CommonLib/build/libdnsperftest.a $(LIBS)


build/sender.o: src/sender.cpp Makefile ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/sender.o -c src/sender.cpp

build/bouncer.o: src/bouncer.cpp Makefile ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/bouncer.o -c src/bouncer.cpp


clean:
	-rm -rf build dnsperftest
