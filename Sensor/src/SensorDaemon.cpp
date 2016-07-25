#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>

#include "../include/sensordaemon.h"
#include <dnsperftest_sensor.h>


SensorDaemon::SensorDaemon()
{

}

SensorDaemon::~SensorDaemon()
{

}

void SensorDaemon::help()
{

}

int SensorDaemon::main(int argc, char **argv)
{
	SystemStat sample1, sample2;
	sampleSensorData(sample1);
	for(;;)
	{
		sleep(1);
		sampleSensorData(sample2);
		printf("The current CPU utilization is : %f %%, FreeMemory: %ld\n",SystemStat::Cpu::getUsage(sample1.cpu,sample2.cpu),sample2.sysinfo.freeram/1024);
		SystemStat::Network::getDelta(sample1.net_receive, sample2.net_receive).print();
		SystemStat::Network::getDelta(sample1.net_transmit, sample2.net_transmit).print();

		sample1=sample2;
	}
	return 0;
}
