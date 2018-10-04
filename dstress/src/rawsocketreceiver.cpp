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

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
struct ETHER
{
	unsigned char destination[6];
	unsigned char source[6];
	unsigned short type;
};
#pragma pack(pop)   /* restore original alignment from stack */


RawSocketReceiver::RawSocketReceiver()
{
	SourcePort=0;
	buffer=(unsigned char*)malloc(65536);
	if (!buffer) throw ppl7::OutOfMemoryException();
	if ((sd = socket(AF_PACKET, SOCK_RAW, htons(0x0800))) == -1) {
		free(buffer);
		ppl7::throwExceptionFromErrno(errno,"Could not create RawReceiverSocket");
	}
}

RawSocketReceiver::~RawSocketReceiver()
{
	close(sd);
	free(buffer);
}

void RawSocketReceiver::setSource(const ppl7::IPAddress &ip_addr, int port)
{
	SourceIP=ip_addr;
	SourcePort=htons(port);
}


bool RawSocketReceiver::socketReady()
{
	fd_set rset;
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=100;
	FD_ZERO(&rset);
	FD_SET(sd,&rset); // Wir wollen nur prüfen, ob wir lesen können
	int ret=select(sd+1,&rset,NULL,NULL,&timeout);
	if (ret<0) return false;
	if (FD_ISSET(sd,&rset)) {
		return true;
	}
	return false;
}

bool RawSocketReceiver::receive(size_t &size, double &rtt)
{
	ssize_t bufused=recvfrom(sd,buffer,65536,0,NULL,NULL);
	if (bufused<34) return false;
	struct ETHER *eth=(struct ETHER*)buffer;
	//ppl7::HexDump(buffer,bufused);
	//printf ("sizeof ETHER=%d, type=%X\n",sizeof(struct ETHER),eth->type);
	if (eth->type!=htons(0x0800)) return false;
	struct ip *iphdr = (struct ip *)(buffer+14);
	if (iphdr->ip_v!=4) return false;
	if (iphdr->ip_src.s_addr!=*(in_addr_t*)SourceIP.addr()) return false;

	struct udphdr *udp = (struct udphdr *)(buffer+14+sizeof(struct ip));
	if (udp->source!=SourcePort) return false;

	struct DNS_HEADER *dns=(struct DNS_HEADER *)(buffer+14+sizeof(struct ip)+sizeof(struct udphdr));

	size=bufused-sizeof(struct ETHER);
	unsigned short r=getQueryRTT(ntohs(dns->id));
	if (r>60000) return false;
	rtt=(double)r/1000.0f;
	return true;
}

