#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "udpecho.h"

void UDPEchoCounter::clear()
{
	packets_received=0;
	packets_send=0;
	bytes_received=0;
	bytes_send=0;
}

void UDPEchoCounter::exportToArray(ppl7::AssocArray &data) const
{
	data.setf("sampleTime","%0.6f",sampleTime);
	data.setf("packets_received","%lu",packets_received);
	data.setf("packets_send","%lu",packets_send);
	data.setf("bytes_received","%lu",bytes_received);
	data.setf("bytes_send","%lu",bytes_send);

}

void UDPEchoCounter::importFromArray(const ppl7::AssocArray &data)
{
	sampleTime=data.getString("sampleTime").toDouble();
	packets_received=data.getString("packets_received").toUnsignedInt64();
	packets_send=data.getString("packets_send").toUnsignedInt64();
	bytes_received=data.getString("bytes_received").toUnsignedInt64();
	bytes_send=data.getString("bytes_send").toUnsignedInt64();
}
