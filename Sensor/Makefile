EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../CommonLib/include
CFLAGS	= -ggdb -O2 -Wall  $(INCLUDE) `ppl7-config --cflags release` $(EXTRA_CFLAGS)
LIBS	= `ppl7-config --libs release` -lrt -lstdc++
LIBDEP  := $(shell ppl7-config --ppllib release)

OBJECTS = build/main.o build/SensorDaemon.o build/SensorClient.o

dnsperftest_sensor: $(OBJECTS) Makefile $(LIBDEP) ../CommonLib/build/libdnsperftest.a
	$(CXX) -O -o dnsperftest_sensor $(CFLAGS) $(OBJECTS) ../CommonLib/build/libdnsperftest.a $(LIBS)

build/main.o: src/main.cpp Makefile include/sensordaemon.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/main.o -c src/main.cpp

build/SensorDaemon.o: src/SensorDaemon.cpp Makefile include/sensordaemon.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/SensorDaemon.o -c src/SensorDaemon.cpp

build/SensorClient.o: src/SensorClient.cpp Makefile include/sensordaemon.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/SensorClient.o -c src/SensorClient.cpp

clean:
	-rm -rf build dnsperftest_sensor