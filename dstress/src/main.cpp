#include <string.h>
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


int main(int argc, char **argv)
{
	try {
		RawSocket sock;
		sock.setDestination("148.251.94.99",53);

		Packet pkt;
		pkt.setDestination("148.251.94.99",53);
		//pkt.setSource("10.122.65.210",0x8000);
		pkt.setSource("148.251.94.99",0x8000);
		pkt.setId(1234);
		pkt.setPayloadDNSQuery("pfp.de SOA");

		ppl7::HexDump(pkt.ptr(),pkt.size());
		int bytes=sock.send(pkt);
		printf ("%d bytes gesendet\n",bytes);
		return 0;

	} catch (const ppl7::Exception &exp) {
		exp.print();
		return 1;
	}
}
