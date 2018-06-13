#include <ppl7.h>
#include <ppl7-inet.h>

#include "dnsperftest_communicator.h"
#include "dnsperftest_sensor.h"

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
	//printf ("Send:\n");
	//msg.list();
	Msg.setPayload(msg);
	ppl7::TCPSocket::write(Msg);
	if (!ppl7::TCPSocket::waitForMessage(Msg,timeout_read_sec,watch_thread)) {
		return false;
	}
	Msg.getPayload(answer);
	if (answer.getString("result") != "ok") {
		throw CommandFailedException("cmd=%s, error=%s",
				(const char*) msg.getString("command"),
				(const char*) answer.getString("error"));
	}
	//printf ("Answer:\n");
	//answer.list();
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


void Communicator::startSensor()
{
	ppl7::AssocArray msg, answer;
	msg.set("command","startsensor");
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("startSensor");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}
}

void Communicator::stopSensor()
{
	ppl7::AssocArray msg, answer;
	msg.set("command","stopsensor");
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("stopSensor");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}
}

void Communicator::getSensorData(std::list<SystemStat> &data)
{
	ppl7::AssocArray msg, answer;
	data.clear();
	msg.set("command","getsensordata");
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("getsensordata");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}
	const ppl7::AssocArray row;
	const ppl7::AssocArray &d=answer.getArray("data");
	ppl7::AssocArray::Iterator it;
	d.reset(it);
	while (d.getNext(it, ppl7::Variant::TYPE_ASSOCARRAY)) {
		SystemStat s;
		s.importFromArray(it.value().toAssocArray());
		data.push_back(s);
	}
}

SystemStat Communicator::getSystemStat()
{
	ppl7::AssocArray msg, answer;
	msg.set("command","getsystemstat");
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("getsensordata");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}
	const ppl7::AssocArray &d=answer.getArray("data");
	SystemStat s;
	s.importFromArray(d);
	return s;
}

void Communicator::startUDPEchoServer(const ppl7::String &hostname, int port, size_t num_threads, size_t PacketSize, bool disable_responses)
{
	ppl7::AssocArray msg, answer;
	msg.set("command","startudpechoserver");
	msg.set("hostname", hostname);
	msg.setf("port", "%d", port);
	msg.setf("packetsize", "%zd", PacketSize);
	msg.setf("threads", "%zd", num_threads);
	msg.setf("disable_responses", "%d", disable_responses);

	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("startudpechoserver");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}

}

void Communicator::stopUDPEchoServer()
{
	ppl7::AssocArray msg, answer;
	msg.set("command","stopudpechoserver");
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("stopudpechoserver");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}

}

void Communicator::getUDPEchoServerData(std::list<UDPEchoCounter> &data)
{
	ppl7::AssocArray msg, answer;
	data.clear();
	msg.set("command","getudpechoserverdata");
	try {
		if (!talk(msg, answer)) {
			ppl7::TCPSocket::disconnect();
			throw ppl7::OperationFailedException("getudpechoserverdata");
		}
	} catch (...) {
		ppl7::TCPSocket::disconnect();
		throw;
	}
	const ppl7::AssocArray row;
	const ppl7::AssocArray &d=answer.getArray("data");
	ppl7::AssocArray::Iterator it;
	d.reset(it);
	while (d.getNext(it, ppl7::Variant::TYPE_ASSOCARRAY)) {
		UDPEchoCounter s;
		s.importFromArray(it.value().toAssocArray());
		data.push_back(s);
	}
}

