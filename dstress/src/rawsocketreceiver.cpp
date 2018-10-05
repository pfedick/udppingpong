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

#ifdef __FreeBSD__
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <net/bpf.h>
#include <net/ethernet.h>
#endif

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


#ifdef __FreeBSD__
static int open_bpf()
{
	int sd;
	for (int i=0;i<255;i++) {
		ppl7::String Device;
		Device.setf("/dev/bpf%d", i);
		sd=open((const char*)Device, O_RDWR);
		if (sd>=0) {
			printf("opened %s\n",(const char*)Device);
			return sd;
		}
	}
	ppl7::throwExceptionFromErrno(errno,"Could not create RawReceiverSocket");
	return -1;
}

#endif

RawSocketReceiver::RawSocketReceiver()
{
	SourceIP.set("0.0.0.0");
	SourcePort=0;
	buflen=4096;
	buffer=(unsigned char*)malloc(buflen);
	if (!buffer) throw ppl7::OutOfMemoryException();
	sd=-1;
#ifdef __FreeBSD__
	sd=open_bpf();
	struct ifreq ifreq;
	strcpy((char *) ifreq.ifr_name, "igb3");
	if (ioctl(sd, BIOCSETIF, &ifreq) < 0) {
		close(sd);
		ppl7::throwExceptionFromErrno(errno,"Could not bind RawReceiverSocket on interface (BIOCSETIF)");
	}
	ioctl(sd, BIOCGBLEN, &buflen);
	printf ("buflen=%d\n",buflen);
	unsigned int tmp = 1;
	if (ioctl(sd, BIOCIMMEDIATE, &tmp) < 0) {
		close(sd);
		ppl7::throwExceptionFromErrno(errno,"BIOCIMMEDIATE failed");
	}
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (ioctl(sd, BIOCSRTIMEOUT, (struct timeval *) &timeout) < 0) {
		close(sd);
		ppl7::throwExceptionFromErrno(errno,"BIOCSRTIMEOUT failed");
	}

	buffer=(unsigned char*)malloc(65536);
	if (!buffer) {
		close(sd);
		throw ppl7::OutOfMemoryException();
	}
#else
	if ((sd = socket(AF_PACKET, SOCK_RAW, htons(0x0800))) == -1) {
		free(buffer);
		ppl7::throwExceptionFromErrno(errno,"Could not create RawReceiverSocket");
	}
#endif
}

RawSocketReceiver::~RawSocketReceiver()
{
	close(sd);
	free(buffer);
}

void RawSocketReceiver::initInterface(const ppl7::String &Device)
{
	if (sd>=0) close(sd);
	sd=-1;
#ifdef __FreeBSD__
	sd=open_bpf();
	struct ifreq ifreq;
	strcpy((char *) ifreq.ifr_name, (const char*)Device);
	if (ioctl(sd, BIOCSETIF, &ifreq) < 0) {
		close(sd);
		ppl7::throwExceptionFromErrno(errno,"Could not bind RawReceiverSocket on interface (BIOCSETIF)");
	}
	ioctl(sd, BIOCGBLEN, &buflen);
	printf ("buflen=%d\n",buflen);
	unsigned int tmp = 1;
	if (ioctl(sd, BIOCIMMEDIATE, &tmp) < 0) {
		close(sd);
		ppl7::throwExceptionFromErrno(errno,"BIOCIMMEDIATE failed");
	}
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if (ioctl(sd, BIOCSRTIMEOUT, (struct timeval *) &timeout) < 0) {
		close(sd);
		ppl7::throwExceptionFromErrno(errno,"BIOCSRTIMEOUT failed");
	}

	buffer=(unsigned char*)malloc(65536);
	if (!buffer) {
		close(sd);
		throw ppl7::OutOfMemoryException();
	}
#else
	if ((sd = socket(AF_PACKET, SOCK_RAW, htons(0x0800))) == -1) {
		free(buffer);
		ppl7::throwExceptionFromErrno(errno,"Could not create RawReceiverSocket");
	}
#endif
}

void RawSocketReceiver::setSource(const ppl7::IPAddress &ip_addr, int port)
{
	SourceIP=ip_addr;
	SourcePort=htons(port);
}


bool RawSocketReceiver::socketReady()
{
#ifdef __FreeBSD__
	return true;
#else
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
#endif
}

bool RawSocketReceiver::receive(size_t &size, double &rtt)
{
#ifdef __FreeBSD__
	ssize_t bufused=read(sd,buffer,buflen);
	if (bufused < sizeof(struct bpf_hdr)) return false;
	struct bpf_hdr *bpf=(struct bpf_hdr *)buffer;
	unsigned char *ptr=buffer+bpf->bh_hdrlen; 
	bufused-=bpf->bh_hdrlen;
#else
	unsigned char *ptr=buffer;
	ssize_t bufused=recvfrom(sd,buffer,buflen,0,NULL,NULL);
#endif
	if (bufused<34) return false;
	struct ETHER *eth=(struct ETHER*)ptr;
	//ppl7::HexDump(ptr,bufused);
	//printf ("sizeof ETHER=%d, type=%X\n",sizeof(struct ETHER),eth->type);
	if (eth->type!=htons(0x0800)) return false;
	struct ip *iphdr = (struct ip *)(ptr+14);
	if (iphdr->ip_v!=4) return false;
	if (iphdr->ip_src.s_addr!=*(in_addr_t*)SourceIP.addr()) return false;

	struct udphdr *udp = (struct udphdr *)(ptr+14+sizeof(struct ip));
	if (udp->uh_sport!=SourcePort) return false;

	struct DNS_HEADER *dns=(struct DNS_HEADER *)(ptr+14+sizeof(struct ip)+sizeof(struct udphdr));

	size=bufused-sizeof(struct ETHER);
	unsigned short r=getQueryRTT(ntohs(dns->id));
	if (r>60000) return false;
	rtt=(double)r/1000.0f;
	return true;
}

