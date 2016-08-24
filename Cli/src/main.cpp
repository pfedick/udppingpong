#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <list>
#include <dnsperftest_communicator.h>
#include <dnsperftest_sensor.h>


static void help()
{

}

int main(int argc, char**argv)
{
	Communicator comm;
	comm.connect("patrickf-xm2.office.denic.de",40000);
	comm.ping();
	printf ("proxy aktivieren\n");
	comm.proxyTo("patrickf-xm2.office.denic.de",40000);
	printf ("ping\n");
	comm.ping();
	printf ("ping done\n");
	comm.startSensor();
	sleep(3);
	printf ("next ping\n");
	comm.ping();
	printf ("ping done\n");
	comm.stopSensor();
	std::list<SystemStat> data;
	comm.getSensorData(data);
	printf ("Wir haben %zd Datensaetze\n",data.size());
	std::list<SystemStat>::const_iterator it;
	for (it=data.begin();it!=data.end();++it) {
		printf ("====================================================\n");
		(*it).print();
	}

	return 0;
}
