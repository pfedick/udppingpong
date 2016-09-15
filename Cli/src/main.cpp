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

void getSensorData(Communicator &comm, SystemStat &previous)
{
	std::list<SystemStat> data;
	comm.getSensorData(data);
	printf ("Wir haben %zd Datensaetze\n",data.size());
	std::list<SystemStat>::const_iterator it;
	for (it=data.begin();it!=data.end();++it) {
		printf ("==================================================== CPU: %0.2f\n",
				SystemStat::Cpu::getUsage(previous.cpu,(*it).cpu));
		(*it).print();
		previous=(*it);
	}
}

int main(int argc, char**argv)
{
	Communicator comm;
	try {
		comm.connect("patrickf-xm3.office.denic.de",50000);
		//comm.ping();
		//printf ("proxy aktivieren\n");
		//comm.proxyTo("patrickf-xm3.office.denic.de",40000);
		printf ("ping\n");
		comm.ping();
		printf ("ping done\n");
		comm.startSensor();
		comm.startUDPEchoServer(1024,2,false);

		std::list<SystemStat> sensordata_list;
		SystemStat sensordata_previous;
		comm.getSensorData(sensordata_list);
		printf ("Wir haben initial %zd Datensaetze\n",sensordata_list.size());
		sensordata_previous=(*sensordata_list.begin());

		for (int i=0;i<6;i++) {
			sleep(1);
			getSensorData(comm, sensordata_previous);
		}
		comm.stopUDPEchoServer();
		printf ("next ping\n");
		comm.ping();
		printf ("ping done\n");
		comm.stopSensor();
		//getSensorData(comm);
		return 0;
	} catch (const ppl7::Exception &ex) {
		ex.print();
		return 1;
	}
}
