/*
 * dnsperftest_sensor.h
 *
 *  Created on: 21.07.2016
 *      Author: patrickf
 */

#ifndef INCLUDE_SENSORDAEMON_H_
#define INCLUDE_SENSORDAEMON_H_


class SensorDaemon : private ppl7::TCPSocket, private ppl7::Signal
{
	private:
		void help();
		ppl7::Logger		Log;
		ppl7::ThreadPool	Clients;

	public:
		SensorDaemon();
		~SensorDaemon();
		int main(int argc, char **argv);
		void removeClient(ppl7::Thread *thread);

		virtual int receiveConnect(ppl7::TCPSocket *socket, const ppl7::String &host, int port);
		virtual void signalHandler(ppl7::Signal::SignalType sig);

};

class SensorClient : public ppl7::Thread
{
	private:
		ppl7::Mutex		Mutex;
		SensorDaemon	*Main;
		ppl7::TCPSocket	*Socket;
		ppl7::Logger	*Log;
		ppl7::String	RemoteHost;
		int				RemotePort;
		ppluint64		LastActivity;
		ppl7::SocketMessage msg;

		void answerFailed(const ppl7::String &error, const ppl7::AssocArray &payload = ppl7::AssocArray());
		void answerOk(const ppl7::AssocArray &payload = ppl7::AssocArray());

	public:
		SensorClient(SensorDaemon *main, ppl7::TCPSocket *socket, ppl7::Logger *log, const ppl7::String &host, int port);
		~SensorClient();
		virtual void run();
		void dispatchMessage(const ppl7::AssocArray &msg);

		void cmdPing(const ppl7::AssocArray &msg);
		void cmdProxyTo(const ppl7::AssocArray &msg);

};



#endif /* INCLUDE_SENSORDAEMON_H_ */
