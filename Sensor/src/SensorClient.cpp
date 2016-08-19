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
