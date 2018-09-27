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


int MakeQuery(const ppl7::String &query, unsigned char *buffer, size_t buffersize)
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
			return bytes;
		}
		t++;
	}
	throw UnknownRRType(tok[1]);
}

