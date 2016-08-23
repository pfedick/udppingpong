#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>

#include "sensordaemon.h"
#include <dnsperftest_sensor.h>


SensorThread::SensorThread()
{

}

SensorThread::~SensorThread()
{
}

void SensorThread::run()
{
	printf ("SensorThread::run\n");
	while(!threadShouldStop()) {
		SystemStat stat;
		sampleSensorData(stat);
		mutex.lock();
		samples.push_back(stat);
		mutex.unlock();
		printf ("sensor data sampled\n");
		double start=ppl7::GetMicrotime();
		threadSleep(1000);
		printf ("SensorThread::run sleep-Time: %0.3f s\n",ppl7::GetMicrotime()-start);
	}
	printf ("SensorThread::ended\n");
}


void SensorThread::getSensorData(std::list<SystemStat> &data)
{
	mutex.lock();
	data=samples;
	samples.clear();
	mutex.unlock();
}
