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

void getUDPEchoStats(Communicator &comm)
{
	std::list<UDPEchoCounter> data;
	comm.getUDPEchoServerData(data);
	printf ("Wir haben %zd Datensaetze\n",data.size());
	std::list<UDPEchoCounter>::const_iterator it;
	for (it=data.begin();it!=data.end();++it) {
		printf ("### UDPEcho packets_received: %llu, bytes_received: %llu, packets_send: %llu, bytes_send: %llu\n",
				(*it).packets_received,
				(*it).bytes_received,
				(*it).packets_send,
				(*it).bytes_send);
	}
}


int main(int argc, char**argv)
{
	Communicator comm;
	Communicator lg1;
	try {
		comm.connect("xxx.xxx.xxx.xxx",443);
		lg1.connect("xxx.xxx.xxx.xxx",443);
		//comm.ping();
		//printf ("proxy aktivieren\n");
		//comm.proxyTo("xxx.xxx.xxx.xxx",40000);
		printf ("ping\n");
		if (!comm.ping()) {
			printf ("Ping to SUT failed\n");
		}
		if (!lg1.ping()) {
			printf ("Ping to LG failed\n");
		}

		printf ("ping done\n");
		comm.startSensor();
		lg1.startSensor();
		printf ("Sensoren gestartet\n");
		//comm.startUDPEchoServer(1024,2,false);


		std::list<SystemStat> sensordata_list;
		SystemStat sensordata_previous;
		comm.getSensorData(sensordata_list);
		printf ("Wir haben initial %zd Datensaetze vom SUT\n",sensordata_list.size());

		sensordata_previous=(*sensordata_list.begin());

		for (int i=0;i<5;i++) {
			sleep(1);
			getSensorData(comm, sensordata_previous);
			//getUDPEchoStats(comm);
		}
		//comm.stopUDPEchoServer();
		printf ("next ping\n");
		comm.ping();
		//lg1.ping();
		printf ("ping done\n");
		comm.stopSensor();
		lg1.stopSensor();
		//getSensorData(comm);
		return 0;
	} catch (const ppl7::Exception &ex) {
		ex.print();
		return 1;
	}
}
