/*
 * This file is part of udppingpong by Patrick Fedick <fedick@denic.de>
 *
 * Copyright (c) 2019 DENIC eG
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>

#include "udpecho.h"


/*!@file
 * \ingroup GroupBouncer
 */


/*!\class UDPEchoBouncerThread
 * \ingroup GroupBouncer
 * \brief Worker-Thread des Bouncers
 *
 */

/*!\class UDPEchoBouncerThread::Counter
 * \ingroup GroupBouncer
 * \brief Datenstruktur, die Anzahl Pakete und Bytes pro Sekunde aufnimmt.
 *
 */


/*!\brief Konstruktor
 *
 * Einige interne Variablen werden mit dem Default-Wert befüllt und Speicher für die
 * UDP-Pakete allokiert. Ausserdem wird ein Socket angelegt und so konfiguriert, dass
 * weitere Sockets auf der gleichen IP und Port erlaubt sind.
 */
UDPEchoBouncerThread::UDPEchoBouncerThread()
{
	noEcho=false;
	sockfd=0;
	counter.packets_received=0;
	counter.packets_send=0;
	counter.bytes_received=0;
	counter.bytes_send=0;
	buffer.malloc(4096);
	pBuffer=(void*)buffer.adr();
	sockfd=0;
	packetSize=0;
}

/*!\brief Destruktor
 *
 */
UDPEchoBouncerThread::~UDPEchoBouncerThread()
{
	if (sockfd) ::close(sockfd);
}

/*!\brief Pakete nicht beantworten
 *
 * Wird diese Funktion mit \p true aufgerufen, werden die eingehenden Pakete nur gezählt, aber
 * nicht beantwortet.
 *
 * @param flag True oder False
 */
void UDPEchoBouncerThread::setNoEcho(bool flag)
{
	noEcho=flag;
}


/*!\brief Paketgröße für Antwortpakete festlegen
 *
 * Legt die Große der Antwortpakete fest.
 * @param bytes Wert zwischen 32 und 4096. Bei Angabe von 0 sind die Antwortpakete
 * genauso groß, wie die eingehenden Pakete.
 */
void UDPEchoBouncerThread::setPacketSize(size_t bytes)
{
	if ((bytes<=4096 && bytes>=32) || bytes==0)
		packetSize=bytes;
}



void UDPEchoBouncerThread::bind(const ppl7::SockAddr &sockaddr)
{
	if (sockfd) ::close(sockfd);
	sockfd=::socket(AF_INET, SOCK_DGRAM, 0);
	if (!sockfd) throw ppl7::CouldNotOpenSocketException("Could not create Socket");
	const int trueValue = 1;
	// Wir erlauben anderen Threads/Programmen sich auf das gleichen Socket zu binden
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
#ifdef SO_REUSEPORT_LB
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT_LB, &trueValue, sizeof(trueValue));
#else
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &trueValue, sizeof(trueValue));
#endif

	memset(&servaddr,0,sizeof(servaddr));
	memcpy(&servaddr,sockaddr.addr(),sockaddr.size());
	// Socket an die IP-Adresse und den Port binden
	if (0 != ::bind(sockfd,(const struct sockaddr *)&servaddr, sizeof(servaddr))) {
		int e=errno;
		throw ppl7::CouldNotBindToInterfaceException("%s:%d, %s",
				(const char*)sockaddr.toIPAddress().toString(),
				sockaddr.port(),
				strerror(e));
	}
	// Der Socket soll nicht blockieren, wenn keine Daten anstehen
	fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0)|O_NONBLOCK);
	int optval = 1;
	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	socklen_t size=sizeof(optval);
	if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, &size) < 0) {
		throw ppl7::CouldNotBindToInterfaceException("%s:%d, %s",
						(const char*)sockaddr.toIPAddress().toString(),
						sockaddr.port(),
						strerror(errno));
	}
	size = optval * 2;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}




/*!\brief Aktuelle Zähler auslesen und auf 0 setzen
 *
 * Die aktuellen Zähler für Anzahl Pakete und Bytes werden ausgelesen und auf 0 zurückgesetzt.
 *
 * @return Datenstruktur UDPEchoBouncerThread::Counter
 */
UDPEchoCounter UDPEchoBouncerThread::getAndClearCounter()
{
	UDPEchoCounter ret;
	mutex.lock();
	ret=counter;
	counter.packets_received=0;
	counter.packets_send=0;
	counter.bytes_received=0;
	counter.bytes_send=0;
	mutex.unlock();
	//printf ("Thread %llu: %llu\n",this->threadGetID(),ret.count);
	return ret;
}

bool UDPEchoBouncerThread::waitForSocketReadable()
{
	struct timespec timeout;
	timeout.tv_sec=0;
	timeout.tv_nsec=500*1000000;
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(sockfd,&rset);
	if (pselect(sockfd+1,&rset,NULL,NULL,&timeout,NULL)>0) {
		if (FD_ISSET(sockfd,&rset)) return true;
	}
	return false;
}

/*!\brief Thread des Workerthreads
 *
 * Diese Methode wird in einem separaten Thread gestartet und wartet in einer Endlos-
 * schleife auf eingehende Pakete. Sobald ein Paket eingeht, wird es an den Absender
 * zurückgeschickt, sofern dies nicht vorher mit UDPEchoBouncerThread::setNoEcho abgeschaltet wurde.
 */
void UDPEchoBouncerThread::run()
{
	struct sockaddr_in cliaddr;
	time_t start = time(NULL);
	time_t next_check = start +1;
	//int socksend=::dup(sockfd);
	while (1) {
		socklen_t clilen = sizeof(cliaddr);
		ssize_t n = ::recvfrom(sockfd, pBuffer, 4096, 0, (struct sockaddr*) (&cliaddr), &clilen);
		ssize_t bytes_send=0;
		if (n >= 0) {
			// Paket zurueck an Absender schicken
			if (!noEcho) {
				if (!packetSize) {
					bytes_send+=n;
					::sendto(sockfd, (void*) pBuffer, n, 0, (struct sockaddr*) (&cliaddr), clilen);
				} else {
					bytes_send=packetSize;
					::sendto(sockfd, (void*) pBuffer, packetSize, 0, (struct sockaddr*) (&cliaddr), clilen);
				}
			}
			//mutex.lock();
			counter.bytes_send+=n;
			if (!noEcho) counter.packets_send++;
			counter.bytes_received+=n;
			counter.packets_received++;
			//mutex.unlock();
		} else {
			waitForSocketReadable();
		}

		if (time(NULL) >= next_check) {
			next_check += 1;
			if (this->threadShouldStop())
				break;

		}
	}
	//::close(socksend);
}
