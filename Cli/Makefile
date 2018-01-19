EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../CommonLib/include
CFLAGS	= -ggdb -O2 -Wall  $(INCLUDE) `ppl7-config --cflags release` $(EXTRA_CFLAGS)
LIBS	= `ppl7-config --libs release` -lrt -lstdc++
LIBDEP  := $(shell ppl7-config --ppllib release)

UNAME := $(shell uname)
ifeq "$(UNAME)" "FreeBSD"
LIBS += -lkvm
endif


OBJECTS = build/main.o

dnsperftest: $(OBJECTS) Makefile $(LIBDEP) ../CommonLib/build/libdnsperftest.a
	$(CXX) -O -o dnsperftest $(CFLAGS) $(OBJECTS) ../CommonLib/build/libdnsperftest.a $(LIBS)

build/main.o: src/main.cpp Makefile
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/main.o -c src/main.cpp


clean:
	-rm -rf build dnsperftest
