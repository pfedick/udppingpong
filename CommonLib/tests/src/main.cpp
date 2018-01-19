#include "dnsperftest_sensor.h"
#include "dnsperftest_communicator.h"


int main(int argc, char **argv)
{
	SystemStat stat;
	sampleSensorData(stat);
	stat.print();
	return 0;
}
