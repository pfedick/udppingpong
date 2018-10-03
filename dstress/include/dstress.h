/*
 * dstress.h
 *
 *  Created on: 27.09.2018
 *      Author: patrickf
 */

#ifndef DSTRESS_INCLUDE_DSTRESS_H_
#define DSTRESS_INCLUDE_DSTRESS_H_

#include <ppl7.h>
#include <ppl7-inet.h>
//#include "udpecho.h"

PPL7EXCEPTION(InvalidDNSQuery, Exception);
PPL7EXCEPTION(UnknownRRType, Exception);
PPL7EXCEPTION(BufferOverflow, Exception);
PPL7EXCEPTION(UnknownDestination, Exception);
PPL7EXCEPTION(InvalidQueryFile, Exception);
PPL7EXCEPTION(UnsupportedIPFamily, Exception);

int MakeQuery(const ppl7::String &query, unsigned char *buffer, size_t buffersize, bool dnssec=false, int udp_payload_size=4096);


class Packet
{
private:
	unsigned char *buffer;
	int buffersize;
	int payload_size;
	bool chksum_valid;

	void updateChecksums();
public:
	Packet();
	~Packet();
	void setSource(const ppl7::String &ip_addr, int port);
	void setDestination(const ppl7::String &ip_addr, int port);
	void setPayload(const void *payload, size_t size);
	void setPayloadDNSQuery(const ppl7::String &query);
	void setId(unsigned short id);

	size_t size() const;
	unsigned char* ptr();

};

class RawSocket
{
private:
	void *buffer;
	int sd;
public:
	RawSocket();
	~RawSocket();
	void setDestination(const ppl7::IPAddress &ip_addr, int port);
	ssize_t send(Packet &pkt);
	ppl7::SockAddr getSockAddr() const;
};

class RawReceiveSocket
{
private:
public:
	RawReceiveSocket();
	~RawReceiveSocket();
};


class DNSSender
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
				ppluint64   counter_0bytes;
				ppluint64   counter_errorcodes[255];
				double		duration;
				double		rtt_total;
				double		rtt_min;
				double		rtt_max;
		};
		ppl7::ThreadPool threadpool;
		ppl7::String Ziel;
		ppl7::String Quelle;
		ppl7::File CSVFile;
		ppl7::File QueryFile;
		ppl7::Array SourceIpList;
		ppl7::Mutex QueryMutex;
		ppluint64 validLinesInQueryFile;
		int Laufzeit;
		int Timeout;
		int ThreadCount;
		float Zeitscheibe;
		bool ignoreResponses;

		void openCSVFile(const ppl7::String &Filename);
		void openQueryFile(const ppl7::String &Filename);
		void run(int queryrate);
		void presentResults(const DNSSender::Results &result);
		void saveResultsToCsv(const DNSSender::Results &result);
		void prepareThreads();
		void getResults(DNSSender::Results &result);
		ppl7::Array getQueryRates(const ppl7::String &QueryRates);
		void readSourceIPList(const ppl7::String &filename);

	public:
		DNSSender();
		void help();
		int main(int argc, char**argv);
		void getQuery(ppl7::String &buffer);
};

class DNSSenderThread : public ppl7::Thread
{
	private:
		ppl7::ByteArray buffer;
		RawSocket Socket;

		ppl7::String destination;
		ppluint64 queryrate;
		ppluint64 counter_send, errors, counter_0bytes;
		ppluint64 counter_errorcodes[255];
		int runtime;
		int timeout;
		double Zeitscheibe;

		double duration;
		bool ignoreResponses;
		bool verbose;

		void sendPacket();
		void waitForTimeout();
		bool socketReady();

		void runWithoutRateLimit();
		void runWithRateLimit();

	public:
		DNSSenderThread();
		~DNSSenderThread();
		void setDestinationIP(const ppl7::IPAddress &destination);
		void setSourceIP(const ppl7::IPAddress &ip);
		void setRandomSource(const ppl7::IPNetwork &net);
		void setRuntime(int seconds);
		void setTimeout(int seconds);
		void setQueryRate(ppluint64 qps);
		void setZeitscheibe(float ms);
		//void setIgnoreResponses(bool flag);
		void setVerbose(bool verbose);
		void run();
		ppluint64 getPacketsSend() const;
		ppluint64 getBytesSend() const;
		/*
		ppluint64 getPacketsReceived() const;
		ppluint64 getBytesReceived() const;
		*/
		ppluint64 getErrors() const;
		ppluint64 getCounter0Bytes() const;
		ppluint64 getCounterErrorCode(int err) const;
		/*
		double getDuration() const;
		double getRoundTripTimeAverage() const;
		double getRoundTripTimeMin() const;
		double getRoundTripTimeMax() const;
		*/
};


#endif /* DSTRESS_INCLUDE_DSTRESS_H_ */
