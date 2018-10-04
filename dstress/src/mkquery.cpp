#include <ppl7.h>
#include <ppl7-inet.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#include "dstress.h"

static const char *rr_types[] = {
		"A", "NS", "MD", "MF", "CNAME", "SOA", "MB", "MG",
		"MR", "NULL", "WKS", "PTR", "HINFO", "MINFO", "MX", "TXT",
		"AAAA", "SRV", "NAPTR", "A6", "TKEY", "IXFR", "AXFR", "MAILB", "MAILA", "*", "ANY", "DS",
		NULL
};
static int rr_code[] = {
		1, 2, 3, 4, 5, 6, 7, 8, \
		9, 10, 11, 12, 13, 14, 15, 16, \
		28, 33, 35, 38, 249, 251, 252, 253, 254, 255, 255, 43 \
};


struct DNS_HEADER
{
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};


#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
struct DNS_OPT
{
	unsigned char name;
	unsigned short type;
	unsigned short udp_payload_size;
	unsigned char extended_rcode;
	unsigned char edns0_version;
	unsigned short z;
	unsigned short data_length;
};
#pragma pack(pop)   /* restore original alignment from stack */

unsigned char edns0[]= {0x00, 0x00, 0x29, 0x10, 00, 00, 00, 0x80, 00, 00, 0x00, 00, 0x0a, 00, 0x08,
		0xd1, 0xfe, 0xa6, 0x1c, 0xe5, 0x17, 0xc8, 0xe4};

int MakeQuery(const ppl7::String &query, unsigned char *buffer, size_t buffersize, bool dnssec, int udp_payload_size)
{
	ppl7::Array tok(query," ");
	if (tok.size()!=2) throw InvalidDNSQuery(query);
	ppl7::String Type=tok[1].toUpperCase();

	int t=0;
	const char *str=Type.c_str();
	while (rr_types[t]!=NULL) {
		if(!strcmp(str,rr_types[t])) {
			int bytes=res_mkquery(QUERY,
					(const char*) tok[0],
					C_IN,
					rr_code[t],
					NULL,0,NULL,buffer,(int)buffersize);
			if (bytes<0) throw InvalidDNSQuery("%s", hstrerror(h_errno));
			if (!dnssec) return bytes;
			//buffer[3]|=32;
			DNS_HEADER *dns=(DNS_HEADER*)buffer;
			dns->ad=1;
			dns->add_count=htons(1);
			DNS_OPT *opt=(DNS_OPT*)(buffer+bytes);
			memset(opt,0,11);
			opt->type=htons(41);
			opt->udp_payload_size=htons(udp_payload_size);
			opt->z=htons(0x8000);	// DO-bit
			return bytes+11;
		}
		t++;
	}
	throw UnknownRRType(tok[1]);
}

