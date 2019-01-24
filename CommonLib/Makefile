EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../ppl7/include

CFLAGS		=  -ggdb -O2 -Wall  $(INCLUDE)  -I/usr/local/include -DHAVE_CONFIG_H  -pthread \
	      \
	$(EXTRA_CFLAGS)


LIBS = ../ppl7/release/libppl7.a   -lresolv -lresolv -lresolv -lresolv -lresolv -lresolv -lresolv -lm  -pthread  \
	  -lpcre   -lrt -lstdc++

LIBDEP = ../ppl7/release/libppl7.a

OBJECTS = build/SampleSensorData.o build/Communicator.o build/UDPEchoCounter.o \
		build/UDPEchoBouncer.o build/UDPEchoBouncerThread.o \
		build/UDPEchoSenderThread.o build/UDPEchoReceiverThread.o \
		build/UDPEchoSender.o
		

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

build/UDPEchoBouncer.o: src/UDPEchoBouncer.cpp Makefile include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/UDPEchoBouncer.o -c src/UDPEchoBouncer.cpp

build/UDPEchoSender.o: src/UDPEchoSender.cpp Makefile include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/UDPEchoSender.o -c src/UDPEchoSender.cpp

build/UDPEchoBouncerThread.o: src/UDPEchoBouncerThread.cpp Makefile include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/UDPEchoBouncerThread.o -c src/UDPEchoBouncerThread.cpp

build/UDPEchoSenderThread.o: src/UDPEchoSenderThread.cpp Makefile include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/UDPEchoSenderThread.o -c src/UDPEchoSenderThread.cpp

build/UDPEchoReceiverThread.o: src/UDPEchoReceiverThread.cpp Makefile include/udpecho.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/UDPEchoReceiverThread.o -c src/UDPEchoReceiverThread.cpp


clean:
	-rm -rf build
