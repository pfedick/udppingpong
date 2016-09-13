#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>

#include <dnsperftest_sensor.h>
#include "../include/dnsperftest_agent.h"


SensorThread::SensorThread()
{

}

SensorThread::~SensorThread()
{
}

void SensorThread::run()
{
	while(!threadShouldStop()) {
		SystemStat stat;
		sampleSensorData(stat);
		mutex.lock();
		samples.push_back(stat);
		mutex.unlock();
		threadSleep(1000);
	}
}


void SensorThread::getSensorData(std::list<SystemStat> &data)
{
	mutex.lock();
	data=samples;
	samples.clear();
	mutex.unlock();
}
