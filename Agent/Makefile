EXTRA_CFLAGS = 
INCLUDE	= -I. -Iinclude -I../CommonLib/include
CFLAGS	= -ggdb -O2 -Wall  $(INCLUDE) `ppl7-config --cflags release` $(EXTRA_CFLAGS)
LIBS	= `ppl7-config --libs release` -lrt -lstdc++
LIBDEP  := $(shell ppl7-config --ppllib release)

OBJECTS = build/main.o build/AgentDaemon.o build/AgentClient.o \
          build/SensorThread.o build/Config.o

dnsperftest_agent: $(OBJECTS) Makefile $(LIBDEP) ../CommonLib/build/libdnsperftest.a
	$(CXX) -O -o dnsperftest_agent $(CFLAGS) $(OBJECTS) ../CommonLib/build/libdnsperftest.a $(LIBS)

build/main.o: src/main.cpp Makefile include/dnsperftest_agent.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/main.o -c src/main.cpp

build/AgentDaemon.o: src/AgentDaemon.cpp Makefile include/dnsperftest_agent.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/AgentDaemon.o -c src/AgentDaemon.cpp

build/AgentClient.o: src/AgentClient.cpp Makefile include/dnsperftest_agent.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/AgentClient.o -c src/AgentClient.cpp

build/SensorThread.o: src/SensorThread.cpp Makefile include/dnsperftest_agent.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/SensorThread.o -c src/SensorThread.cpp

build/Config.o: src/Config.cpp Makefile include/dnsperftest_agent.h
	mkdir -p build
	$(CXX) $(CFLAGS) -o build/Config.o -c src/Config.cpp

clean:
	-rm -rf build dnsperftest_agent