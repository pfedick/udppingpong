#include <ppl7.h>
#include <ppl7-inet.h>

#include "dnsperftest_communicator.h"

Communicator::Communicator()
{
	lastuse=0;
	lastping=0;
	pingtime=0.0f;
	Msg.enableCompression(true);
	timeout_connect_sec=0;
	timeout_connect_usec=0;
	timeout_read_sec=0;
	timeout_read_usec=0;
}

Communicator::~Communicator()
{
	ppl7::TCPSocket::disconnect();
}

void Communicator::setConnectTimeout(int sec, int usec)
{
	timeout_connect_sec=sec;
	timeout_connect_usec=usec;
	setTimeoutConnect(sec, usec);
}

void Communicator::setReadTimeout(int sec, int usec)
{
	timeout_read_sec=sec;
	timeout_read_usec=usec;
	setTimeoutWrite(sec, usec);
	setTimeoutRead(sec, usec);
}

void Communicator::connect(const ppl7::String &Hostname, int Port)
{
	ppl7::TCPSocket::connect(Hostname, Port);
}

void Communicator::disconnect()
{
	ppl7::TCPSocket::disconnect();
}

bool Communicator::ping()
{
	if (!isConnected()) return false;
	double start=ppl7::GetMicrotime();
	ppl7::AssocArray msg, answer;
	msg.set("command","ping");
	msg.setf("mytime","%lu", ppl7::GetTime());
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			return false;
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		return false;
	}
	pingtime=ppl7::GetMicrotime()-start;
	lastping=ppl7::GetTime();
	return true;
}


bool Communicator::talk(const ppl7::AssocArray &msg, ppl7::AssocArray &answer, ppl7::Thread *watch_thread)
{
	printf ("Send:\n");
	msg.list();
	Msg.setPayload(msg);
	ppl7::TCPSocket::write(Msg);
	if (!ppl7::TCPSocket::waitForMessage(Msg,timeout_read_sec,watch_thread)) {
		return false;
	}
	Msg.getPayload(answer);
	printf ("Answer:\n");
	answer.list();
	return true;
}

void Communicator::proxyTo(const ppl7::String &Hostname, int Port)
{
	ppl7::AssocArray msg, answer;
	msg.set("command","proxyto");
	msg.set("host",Hostname);
	msg.setf("port","%d", Port);
	msg.setf("timeout_connect_sec","%d", timeout_connect_sec);
	msg.setf("timeout_connect_usec","%d", timeout_connect_usec);
	msg.setf("timeout_read_sec","%d", timeout_read_sec);
	msg.setf("timeout_read_usec","%d", timeout_read_usec);
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("proxyto [%s]",(const char*)answer.getString("error"));
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}
}
