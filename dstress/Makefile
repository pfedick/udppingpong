EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../CommonLib/include -I../ppl7/include

CFLAGS		=  -ggdb -O2 -Wall  $(INCLUDE)  -I/usr/local/include -DHAVE_CONFIG_H  -pthread \
	      \
	$(EXTRA_CFLAGS)


LIBS = ../ppl7/release/libppl7.a   -lresolv -lresolv -lresolv -lresolv -lresolv -lresolv -lresolv -lm  -pthread  \
	  -lpcre  -lrt -lstdc++

LIBDEP = ../ppl7/release/libppl7.a


UNAME := $(shell uname)
ifeq "$(UNAME)" "FreeBSD"
LIBS += -lkvm
endif

OBJECTS = build/main.o \
	build/mkquery.o \
	build/rawsocketsender.o \
	build/rawsocketreceiver.o \
	build/DNSSenderThread.o \
	build/DNSReceiverThread.o \
	build/payload.o \
	build/packet.o  

all: dstress

dstress: $(OBJECTS) Makefile $(LIBDEP) ../CommonLib/build/libdnsperftest.a
	$(CXX) -O -o dstress $(CFLAGS) $(OBJECTS)  ../CommonLib/build/libdnsperftest.a $(LIBS)



build/main.o: src/main.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/main.o -c src/main.cpp

build/mkquery.o: src/mkquery.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/mkquery.o -c src/mkquery.cpp

build/packet.o: src/packet.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/packet.o -c src/packet.cpp

build/rawsocketsender.o: src/rawsocketsender.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/rawsocketsender.o -c src/rawsocketsender.cpp

build/rawsocketreceiver.o: src/rawsocketreceiver.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/rawsocketreceiver.o -c src/rawsocketreceiver.cpp

build/payload.o: src/payload.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/payload.o -c src/payload.cpp

build/DNSSenderThread.o: src/DNSSenderThread.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/DNSSenderThread.o -c src/DNSSenderThread.cpp

build/DNSReceiverThread.o: src/DNSReceiverThread.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/DNSReceiverThread.o -c src/DNSReceiverThread.cpp

clean:
	-rm -rf build dstress
