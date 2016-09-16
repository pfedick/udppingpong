EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude
CFLAGS	= -ggdb -O2 -Wall  $(INCLUDE) `ppl7-config --cflags release` $(EXTRA_CFLAGS)
LIBS	= `ppl7-config --libs release` -lrt -lstdc++
LIBDEP  := $(shell ppl7-config --ppllib release)

OBJECTS = build/SampleSensorData.o build/Communicator.o build/UDPEchoCounter.o

build/libdnsperftest.a: $(OBJECTS) Makefile $(LIBDEP)
	ar -r build/libdnsperftest.a $(OBJECTS)

build/SampleSensorData.o: src/SampleSensorData.cpp Makefile include/dnsperftest_sensor.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/SampleSensorData.o -c src/SampleSensorData.cpp

build/Communicator.o: src/Communicator.cpp Makefile include/dnsperftest_communicator.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/Communicator.o -c src/Communicator.cpp

build/UDPEchoCounter.o: src/UDPEchoCounter.cpp Makefile include/dnsperftest_sensor.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/UDPEchoCounter.o -c src/UDPEchoCounter.cpp

clean:
	-rm -rf build