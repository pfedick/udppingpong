EXTRA_CFLAGS = 
INCLUDE	= -I. -I../include
CFLAGS	= -ggdb -O2 -Wall  $(INCLUDE) `ppl7-config --cflags release` $(EXTRA_CFLAGS)
LIBS	= `ppl7-config --libs release` -lrt -lstdc++
UNAME := $(shell uname)
ifeq "$(UNAME)" "FreeBSD"
LIBS += -lkvm
endif


LIBDEP  := $(shell ppl7-config --ppllib release)

OBJECTS = build/main.o

testsuite: $(OBJECTS) Makefile $(LIBDEP) ../build/libdnsperftest.a
	$(CXX) -O -o testsuite $(CFLAGS) $(OBJECTS) ../build/libdnsperftest.a $(LIBS)

build/main.o: src/main.cpp Makefile ../include/dnsperftest_sensor.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/main.o -c src/main.cpp


clean:
	-rm -rf build testsuite
