#ifndef INCLUDE_DNSPERFTEST_AGENT_H_
#define INCLUDE_DNSPERFTEST_AGENT_H_

#include <list>
#include <ppl7.h>
#include <ppl7-inet.h>

#include <dnsperftest_sensor.h>
#include "udpecho.h"
class UDPEchoBouncer;

PPL7EXCEPTION(NoConfigurationFileFound, FileNotFoundException);

class SensorThread : public ppl7::Thread
{
	private:
		ppl7::Mutex				mutex;
		std::list<SystemStat>	samples;

	public:
		SensorThread();
		~SensorThread();
		virtual void run();
		void getSensorData(std::list<SystemStat> &data);
};

class Config
{
	private:
	public:
		Config();
		void loadFromFile(const ppl7::String &Filename);
		void loadFromFile(const std::list<ppl7::String> &FileList);

		ppl7::String 	InterfaceName;
		int 			InterfacePort;

};

class UDPEchoBouncerServer : public UDPEchoBouncer, public ppl7::Thread
{
private:
	std::list<UDPEchoCounter>	samples;
	ppl7::Mutex	counterMutex;
public:
	void run();
	void clearStats();
	void getStats(std::list<UDPEchoCounter> &data);
};


class AgentDaemon : private ppl7::TCPSocket, private ppl7::Signal
{
	private:
		void help();
		ppl7::Logger		Log;
		ppl7::ThreadPool	Clients;
		SensorThread		Sensor;
		Config				conf;
		UDPEchoBouncerServer	UDPEchoServer;
		UDPEchoSender			udpechosender;


	public:
		AgentDaemon();
		~AgentDaemon();
		int main(int argc, char **argv);
		void removeClient(ppl7::Thread *thread);

		virtual int receiveConnect(ppl7::TCPSocket *socket, const ppl7::String &host, int port);
		virtual void signalHandler(ppl7::Signal::SignalType sig);

		void startSensor();
		void stopSensor();
		void getSensorData(std::list<SystemStat> &data);


		void startUDPEchoServer(const ppl7::String &hostname, int port, size_t num_threads, size_t PacketSize=0, bool disable_responses=false);
		void stopUDPEchoServer();
		void getUDPEchoServerData(std::list<UDPEchoCounter> &data);

		void startUDPEchoSender();
		void stopUDPEchoSender();
		void getUDPEchoSenderData();



};

class AgentClient : public ppl7::Thread
{
	private:
		ppl7::Mutex		Mutex;
		AgentDaemon	*Main;
		ppl7::TCPSocket	*Socket;
		ppl7::Logger	*Log;
		ppl7::String	RemoteHost;
		int				RemotePort;
		ppluint64		LastActivity;
		ppl7::SocketMessage msg;

		void answerFailed(const ppl7::String &error, const ppl7::AssocArray &payload = ppl7::AssocArray());
		void answerOk(const ppl7::AssocArray &payload = ppl7::AssocArray());

	public:
		AgentClient(AgentDaemon *main, ppl7::TCPSocket *socket, ppl7::Logger *log, const ppl7::String &host, int port);
		~AgentClient();
		virtual void run();
		void dispatchMessage(const ppl7::AssocArray &msg);

		void cmdPing(const ppl7::AssocArray &msg);
		void cmdProxyTo(const ppl7::AssocArray &msg);
		void cmdStartSensor();
		void cmdStopSensor();
		void cmdGetSensorData();
		void cmdGetSystemStat();
		void cmdStartUDPEchoServer(const ppl7::AssocArray &params);
		void cmdStopUDPEchoServer();
		void cmdGetUDPEchoServerData();
};




#endif /* INCLUDE_DNSPERFTEST_AGENT_H_ */
