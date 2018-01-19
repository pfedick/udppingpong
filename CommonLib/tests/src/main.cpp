#include "dnsperftest_sensor.h"
#include "dnsperftest_communicator.h"


int main(int argc, char **argv)
{
	SystemStat stat;
        try {
	    sampleSensorData(stat);
	    stat.print();
        } catch ( const ppl7::Exception &exp) {
           exp.print();
        }
	return 0;
}
