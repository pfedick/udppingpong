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

#define USZ sizeof(struct udphdr)
#define ISZ sizeof(struct ip)
#define HDRSZ ISZ+USZ
#define MAXPACKETSIZE 4096
//#define PCKSZ HDRSZ+MAXPACKETSIZE+1

static unsigned short in_cksum(unsigned short *addr, int len)
{
	int nleft  = len;
	unsigned short *w = addr;
	int sum    = 0;
	unsigned short answer = 0;
	while (nleft > 1)  {
		sum   += *w++;
		nleft -=  2;
	}
	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}
	sum  = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

static unsigned short udp_cksum(const struct ip *iphdr, const struct udphdr *udp, const unsigned char *payload, size_t payload_size)
{
	unsigned char cbuf[MAXPACKETSIZE];
	memset(cbuf, 0, sizeof(cbuf));
	unsigned char *p = (unsigned char *)cbuf;
	*(unsigned int *)p = iphdr->ip_src.s_addr;
	p += sizeof(unsigned int);
	*(unsigned int *)p = iphdr->ip_dst.s_addr;
	p += sizeof(unsigned int);
	*(unsigned char *)p++ = 0;
	*(unsigned char *)p++ = iphdr->ip_p;
	*(unsigned short*)p   = udp->len;
	p += sizeof(unsigned short);
	memcpy(p, udp, USZ);
	p += USZ;
	memcpy(p, payload, payload_size);
	return in_cksum((unsigned short*)cbuf,sizeof(uint)*3+sizeof(struct udphdr)+payload_size+(payload_size%2));
}



Packet::Packet()
{
	buffersize=MAXPACKETSIZE;
	payload_size=0;
	buffer=(unsigned char*)calloc(1,buffersize);
	if (!buffer) throw ppl7::OutOfMemoryException();

	struct ip *iphdr = (struct ip *)buffer;
	struct udphdr *udp = (struct udphdr *)(buffer+ISZ);

	iphdr->ip_hl  = ISZ >> 2;
	iphdr->ip_v   = IPVERSION;
	iphdr->ip_tos = 0;
	iphdr->ip_off = 0;
	iphdr->ip_ttl = 64;
	iphdr->ip_p   = IPPROTO_UDP;
	iphdr->ip_sum = 0;
	iphdr->ip_id  = 0;
	iphdr->ip_src.s_addr = 0;
	iphdr->ip_dst.s_addr = 0;
	iphdr->ip_len=htons(HDRSZ+payload_size);
	iphdr->ip_sum = in_cksum((unsigned short*)iphdr,ISZ);

	udp->len=htons(USZ+payload_size);
	chksum_valid=false;
}

Packet::~Packet()
{
	free(buffer);
}

void Packet::setSource(const ppl7::String &ip_addr, int port)
{
	struct ip *iphdr = (struct ip *)buffer;
	struct udphdr *udp = (struct udphdr *)(buffer+ISZ);
	iphdr->ip_src.s_addr = inet_addr(ip_addr);
	udp->source=htons(port);
	chksum_valid=false;
}

void Packet::setDestination(const ppl7::String &ip_addr, int port)
{
	struct ip *iphdr = (struct ip *)buffer;
	struct udphdr *udp = (struct udphdr *)(buffer+ISZ);
	iphdr->ip_dst.s_addr = inet_addr(ip_addr);
	udp->dest=htons(port);
	chksum_valid=false;
}

void Packet::setId(unsigned short id)
{
	struct ip *iphdr = (struct ip *)buffer;
	iphdr->ip_id  = htons(id);
	chksum_valid=false;
}

void Packet::setPayload(const void *payload, size_t size)
{
	if (size+HDRSZ>MAXPACKETSIZE) throw BufferOverflow("%zd > %zd",size,MAXPACKETSIZE-HDRSZ);
	memcpy(buffer+HDRSZ,payload,size);
	payload_size=size;
	struct ip *iphdr = (struct ip *)buffer;
	struct udphdr *udp = (struct udphdr *)(buffer+ISZ);
	iphdr->ip_len=htons(HDRSZ+payload_size);
	udp->len=htons(USZ+payload_size);
	chksum_valid=false;
}

void Packet::setPayloadDNSQuery(const ppl7::String &query)
{
	payload_size=MakeQuery(query,buffer+HDRSZ,buffersize-HDRSZ);
	struct ip *iphdr = (struct ip *)buffer;
	struct udphdr *udp = (struct udphdr *)(buffer+ISZ);
	iphdr->ip_len=htons(HDRSZ+payload_size);
	udp->len=htons(USZ+payload_size);
	chksum_valid=false;
}

void Packet::updateChecksums()
{
	struct ip *iphdr = (struct ip *)buffer;
	struct udphdr *udp = (struct udphdr *)(buffer+ISZ);
	iphdr->ip_sum = in_cksum((unsigned short*)iphdr,ISZ);
	udp->check=udp_cksum(iphdr,udp,buffer+HDRSZ,payload_size);
	chksum_valid=true;
}

size_t Packet::size() const
{
	return HDRSZ+payload_size;
}

unsigned char* Packet::ptr()
{
	if (!chksum_valid) updateChecksums();
	return buffer;
}



