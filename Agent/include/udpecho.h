#ifndef UDPECHO_H_
#define UDPECHO_H_

#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dnsperftest_sensor.h>

typedef struct {
		ppluint64 id;
		double time;
} PACKET;

class UDPEchoSender
{
	private:

		class Results
		{
			public:
				int			queryrate;
				ppluint64	counter_send;
				ppluint64	counter_received;
				ppluint64	bytes_received;
				ppluint64	counter_errors;
				ppluint64	packages_lost;
				double		duration;
				double		rtt_total;
				double		rtt_min;
				double		rtt_max;
		};
		ppl7::ThreadPool threadpool;
		ppl7::String Ziel;
		ppl7::String Quelle;
		ppl7::File CSVFile;
		int Packetsize;
		int Laufzeit;
		int Timeout;
		int ThreadCount;
		int Zeitscheibe;
		bool ignoreResponses;

		void openCSVFile(const ppl7::String Filename);
		void run(int queryrate);
		void presentResults(const UDPEchoSender::Results &result);
		void saveResultsToCsv(const UDPEchoSender::Results &result);
		void prepareThreads();
		void getResults(UDPEchoSender::Results &result);
		ppl7::Array getQueryRates(const ppl7::String &QueryRates);

	public:
		UDPEchoSender();
		void help();
		int main(int argc, char**argv);
};

class UDPEchoReceiverThread : public ppl7::Thread
{
	private:
		int sockfd;
		ppl7::ByteArray recbuffer;
		ppluint64 counter_received;
		ppluint64 bytes_received;

		double rtt_total, rtt_min, rtt_max;

		void receivePackets();

	public:
		UDPEchoReceiverThread();
		~UDPEchoReceiverThread();
		void setSocketDescriptor(int sockfd);
		void run();
		void resetCounter();
		ppluint64 getPacketsReceived() const;
		ppluint64 getBytesReceived() const;
		double getRoundTripTimeAverage() const;
		double getRoundTripTimeMin() const;
		double getRoundTripTimeMax() const;

};



class UDPEchoSenderThread : public ppl7::Thread
{
	private:
		ppl7::ByteArray buffer;
		ppl7::UDPSocket Socket;
		UDPEchoReceiverThread receiver;

		ppl7::String destination;
		size_t packetsize;
		ppluint64 queryrate;
		int runtime;
		int timeout;
		double Zeitscheibe;
		ppluint64 counter_send, errors;
		double duration;
		int sockfd;
		bool ignoreResponses;

		void sendPacket();
		void waitForTimeout();

		void runWithoutRateLimit();
		void runWithRateLimit();

	public:
		UDPEchoSenderThread();
		~UDPEchoSenderThread();
		void setDestination(const ppl7::String &destination);
		void setPacketsize(size_t size);
		void setRuntime(int seconds);
		void setTimeout(int seconds);
		void setQueryRate(ppluint64 qps);
		void setZeitscheibe(int ms);
		void setIgnoreResponses(bool flag);
		void run();
		ppluint64 getPacketsSend() const;
		ppluint64 getPacketsReceived() const;
		ppluint64 getBytesReceived() const;
		ppluint64 getErrors() const;
		double getDuration() const;
		double getRoundTripTimeAverage() const;
		double getRoundTripTimeMin() const;
		double getRoundTripTimeMax() const;
};



class UDPEchoBouncer : private ppl7::Thread
{
	private:
		ppl7::ThreadPool threadpool;
		bool noEcho;
		struct sockaddr_in servaddr;
		ppl7::SockAddr sockaddr;
		int sockfd;
		size_t packetSize;

		ppl7::Mutex	counterMutex;
		std::list<UDPEchoCounter>	samples;


		ppl7::SockAddr getSockAddr(const ppl7::String &Hostname, int Port);
		void startBouncerThreads(size_t ThreadCount);
		void bind(const ppl7::SockAddr &sockaddr);
		void createSocket();

	public:
		UDPEchoBouncer();
		~UDPEchoBouncer();
		void run();

		void setFixedResponsePacketSize(size_t size);
		void setInterface(const ppl7::String &InterfaceName, int Port);
		void disableResponses(bool flag);
		void start(size_t num_threads);
		void stop();
		bool isRunning();

		void clearStats();
		void getStats(std::list<UDPEchoCounter> &data);

};



class UDPEchoBouncerThread : public ppl7::Thread
{
	private:
		int sockfd;
		ppl7::ByteArray buffer;
		void *pBuffer;
		ppl7::Mutex mutex;
		bool noEcho;
		UDPEchoCounter counter;
		size_t packetSize;


	public:
		UDPEchoBouncerThread();
		~UDPEchoBouncerThread();
		void setNoEcho(bool flag);
		void setPacketSize(size_t bytes);
		void setSocketDescriptor(int sockfd);
		void run();
		UDPEchoCounter getAndClearCounter();

};


#endif /* UDPECHO_H_ */
