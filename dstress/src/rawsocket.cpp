#include <ppl7.h>
#include <ppl7-inet.h>
#include <string.h>
#include <stdlib.h>

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

RawSocket::RawSocket()
{
	buffer=calloc(1,sizeof(struct sockaddr_in));
	if (!buffer) throw ppl7::OutOfMemoryException();
	struct sockaddr_in *dest=(struct sockaddr_in *)buffer;
	dest->sin_addr.s_addr=-1;
	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		ppl7::throwExceptionFromErrno(errno,"Could not create RawSocket");
	}
	unsigned int set =1;
	if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &set, sizeof(set)) < 0) {
		ppl7::throwExceptionFromErrno(errno,"Could not set socket option IP_HDRINCL");
	}
}

RawSocket::~RawSocket()
{
	close(sd);
}

void RawSocket::setDestination(const ppl7::String &ip_addr, int port)
{
	struct sockaddr_in *dest=(struct sockaddr_in *)buffer;
	dest->sin_family=AF_INET;
	dest->sin_port=htons(port);
	dest->sin_addr.s_addr=inet_addr(ip_addr);
}

ssize_t RawSocket::send(Packet &pkt)
{
	struct sockaddr_in *dest=(struct sockaddr_in *)buffer;
	if (dest->sin_addr.s_addr==(unsigned int)-1) throw UnknownDestination();
	return sendto(sd, pkt.ptr(),pkt.size(),0,
			(const struct sockaddr *)dest, sizeof(struct sockaddr_in));

}
