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

SensorClient::SensorClient(SensorDaemon *main, ppl7::TCPSocket *socket, ppl7::Logger *log, const ppl7::String &host, int port)
{
	this->Main=main;
	this->Socket=socket;
	this->RemoteHost=host;
	this->RemotePort=port;
	this->Log=log;
	LastActivity=ppl7::GetTime();
}


SensorClient::~SensorClient()
{
	if (Socket) delete Socket;
	if (Log) {
		Log->print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,ppl7::ToString("Thread deleted for client: %s:%u",(const char*)RemoteHost,RemotePort));
	}
	if (Main) Main->removeClient(this);
}

void SensorClient::run()
{
	threadDeleteOnExit(1);
	Log->print(ppl7::Logger::DEBUG,1,__FILE__,__LINE__,ppl7::ToString("Thread started for client: %s:%u",(const char*)RemoteHost,RemotePort));
	msg.enableCompression(true);
	while (1) {
		if (threadShouldStop()) {
			Log->print(ppl7::Logger::DEBUG,3,__FILE__,__LINE__,ppl7::ToString("Thread signaled to stop, disconnecting client: %s:%u",(const char*)RemoteHost,RemotePort));
			break;
		}
		try {
			if (Socket->waitForMessage(msg,1,this)) {
				ppl7::AssocArray payload;
				msg.getPayload(payload);
				Log->printArray(ppl7::Logger::DEBUG,10,payload, ppl7::String("received message"));
				dispatchMessage(payload);
			}
		} catch (const ppl7::BrokenPipeException &e) {
			Log->print(ppl7::Logger::ERR,1,__FILE__,__LINE__,ppl7::ToString("Client disconnected: %s:%u",(const char*)RemoteHost,RemotePort));
			break;
		} catch (const ppl7::Exception &e) {
			Log->printException(__FILE__,__LINE__,e);
			Log->print(ppl7::Logger::ERR,1,__FILE__,__LINE__,"Disconnecting client after exception");
			break;
		}
	}
	Log->print(ppl7::Logger::DEBUG,4,__FILE__,__LINE__,ppl7::ToString("Thread ended for client: %s:%u",(const char*)RemoteHost,RemotePort));
}

void SensorClient::answerFailed(const ppl7::String &error, const ppl7::AssocArray &payload)
{
	ppl7::AssocArray res(payload);
	res.set("result","failed");
	res.set("error",error);
	msg.setPayload(res);
	Socket->write(msg);
}

void SensorClient::answerOk(const ppl7::AssocArray &payload)
{
	ppl7::AssocArray res(payload);
	res.set("result","ok");
	msg.setPayload(res);
	Socket->write(msg);
}


void SensorClient::dispatchMessage(const ppl7::AssocArray &msg)
{
	const ppl7::String &command=msg.get("command");
	if (command=="ping") {
		cmdPing(msg);
	} else if (command=="proxyto") {
		cmdProxyTo(msg);
	} else if (command=="startsensor") {
		cmdStartSensor();
	} else if (command=="stopsensor") {
		cmdStopSensor();
	} else if (command=="getsensordata") {
		cmdGetSensorData();
	} else {
		answerFailed(ppl7::ToString("unknown command: ")+command);
	}
}

void SensorClient::cmdPing(const ppl7::AssocArray &msg)
{
	ppl7::AssocArray answer;
	answer.setf("mytime","%lu", ppl7::GetTime());
	answer.set("yourtime",msg.get("mytime"));
	answerOk(answer);
}

void SensorClient::cmdProxyTo(const ppl7::AssocArray &msg)
{
	ppl7::String Host=msg.getString("host");
	int Port=msg.getString("port").toInt();
	int timeout_connect_sec=msg.getString("timeout_connect_sec").toInt();
	int timeout_connect_usec=msg.getString("timeout_connect_usec").toInt();
	int timeout_read_sec=msg.getString("timeout_read_sec").toInt();
	int timeout_read_usec=msg.getString("timeout_read_usec").toInt();
	if (Host.isEmpty()) {
		answerFailed("parameter missing [host]");
		return;
	}
	if (!Port) {
		answerFailed("parameter missing [port]");
		return;
	}
	char *buffer=(char*)malloc(65536);
	if (!buffer) {
		answerFailed("out of memory");
		return;
	}
	ppl7::TCPSocket proxy;
	proxy.setTimeoutConnect(timeout_connect_sec,timeout_connect_usec);
	try {
		proxy.connect(Host, Port);
		proxy.setTimeoutRead(timeout_read_sec,timeout_read_usec);
		proxy.setTimeoutWrite(timeout_read_sec,timeout_read_usec);
		proxy.setBlocking(false);
	} catch (const ppl7::Exception &ex) {
		free(buffer);
		answerFailed(ex.toString());
		return;
	} catch (...) {
		free(buffer);
		answerFailed("unhandled exception occured");
		return;
	}
	answerOk();
	Log->print(ppl7::Logger::DEBUG,1,__FILE__,__LINE__,ppl7::ToString("Proxy startet for client: %s:%u to %s:%u",
			(const char*)RemoteHost,RemotePort,
			(const char*)Host, Port));
	Socket->setBlocking(false);
	try {
		while(!threadShouldStop()) {
			while (Socket->waitForIncomingData(0,100000)) {
				size_t bytes=Socket->read(buffer,65536);
				proxy.write(buffer,bytes);
			}
			while (proxy.waitForIncomingData(0,100000)) {
				size_t bytes=proxy.read(buffer,65536);
				Socket->write(buffer,bytes);
			}

		}
	} catch (...) {

	}
	proxy.disconnect();
	free(buffer);
	threadShouldStop();
	Log->print(ppl7::Logger::DEBUG,1,__FILE__,__LINE__,ppl7::ToString("Proxy stopped for client: %s:%u to %s:%u",
			(const char*)RemoteHost,RemotePort,
			(const char*)Host, Port));

}

void SensorClient::cmdStartSensor()
{
	Main->startSensor();
	answerOk();
}

void SensorClient::cmdStopSensor()
{
	Main->stopSensor();
	answerOk();
}

void SensorClient::cmdGetSensorData()
{
	std::list<SystemStat> data;
	std::list<SystemStat>::const_iterator it;
	Main->getSensorData(data);
	ppl7::AssocArray answer;
	ppl7::AssocArray row;
	for (it=data.begin();it!=data.end();++it) {
		(*it).exportToArray(row);
		answer.set("data/[]",row);
	}
	//answer.list();
	answerOk(answer);
}
