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

PPL7EXCEPTION(InvalidDNSQuery, Exception);
PPL7EXCEPTION(UnknownRRType, Exception);
PPL7EXCEPTION(BufferOverflow, Exception);
PPL7EXCEPTION(UnknownDestination, Exception);

int MakeQuery(const ppl7::String &query, unsigned char *buffer, size_t buffersize);


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
	void setDestination(const ppl7::String &ip_addr, int port);
	ssize_t send(Packet &pkt);
};

#endif /* DSTRESS_INCLUDE_DSTRESS_H_ */
