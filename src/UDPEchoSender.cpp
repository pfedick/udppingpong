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

#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "udpecho.h"

UDPEchoSender::UDPEchoSender()
{
	Packetsize=64;
	Timeout=5;
	ThreadCount=1;
	ignoreResponses=false;
	verbose=false;
	Timeslice=1.0;
	Runtime=10;
}


UDPEchoSender::~UDPEchoSender()
{
	stop();
	destroyThreads();
}

void UDPEchoSender::setDestination(const ppl7::String &destination)
{
	Destination=destination;
}

void UDPEchoSender::setRuntime(int sec)
{
	Runtime=sec;
}

void UDPEchoSender::setTimeout(int sec)
{
	Timeout=sec;
}

void UDPEchoSender::setThreads(size_t num)
{
	if (num<1) num=1;
	ThreadCount=num;
}

void UDPEchoSender::setPacketsize(int bytes)
{
	if (bytes<(int)sizeof(PACKET)) bytes=(int)sizeof(PACKET);
	Packetsize=bytes;
}

void UDPEchoSender::setIgnoreResponses(bool ignore)
{
	ignoreResponses=ignore;
}

void UDPEchoSender::setTimeslice(float ms)
{
	Timeslice=ms;
}

void UDPEchoSender::setVerbose(bool verbose)
{
	this->verbose=verbose;
}


void UDPEchoSender::addSourceIP(const ppl7::String &ip)
{
	SourceIpList.add(ip);
}

void UDPEchoSender::addSourceIP(const std::list<ppl7::String> &ip_list)
{
	std::list<ppl7::String>::const_iterator it;
	for (it=ip_list.begin();it!=ip_list.end();++it) {
		SourceIpList.add((*it));
	}
}

void UDPEchoSender::addSourceIP(const ppl7::Array &ip_list)
{
	SourceIpList.add(ip_list);
}

void UDPEchoSender::clearSourceIPs()
{
	SourceIpList.clear();
}

void UDPEchoSender::prepareThreads()
{
	if (ThreadCount!=threadpool.size()) {
		destroyThreads();
		for (size_t i=0;i<ThreadCount;i++) {
			UDPEchoSenderThread *thread=new UDPEchoSenderThread();
			threadpool.addThread(thread);
		}
	}
	size_t si=0;
	ppl7::ThreadPool::iterator it;
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		UDPEchoSenderThread *thread=(UDPEchoSenderThread*)(*it);
		thread->setDestination(Destination);
		thread->setPacketsize(Packetsize);
		thread->setRuntime(Runtime);
		thread->setTimeout(Timeout);
		thread->setZeitscheibe(Timeslice);
		thread->setIgnoreResponses(ignoreResponses);
		thread->setVerbose(verbose);
		if (SourceIpList.size()>0) {
			thread->setSourceIP(SourceIpList[si]);
			si++;
			if (si>=SourceIpList.size()) si=0;
		}
	}
}

void UDPEchoSender::destroyThreads()
{
	threadpool.destroyAllThreads();
}

void UDPEchoSender::start(int queryrate)
{
	stop();
	if (ThreadCount!=threadpool.size())destroyThreads();
	prepareThreads();
	ppl7::ThreadPool::iterator it;
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		((UDPEchoSenderThread*)(*it))->setQueryRate(queryrate/ThreadCount);
	}
	threadpool.startThreads();
}

void UDPEchoSender::stop()
{
	threadpool.stopThreads();
}


bool UDPEchoSender::isRunning()
{
	return threadpool.running();
}


UDPSenderResults UDPEchoSender::getResults()
{
	UDPSenderResults result;
	ppl7::ThreadPool::iterator it;
	result.counter_send=0;
	result.counter_received=0;
	result.bytes_received=0;
	result.counter_errors=0;
	result.counter_0bytes=0;
	result.duration=0.0;
	result.rtt_total=0.0f;
	result.rtt_min=0.0f;
	result.rtt_max=0.0f;
	for (int i=0;i<255;i++) result.counter_errorcodes[i]=0;

	for (it=threadpool.begin();it!=threadpool.end();++it) {
		result.counter_send+=((UDPEchoSenderThread*)(*it))->getPacketsSend();
		result.counter_received+=((UDPEchoSenderThread*)(*it))->getPacketsReceived();
		result.bytes_received+=((UDPEchoSenderThread*)(*it))->getBytesReceived();
		result.counter_errors+=((UDPEchoSenderThread*)(*it))->getErrors();
		result.counter_0bytes+=((UDPEchoSenderThread*)(*it))->getCounter0Bytes();
		result.duration+=((UDPEchoSenderThread*)(*it))->getDuration();
		result.rtt_total+=((UDPEchoSenderThread*)(*it))->getRoundTripTimeAverage();
		double rtt=((UDPEchoSenderThread*)(*it))->getRoundTripTimeMin();
		if (result.rtt_min==0) result.rtt_min=rtt;
		else if (rtt<result.rtt_min) result.rtt_min=rtt;
		rtt=((UDPEchoSenderThread*)(*it))->getRoundTripTimeMax();
		if (rtt>result.rtt_max) result.rtt_max=rtt;
		for (int i=0;i<255;i++) result.counter_errorcodes[i]+=((UDPEchoSenderThread*)(*it))->getCounterErrorCode(i);
	}
	result.packages_lost=result.counter_send-result.counter_received;
	result.duration=result.duration/(double)ThreadCount;
	return result;
}



