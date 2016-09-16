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
#include "../include/udpecho.h"


AgentDaemon::AgentDaemon()
{
	Log.openSyslog("DNSPerfTestSensor");
	Log.setLogLevel(ppl7::Logger::ERR,10);
	Log.setLogLevel(ppl7::Logger::CRIT,10);
	Log.setLogLevel(ppl7::Logger::INFO,10);
	Log.setLogLevel(ppl7::Logger::DEBUG,10);
}

AgentDaemon::~AgentDaemon()
{

}

void AgentDaemon::help()
{

}

int AgentDaemon::main(int argc, char **argv)
{
	try {
		if (ppl7::HaveArgv(argc,argv,"-c")) {
			conf.loadFromFile(ppl7::GetArgv(argc,argv,"-c"));
		} else {
			if (ppl7::File::exists("dnsperftest.conf")) conf.loadFromFile("dnsperftest.conf");
			else {
				printf ("ERROR: no configuration file found\n");
				help();
				return 1;
			}
		}
	} catch (const ppl7::Exception &ex) {
		printf ("ERROR: Could not load configuration file\n");
		ex.print();
		help();
		return 1;
	}

	catchSignal(ppl7::Signal::Signal_SIGINT);
	catchSignal(ppl7::Signal::Signal_SIGHUP);
	catchSignal(ppl7::Signal::Signal_SIGTERM);
	catchSignal(ppl7::Signal::Signal_SIGPIPE);


	Log.print(ppl7::Logger::INFO,1,__FILE__,__LINE__,ppl7::ToString("Starting, Listen on: %s:%u",(const char*)conf.InterfaceName,conf.InterfacePort));
	try {
		bind(conf.InterfaceName,conf.InterfacePort);
		listen();
	} catch (const ppl7::Exception &e) {
		Log.printException(__FILE__,__LINE__,e);
		Log.print(ppl7::Logger::CRIT,1,__FILE__,__LINE__,"Terminating after Exception");
		return 1;
	}
	Clients.signalStopThreads();
	Clients.stopThreads();
	Sensor.threadStop();
	Log.print(ppl7::Logger::INFO,1,__FILE__,__LINE__,"Terminating normal");
	return 0;
}

void AgentDaemon::signalHandler(ppl7::Signal::SignalType sig)
{
	switch (sig) {
		case Signal_SIGINT:
		case Signal_SIGHUP:
		case Signal_SIGTERM:
			Log.print(ppl7::Logger::INFO,1,__FILE__,__LINE__,"SIGINT, SIGTERM or SIGHUP received, shutting down...");
			//Shutdown=true;
			signalStopListen();
			// TODO:
			//Clients.SignalStop();
			break;
		case Signal_SIGPIPE:
			break;
		default:
			break;
	}
	catchSignal(sig);
}


int AgentDaemon::receiveConnect(ppl7::TCPSocket *socket, const ppl7::String &host, int port)
{
	Log.print(ppl7::Logger::INFO,1,__FILE__,__LINE__,ppl7::ToString("Connect from: %s:%u",(const char*)host,port));
	try {
		AgentClient *client=new AgentClient(this, socket, &Log, host, port);
		//size_t stack=client->threadGetMinimumStackSize();
		//if (stack<1024*1204) stack=1024*1024;
		//client->threadSetStackSize(stack);
		client->threadStart();
		Clients.addThread(client);
		return 1;
	} catch (const ppl7::Exception &e) {
		e.print();
		Log.printException(__FILE__,__LINE__,e);
		Log.print(ppl7::Logger::ERR,1,__FILE__,__LINE__,"Disconnect client after Exception");
	}
	return 0;
}

void AgentDaemon::removeClient(ppl7::Thread *thread)
{
	Clients.removeThread(thread);
}


void AgentDaemon::startSensor()
{
	if (!Sensor.threadIsRunning()) {
		Sensor.threadStart();
		Log.print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,"SensorThread started");
	}
}

void AgentDaemon::stopSensor()
{
	Sensor.threadStop();
	Log.print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,"SensorThread stopped");
}

void AgentDaemon::getSensorData(std::list<SystemStat> &data)
{
	Sensor.getSensorData(data);
}

void AgentDaemon::startUDPEchoServer(size_t PacketSize, size_t num_threads, bool disable_responses)
{
	stopUDPEchoServer();
	Log.print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,
			ppl7::ToString("Starting UDPEchoServer @ %s:%d",
					(const char*)conf.UDPEchoInterfaceName, conf.UDPEchoInterfacePort));
	UDPEchoServer.setFixedResponsePacketSize(PacketSize);
	UDPEchoServer.disableResponses(disable_responses);
	UDPEchoServer.setInterface(conf.UDPEchoInterfaceName, conf.UDPEchoInterfacePort);
	UDPEchoServer.start(num_threads);
	Log.print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,"UDPEchoServer started");
}

void AgentDaemon::stopUDPEchoServer()
{
	if (UDPEchoServer.isRunning()) {
		Log.print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,"Stopping UDPEchoServer");
		UDPEchoServer.stop();
		Log.print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,"UDPEchoServer stopped");
	}
}

void AgentDaemon::getUDPEchoServerData(std::list<UDPEchoCounter> &data)
{
	UDPEchoServer.getStats(data);
}
