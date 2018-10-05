#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "dstress.h"


DNSReceiverThread::DNSReceiverThread()
{
	counter_packets_received=0;
	counter_bytes_received=0;
	min_rtt=max_rtt=total_rtt=0.0f;
}

DNSReceiverThread::~DNSReceiverThread()
{

}

void DNSReceiverThread::setInterface(const ppl7::String &Device)
{
	Socket.initInterface(Device);
}

void DNSReceiverThread::setSource(const ppl7::IPAddress &ip, int port)
{
	Socket.setSource(ip, port);
}

void DNSReceiverThread::run()
{
	size_t size;
	double rtt;
	counter_packets_received=0;
	counter_bytes_received=0;
	min_rtt=max_rtt=total_rtt=0.0f;
#ifdef __FreeBSD__
	int ccc=0;
	while (1) {
		ccc++;
		if (ccc>100000) {
			ccc=0;
			if (this->threadShouldStop()) return;
		}
		if (Socket.receive(size,rtt)) {
			counter_packets_received++;
			counter_bytes_received+=size;
			total_rtt+=rtt;
			if (rtt>max_rtt) max_rtt=rtt;
			if (rtt<min_rtt || min_rtt==0.0f) min_rtt=rtt;
		}
	}
#else
	double start=ppl7::GetMicrotime();
	double now,next_checktime=start+0.1;
	while (1) {
		if (Socket.socketReady()) {
			if (Socket.receive(size,rtt)) {
				counter_packets_received++;
				counter_bytes_received+=size;
				total_rtt+=rtt;
				if (rtt>max_rtt) max_rtt=rtt;
				if (rtt<min_rtt || min_rtt==0.0f) min_rtt=rtt;
			}
		}
		now=ppl7::GetMicrotime();
		if (now>next_checktime) {
			next_checktime=now+0.1;
			if (this->threadShouldStop()) break;
		}
	}
#endif
}

ppluint64 DNSReceiverThread::getPacketsReceived() const
{
	return counter_packets_received;
}

ppluint64 DNSReceiverThread::getBytesReceived() const
{
	return counter_bytes_received;
}

double DNSReceiverThread::getDuration() const
{
	return total_rtt;
}

double DNSReceiverThread::getRoundTripTimeAverage() const
{
	if (counter_packets_received) return total_rtt/counter_packets_received;
	return 0.0f;
}

double DNSReceiverThread::getRoundTripTimeMin() const
{
	return min_rtt;
}

double DNSReceiverThread::getRoundTripTimeMax() const
{
	return max_rtt;
}



