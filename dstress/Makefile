EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../CommonLib/include
CFLAGS	= -ggdb -O2 -Wall  $(INCLUDE) `ppl7-config --cflags release` $(EXTRA_CFLAGS)
LIBS	= `ppl7-config --libs release` -lrt -lstdc++
LIBDEP  := $(shell ppl7-config --ppllib release)

UNAME := $(shell uname)
ifeq "$(UNAME)" "FreeBSD"
LIBS += -lkvm
endif

OBJECTS = build/main.o \
	build/mkquery.o \
	build/rawsocket.o \
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

build/rawsocket.o: src/rawsocket.cpp Makefile include/dstress.h ../CommonLib/include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/rawsocket.o -c src/rawsocket.cpp

clean:
	-rm -rf build dstress
