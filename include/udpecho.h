/*
 * This file is part of udppingpong by Patrick Fedick <fedick@denic.de>
 *
 * Copyright (c) 2019 DENIC eG
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UDPECHO_H_
#define UDPECHO_H_

#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

typedef struct {
		int64_t id;
		double time;
} PACKET;

class UDPEchoCounter {
	public:
		int64_t packets_received;
		int64_t packets_send;
		int64_t bytes_received;
		int64_t bytes_send;
		double sampleTime;
		void clear();
		void exportToArray(ppl7::AssocArray &data) const;
		void importFromArray(const ppl7::AssocArray &data);
};


class UDPSenderResults
{
	public:
		int			queryrate;
		int64_t	counter_send;
		int64_t	counter_received;
		int64_t	bytes_send;
		int64_t	bytes_received;
		int64_t	counter_errors;
		int64_t	packages_lost;
		int64_t   counter_0bytes;
		int64_t   counter_errorcodes[255];
		double		duration;
		double		rtt_total;
		double		rtt_min;
		double		rtt_max;
};

class UDPEchoReceiverThread : public ppl7::Thread
{
	private:
		int sockfd;
		ppl7::ByteArray recbuffer;
		int64_t counter_received;
		int64_t bytes_received;

		double rtt_total, rtt_min, rtt_max;

		void countPacket(const PACKET *p, ssize_t bytes);

	public:
		UDPEchoReceiverThread();
		~UDPEchoReceiverThread();
		void setSocketDescriptor(int sockfd);
		void run();
		void resetCounter();
		int64_t getPacketsReceived() const;
		int64_t getBytesReceived() const;
		double getRoundTripTimeAverage() const;
		double getRoundTripTimeMin() const;
		double getRoundTripTimeMax() const;

};

class UDPEchoSender
{
private:
	ppl7::Array 		SourceIpList;
	ppl7::ThreadPool	threadpool;
	ppl7::String		Destination;
	int					Packetsize;
	int					Timeout;
	size_t				ThreadCount;
	int					Runtime;
	float				Timeslice;
	bool				ignoreResponses;
	bool				verbose;

	void prepareThreads();
	void destroyThreads();

public:
	UDPEchoSender();
	~UDPEchoSender();
	void setDestination(const ppl7::String &destination);
	void setRuntime(int sec);
	void setTimeout(int sec);
	void setThreads(size_t num);
	void setPacketsize(int bytes);
	void setIgnoreResponses(bool ignore);
	void setTimeslice(float ms);
	void setVerbose(bool verbose);
	void addSourceIP(const ppl7::String &ip);
	void addSourceIP(const ppl7::Array &ip_list);
	void addSourceIP(const std::list<ppl7::String> &ip_list);
	void clearSourceIPs();
	void start(int queryrate);
	void stop();
	bool isRunning();
	UDPSenderResults getResults();
};


class UDPEchoSenderThread : public ppl7::Thread
{
	private:
		ppl7::ByteArray buffer;
		ppl7::UDPSocket Socket;
		UDPEchoReceiverThread receiver;

		ppl7::String destination;
		size_t packetsize;
		int64_t queryrate;
		int64_t counter_send, errors, counter_0bytes;
		int64_t counter_errorcodes[255];
		int runtime;
		int timeout;
		double Zeitscheibe;

		double duration;
		int sockfd;
		bool ignoreResponses;
		bool verbose;
		bool alwaysRandomize;

		void sendPacket();
		void waitForTimeout();
		bool socketReady();

		void runWithoutRateLimit();
		void runWithRateLimit();

	public:
		UDPEchoSenderThread();
		~UDPEchoSenderThread();
		void setDestination(const ppl7::String &destination);
		void setPacketsize(size_t size);
		void setRuntime(int seconds);
		void setTimeout(int seconds);
		void setQueryRate(int64_t qps);
		void setZeitscheibe(float ms);
		void setIgnoreResponses(bool flag);
		void setSourceIP(const ppl7::String &ip);
		void setVerbose(bool verbose);
		void setAlwaysRandomize(bool flag);
		void run();
		int64_t getPacketsSend() const;
		int64_t getPacketsReceived() const;
		int64_t getBytesReceived() const;
		int64_t getErrors() const;
		int64_t getCounter0Bytes() const;
		int64_t getCounterErrorCode(int err) const;
		double getDuration() const;
		double getRoundTripTimeAverage() const;
		double getRoundTripTimeMin() const;
		double getRoundTripTimeMax() const;
};



class UDPEchoBouncer
{
	private:
		ppl7::ThreadPool threadpool;
		struct sockaddr_in servaddr;
		ppl7::SockAddr sockaddr;
		int sockfd;
		size_t packetSize;
		bool noEcho;
		bool running;
		ppl7::SockAddr getSockAddr(const ppl7::String &Hostname, int Port);
		void startBouncerThreads(size_t ThreadCount);
		void bind(const ppl7::SockAddr &sockaddr);
		void createSocket();

	public:
		UDPEchoBouncer();
		~UDPEchoBouncer();
		void setFixedResponsePacketSize(size_t size);
		void setInterface(const ppl7::String &InterfaceName, int Port);
		void disableResponses(bool flag);
		void start(size_t num_threads);
		void stop();
		bool isRunning();
		UDPEchoCounter getCounter();
};



class UDPEchoBouncerThread : public ppl7::Thread
{
	public:
		class Counter {
			public:
				int64_t count;
				int64_t bytes;
		};
	private:
		int sockfd;
		ppl7::SockAddr out_addr;
		ppl7::ByteArray buffer;
		void *pBuffer;
		ppl7::Mutex mutex;
		bool noEcho;
		UDPEchoCounter counter;
		size_t packetSize;

		bool waitForSocketReadable();

	public:
		UDPEchoBouncerThread();
		~UDPEchoBouncerThread();
		void setNoEcho(bool flag);
		void setPacketSize(size_t bytes);
		void setSocketDescriptor(int sockfd);
		void setSocketAddr(const ppl7::SockAddr &adr);
		void run();
		UDPEchoCounter getAndClearCounter();

};


#endif /* UDPECHO_H_ */
