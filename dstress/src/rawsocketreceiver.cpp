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
#include <machine/atomic.h>
#define ZBUF_SIZE 8192
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
	sd=-1;
	buffer=NULL;
#ifdef __FreeBSD__
	sd=open_bpf();
	buffer=(unsigned char*)malloc(sizeof(struct bpf_zbuf));
	if (!buffer) { close(sd); throw ppl7::OutOfMemoryException();}
	struct bpf_zbuf *zbuf=(struct bpf_zbuf*)buffer;
	
	unsigned int bufmode=BPF_BUFMODE_ZBUF;
	if (ioctl(sd, BIOCSETBUFMODE, &bufmode) < 0) {
		int e=errno;
		close(sd);
		free(buffer);
		ppl7::throwExceptionFromErrno(e,"BIOCSETBUFMODE with BPF_BUFMODE_ZBUF failed");
	}

	/*
	size_t zbufsize=0;
	if (ioctl(sd, BIOCGETZMAX, &zbufsize) < 0) {
		int e=errno;
		close(sd);
		free(buffer);
		ppl7::throwExceptionFromErrno(e,"BIOCGETZMAX");
	}*/
	unsigned int tstype=BPF_T_MICROTIME;
	if (ioctl(sd, BIOCSTSTAMP, &tstype) < 0) {
		int e=errno;
		close(sd);
		free(buffer);
		ppl7::throwExceptionFromErrno(e,"BIOCSTSTAMP");
	}
	zbuf->bz_buflen=ZBUF_SIZE;
	zbuf->bz_bufa=malloc(ZBUF_SIZE);
	if (!zbuf->bz_bufa) {
                close(sd);
		free(buffer);
		throw ppl7::OutOfMemoryException();
	}
	zbuf->bz_bufb=malloc(ZBUF_SIZE);
	if (!zbuf->bz_bufb) {
                close(sd);
		free(zbuf->bz_bufa);
		free(buffer);
		throw ppl7::OutOfMemoryException();
	}
	memset(zbuf->bz_bufa,0,ZBUF_SIZE);
	memset(zbuf->bz_bufb,0,ZBUF_SIZE);

	if (ioctl(sd, BIOCSETZBUF, zbuf) < 0) {
		int e=errno;
                close(sd);
		free(zbuf->bz_bufa);
		free(zbuf->bz_bufb);
		free(buffer);
                ppl7::throwExceptionFromErrno(e,"BIOCGETZMAX");
        }

	
#else
	buffer=(unsigned char*)malloc(buflen);
	if (!buffer) throw ppl7::OutOfMemoryException();
	if ((sd = socket(AF_PACKET, SOCK_RAW, htons(0x0800))) == -1) {
		int e=errno;
		free(buffer);
		ppl7::throwExceptionFromErrno(e,"Could not create RawReceiverSocket");
	}
#endif
}

RawSocketReceiver::~RawSocketReceiver()
{
	close(sd);
#ifdef __FreeBSD__
	struct bpf_zbuf *zbuf=(struct bpf_zbuf*)buffer;
	free(zbuf->bz_bufa);
	free(zbuf->bz_bufb);
#endif
	free(buffer);
}

void RawSocketReceiver::initInterface(const ppl7::String &Device)
{
#ifdef __FreeBSD__
	struct ifreq ifreq;
	strcpy((char *) ifreq.ifr_name, (const char*)Device);
	if (ioctl(sd, BIOCSETIF, &ifreq) < 0) {
		ppl7::throwExceptionFromErrno(errno,"Could not bind RawReceiverSocket on interface (BIOCSETIF)");
	}
#endif
}

void RawSocketReceiver::setSource(const ppl7::IPAddress &ip_addr, int port)
{
	SourceIP=ip_addr;
	SourcePort=htons(port);
#ifdef __FreeBSD__
	// Install packet filter in bpf
	int sip=htonl(*(int*)SourceIP.addr());
	struct bpf_insn insns[] = {
		// load halfword at position 12 from packet into register
		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),
		// is it 0x800? if no, jump over 5 instructions, else jump over 0
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x0800, 0, 7),
		// source ip
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 26),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, (unsigned int)sip, 0, 5),

		// udp?
		BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 23),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 17, 0, 3),

		// source port
		BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 34),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, (unsigned int)port, 0, 1),

		/* if we reach here, return -1 which will allow the packet to be read */
		BPF_STMT(BPF_RET+BPF_K, (u_int)-1),
		/* if we reach here, return 0 which will ignore the packet */
		BPF_STMT(BPF_RET+BPF_K, 0),
	};
	struct bpf_program bpf_program = {
		10,
		(struct bpf_insn *) &insns
	};
	if (ioctl(sd, BIOCSETF, (struct bpf_program *) &bpf_program) < 0) {
		throw FailedToInitializePacketfilter();
	}
#endif
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

#ifdef __FreeBSD__
/*
 *	Return ownership of a buffer to	the kernel for reuse.
 */
static void buffer_acknowledge(struct bpf_zbuf_header *bzh)
{
	atomic_store_rel_int(&bzh->bzh_user_gen, bzh->bzh_kernel_gen);
}

static int buffer_check(struct bpf_zbuf_header *bzh)
{
	return (bzh->bzh_user_gen !=
			atomic_load_acq_int(&bzh->bzh_kernel_gen));
}

static void read_zbuffer(struct bpf_zbuf_header *zhdr, ppluint64 &num_pkgs, ppluint64 &bytes_rcv, double &rtt, double &min, double &max)
{
	size_t size=zhdr->bzh_kernel_len-sizeof(struct bpf_zbuf_header);
	//size_t size=zhdr->bzh_kernel_len;
	unsigned char *ptr=(unsigned char *)zhdr+sizeof(struct bpf_zbuf_header);
	size_t done=0;
	while (done<size) {
		struct bpf_hdr* bpfh=(struct bpf_hdr*)ptr;
		if (bpfh->bh_caplen==0 || bpfh->bh_hdrlen==0) break;
		size_t chunk_size=BPF_WORDALIGN(bpfh->bh_caplen+bpfh->bh_hdrlen);
		num_pkgs++;
		bytes_rcv+=bpfh->bh_datalen;
		struct DNS_HEADER *dns=(struct DNS_HEADER *)(ptr+bpfh->bh_hdrlen+14+sizeof(struct ip)+sizeof(struct udphdr));
		double rd=getQueryRTT(ntohs(dns->id));
		rtt+=rd;
		if (rd<min || min==0) min=rd;
		if (rd>max) max=rd;

		ptr+=chunk_size;
		done+=chunk_size;
	}
	buffer_acknowledge(zhdr);
}
void RawSocketReceiver::receive(ppluint64 &num_pkgs, ppluint64 &bytes_rcv, double &rtt, double &min, double &max)
{
	struct bpf_zbuf *zbuf=(struct bpf_zbuf*)buffer;
	struct bpf_zbuf_header *zhdr=NULL;
	if (buffer_check((struct bpf_zbuf_header *)zbuf->bz_bufa)) {
		zhdr=((struct bpf_zbuf_header *)zbuf->bz_bufa);
		read_zbuffer(zhdr, num_pkgs, bytes_rcv, rtt, min, max);
	}
	if (buffer_check((struct bpf_zbuf_header *)zbuf->bz_bufb)) {
		zhdr=((struct bpf_zbuf_header *)zbuf->bz_bufb);
		read_zbuffer(zhdr, num_pkgs, bytes_rcv, rtt, min, max);
	}
}

#else
bool RawSocketReceiver::receive(size_t &size, double &rtt)
{
	unsigned char *ptr=buffer;
	ssize_t bufused=recvfrom(sd,buffer,buflen,0,NULL,NULL);
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
	size=bufused-sizeof(struct ETHER);

	struct DNS_HEADER *dns=(struct DNS_HEADER *)(ptr+14+sizeof(struct ip)+sizeof(struct udphdr));

	rtt=getQueryRTT(ntohs(dns->id));
	return true;
}
#endif
