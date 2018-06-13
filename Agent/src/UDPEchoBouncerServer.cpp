#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include <dnsperftest_sensor.h>
#include "../include/dnsperftest_agent.h"
#include "../include/udpecho.h"


void UDPEchoBouncerServer::run()
{
	while (!threadShouldStop()) {
		UDPEchoCounter counter=getCounter();
		counterMutex.lock();
		samples.push_back(counter);
		counterMutex.unlock();
		threadSleep(1000);
	}
}

void UDPEchoBouncerServer::clearStats()
{
	counterMutex.lock();
	samples.clear();
	counterMutex.unlock();
}

void UDPEchoBouncerServer::getStats(std::list<UDPEchoCounter> &data)
{
	counterMutex.lock();
	data=samples;
	samples.clear();
	counterMutex.unlock();
}

