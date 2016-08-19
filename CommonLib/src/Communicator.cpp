#include <ppl7.h>
#include <ppl7-inet.h>

#include "dnsperftest_communicator.h"

Communicator::Communicator()
{
	lastuse=0;
	lastping=0;
	pingtime=0.0f;
	timeout=0;
	Msg.enableCompression(true);
}

Communicator::~Communicator()
{
	ppl7::TCPSocket::disconnect();
}

void Communicator::setConnectTimeout(int sec, int usec)
{
	setTimeoutConnect(sec, usec);
}

void Communicator::setReadTimeout(int sec, int usec)
{
	setTimeoutWrite(sec, usec);
	setTimeoutRead(sec, usec);
	timeout=sec;
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
	if (!ppl7::TCPSocket::waitForMessage(Msg,timeout,watch_thread)) {
		return false;
	}
	Msg.getPayload(answer);
	printf ("Answer:\n");
	answer.list();
	return true;
}
