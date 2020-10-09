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
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "udpecho.h"


/*!@file
 * \ingroup GroupSender
 */


/*!\brief Globaler Mutex für die Paket-ID
 *
 */
ppl7::Mutex PacketIdMutex;

/*!\brief Globaler Zähler für die Paket-ID
 *
 */
int64_t PacketId=0;


/*!\class SenderThread
 * \ingroup GroupSender
 * \brief Worker-Thread des Senders
 *
 */


/*!\brief Konstruktor
 *
 * Einige interne Variablen werden mit dem Default-Wert befüllt.
 */
UDPEchoSenderThread::UDPEchoSenderThread()
{
	packetsize=512;
	runtime=10;
	timeout=5;
	counter_send=0;
	errors=0;
	counter_0bytes=0;
	duration=0.0;
	ignoreResponses=true;
	for (int i=0;i<255;i++) counter_errorcodes[i]=0;
	verbose=false;
	alwaysRandomize=false;
	queryrate=0;
	Zeitscheibe=0.0f;
	sockfd=::socket(AF_INET, SOCK_DGRAM, 0);
	if (!sockfd) throw ppl7::CouldNotOpenSocketException("Could not create Socket");
	const int trueValue = 1;
	// Wir erlauben anderen Threads/Programmen sich auf das gleichen Socket zu binden
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
	#ifdef SO_REUSEPORT_LB
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT_LB, &trueValue, sizeof(trueValue));
		//setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &trueValue, sizeof(trueValue));
	#else
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &trueValue, sizeof(trueValue));
	#endif

}

/*!\brief Destruktor
 *
 * Stoppt den ReceiverThread (sofern er noch läuft) und schließt den Socket.
 */
UDPEchoSenderThread::~UDPEchoSenderThread()
{
	receiver.threadStop();
	if (sockfd) ::close(sockfd);
}

/*!\brief Zieladresse setzen
 *
 * @param destination String mit Zieladresse und Port
 */
void UDPEchoSenderThread::connect(const ppl7::String &destination)
{
	if (destination.isEmpty())
		throw ppl7::IllegalArgumentException("UDPEchoSenderThread::connect(const String &destination)");
	ppl7::Array hostname = StrTok(destination, ":");
	if (hostname.size() != 2)
		throw ppl7::IllegalArgumentException("UDPEchoSenderThread::connect(const String &destination)");
	ppl7::String portname = hostname.get(1);
	int port = portname.toInt();
	if (port <= 0 && portname.size() > 0) {
		// Vielleicht wurde ein Service-Namen angegeben?
		struct servent *s = getservbyname((const char*) portname, "udp");
		if (s) {
			unsigned short int p = s->s_port;
			port = (int) ntohs(p);
		} else {
			throw ppl7::IllegalPortException("UDPEchoSenderThread::connect(const String &destination=%s)", (const char*) destination);
		}
	}
	if (port <= 0)
		throw ppl7::IllegalPortException("UDPEchoSenderThread::connect(const String &destination=%s)", (const char*) destination);
	connect(hostname.get(0), port);
}

void UDPEchoSenderThread::connect(const ppl7::String &hostname, int port)
{
	struct addrinfo hints, *res, *ressave;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	char portstr[10];
	sprintf(portstr, "%i", port);
	int n=0;
	if ((n=getaddrinfo((const char*) hostname, portstr, &hints, &res)) != 0)
		throwExceptionFromEaiError(n, ppl7::ToString("UDPEchoSenderThread::connect: host=%s, port=%i", (const char*) hostname, port));
	ressave = res;
	int e = 0, conres = 0;
	do {
		conres = ::connect(sockfd, res->ai_addr, res->ai_addrlen);
		ppl7::SockAddr current_addr=ppl7::SockAddr((const void*)res->ai_addr,(size_t)res->ai_addrlen);
		printf("connecting to %s, conres=%d\n",(const char*)current_addr.toIPAddress().toString(), conres);
		e = errno;
		if (conres == 0) break;
	} while ((res = res->ai_next) != NULL);
	freeaddrinfo(ressave);
	if (conres !=0 || res==NULL) {
		ppl7::throwSocketException(e, ppl7::ToString("Host: %s, Port: %d", (const char*) hostname, port));
	}

	// Der Socket soll nicht blockieren, wenn keine Daten anstehen
	fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0)|O_NONBLOCK);
	int optval = 1;
	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	socklen_t size=sizeof(optval);
	if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, &size) < 0) {
		ppl7::throwSocketException(errno, ppl7::ToString("getsockopt failed on Host: %s, Port: %d", (const char*) hostname, port));
	}
	size = optval * 2;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size))!=0) {
		ppl7::throwSocketException(errno, ppl7::ToString("setsockopt failed on Host: %s, Port: %d", (const char*) hostname, port));
	}
}

/*!\brief Paketgröße setzen
 *
 * Setzt die Paketgröße
 * @param size Paketgröße in Bytes
 */
void UDPEchoSenderThread::setPacketsize(size_t size)
{
	packetsize=size;
}

/*!\brief Laufzeit festlegen
 *
 * Legt die Laufzeit für den Testlauf fest.
 *
 * @param seconds Laufzeit
 */
void UDPEchoSenderThread::setRuntime(int seconds)
{
	runtime=seconds;
}

/*!\brief Timeout setzen
 *
 * Setzt den Timeout für den Lasttest. Nachdem die Laufzeit für das Senden von Paketen abgelaufen ist,
 * wird noch \seconds Sekunden auf rückkehrende Pakete gewartet.
 *
 * @param seconds Timeout
 */
void UDPEchoSenderThread::setTimeout(int seconds)
{
	timeout=seconds;
}

/*!\brief Gewünschte Query-Rate pro Sekunde einstellen
 *
 * Ein Wert > 0 aktiviert das Rate-Limiting. Der Sender versucht die gewünschte Anzahl Pakete
 * gleichmäßig auf die Sekunde zu verteilen. Dazu werden Zeitscheiben verwendet, deren Dauer
 * mit der Methode SenderThread::setZeitscheibe konfiguriert werden kann.
 *
 * @param qps Queries pro Sekunde
 */
void UDPEchoSenderThread::setQueryRate(int64_t qps)
{
	queryrate=qps;
}

/*!\brief Dauer einer Zeitscheibe bei aktiviertem Rate-Limit einstellen
 *
 * Legt die Dauer einer Zeitscheibe bei aktiviertem Rate-Limit fest. Dabei werden die zu
 * sendenen Pakete pro Sekunde gleichmäßig auf alle Zeitscheiben in der Sekunde verteilt.
 *
 * Der Wert muss mindestens 1 sein (=Default), maximal 1000. Ferner muss der Wert von
 * 1000 teilbar sein.
 *
 * @param ms Dauer einer Zeitscheibe in Millisekunden
 *
 * @exception ppl7::InvalidArgumentsException Wird geworfen, wenn der Wert \p ms nicht den
 * genannten kriterien entspricht.
 */
void UDPEchoSenderThread::setZeitscheibe(float ms)
{
	if (ms==0.0f || ms >1000.0f) throw ppl7::InvalidArgumentsException();
	//if ((1000 % ms)!=0) throw ppl7::InvalidArgumentsException();
	Zeitscheibe=(double)ms/1000;
}


/*!\brief Antwortpakete ignorieren
 *
 * Falls gesetzt, werden nur Pakete rausgeschickt, die Antworten aber ignoriert
 *
 * @param flag True oder False
 */
void UDPEchoSenderThread::setIgnoreResponses(bool flag)
{
	ignoreResponses=flag;
}

static ppl7::SockAddr getSockAddr(const ppl7::String &Hostname, int Port)
{
	std::list<ppl7::IPAddress> result;
	size_t hostcount=ppl7::GetHostByName(Hostname, result);
	if (hostcount<1) {
		throw ppl7::HostNotFoundException("Hostname nicht aufloesbar [%s]",(const char*)Hostname);
	} else if (hostcount>1) {
		throw ppl7::ResolverException("ERROR: Hostname nicht eindeutig, er loest mit %td IP-Adressen auf\n",hostcount);
	}
	ppl7::IPAddress &address=result.front();
	return ppl7::SockAddr(address,Port);
}



void UDPEchoSenderThread::setSourceIP(const ppl7::String &ip)
{
	ppl7::SockAddr sockaddr=::getSockAddr(ip,0);
	struct sockaddr_in servaddr;
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
}

void UDPEchoSenderThread::setVerbose(bool verbose)
{
	this->verbose=verbose;
}

void UDPEchoSenderThread::setAlwaysRandomize(bool flag)
{
	this->alwaysRandomize=flag;
}

bool UDPEchoSenderThread::socketReady()
{
	fd_set wset;
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=100;
	FD_ZERO(&wset);
	FD_SET(sockfd,&wset); // Wir wollen nur prüfen, ob wir schreiben können
	int ret=select(sockfd+1,NULL,&wset,NULL,&timeout);
	if (ret<0) return false;
	if (FD_ISSET(sockfd,&wset)) {
		return true;
	}
	return false;
}



/*!\brief Einzelnes Paket senden
 *
 * Generiert ein neues Paket. Die ersten 8 Byte enthalten dabei eine eindeutige fortlaufende
 * ID, die nächsten 8 Byte einen Wert in Double-Precision mit der aktuellen, mikrosekunden genauen
 * Uhrzeit des Servers. Anhand der Uhrzeit kann die Laufzeit eines rückkehrenden Pakets berechnet
 * werden.
 */
void UDPEchoSenderThread::sendPacket()
{
	PACKET *p=(PACKET*)buffer.ptr();
	if (alwaysRandomize) {
		char *b=(char*)p;
		for (size_t i=0;i<packetsize;i++) {
			b[i]=(char)ppl7::rand(0,255);
		}
	}
	p->time=ppl7::GetMicrotime();
	ssize_t n=::send(sockfd,p,packetsize,0);
	if (n>0 && (size_t)n==packetsize) {
		counter_send++;
	} else if (n<0) {
		if (errno<255) counter_errorcodes[errno]++;
		errors++;
	} else {
		counter_0bytes++;
	}
}

/*!\brief Worker-Thread
 *
 * Diese Methode ist der Einstiegspunkt fuer den Workerthread. Hier wird der Socket initialisiert
 * und mit dem Ziel "connected". Sofern Antwortpakete nicht ignoriert werden sollen, wird noch
 * der ReceiverThread gestartet und dann die Last. Jenachdem, ob ein Ratelimit angegeben
 * wurde oder nicht, wird entweder die Methode SenderThread::runWithRateLimit oder SenderThread::runWithoutRateLimit
 * aufgerufen. Nach Ablauf der Laufzeit wird dann noch die Methode SenderThread::waitForTimeout
 * aufgerufen, bevor der Socket wieder geschlossen wird.
 */
void UDPEchoSenderThread::run()
{
	buffer=ppl7::Random(packetsize);
	receiver.setSocketDescriptor(sockfd);
	receiver.resetCounter();
	if (!ignoreResponses)
		receiver.threadStart();
	counter_send=0;
	counter_0bytes=0;
	errors=0;
	duration=0.0;
	for (int i=0;i<255;i++) counter_errorcodes[i]=0;
	double start=ppl7::GetMicrotime();
	if (queryrate>0) {
		runWithRateLimit();
	} else {
		runWithoutRateLimit();
	}
	duration=ppl7::GetMicrotime()-start;
	waitForTimeout();
	receiver.threadStop();
	//close(sockfd);
}

/*!\brief Generiert und empfängt soviele Pakete wie möglich
 *
 * In einer Endlosschleife werden permanent Pakete generiert und versenden.
 */
void UDPEchoSenderThread::runWithoutRateLimit()
{
	double start=ppl7::GetMicrotime();
	double end=start+(double)runtime;
	double now,next_checktime=start+0.1;
	while (1) {
		if (socketReady()) {
			sendPacket();
		}
		now=ppl7::GetMicrotime();
		if (now>next_checktime) {
			next_checktime=now+0.1;
			if (this->threadShouldStop()) break;
		}
		if (now>end) break;
	}
}

/*!\brief Nanosekundengenaue Uhrzeit auslesen
 *
 * Verwendet die Realtime-Uhr des Betriebssystems um die nanosekundengenaue
 * Uhrzeit auszulesen.
 * @return Uhrzeit als Gleitkommazahl. Die Vorkommastellen enthalten die Sekunden seit
 * Epoch, die Nachkommastellen die Nanosekunden.
 */
static inline double getNsec()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (double)ts.tv_sec+((double)ts.tv_nsec/1000000000.0);
}

ppl7::SockAddr UDPEchoSenderThread::getSockAddr() const
{
	if (!sockfd)
		throw ppl7::NotConnectedException();
	struct sockaddr addr;
	socklen_t len=sizeof(addr);
	int ret=getsockname(sockfd, &addr, &len);
	if (ret<0) ppl7::throwSocketException(errno, "UDPEchoSenderThread::getSockAddr");
	return ppl7::SockAddr((const void*)&addr,(size_t)len);
}


/*!\brief Generiert eine gewünschte Anzahl Pakete pro Sekunde
 *
 * In einer Endlosschleife werden Pakete generiert und versendet.
 * Um die gewünschte Queryrate zu erreichen, wird die Laufzeit in Zeitscheiben unterteilt und berechnet,
 * wieviele Pakete pro Zeitscheibe versendet werden müssen.
 *
 * \see SenderThread::setRuntime Setzen der Laufzeit
 * \see SenderThread::setQueryRate Setzen der Queryrate
 * \see SenderThread::setZeitscheibe Setzen der Länge einer Zeitscheibe
 *
 * \note Falls das Senden der Pakete länger als die Zeitscheibe dauert, kann die gewünschte Queryrate
 * nicht erreichtwerden. Es liegt ein Bottleneck auf dem Lastgenerator vor. Dies dürfte in der Regel die
 * CPU sein.
 */
void UDPEchoSenderThread::runWithRateLimit()
{
	struct timespec ts;
	int64_t total_zeitscheiben=runtime*1000/(Zeitscheibe*1000.0);
	int64_t queries_rest=runtime*queryrate;
	ppl7::SockAddr addr=getSockAddr();
	if (verbose) {
		printf ("Laufzeit: %d s, Dauer Zeitscheibe: %0.6f s, Zeitscheiben total: %lu, Qpzs: %lu, Source: %s:%d\n",
				runtime,Zeitscheibe,total_zeitscheiben,
				queries_rest/total_zeitscheiben,
				(const char*)addr.toIPAddress().toString(), addr.port());
	}
	double now=getNsec();
	double naechste_zeitscheibe=now;
	double next_checktime=now+0.1;

	double start=ppl7::GetMicrotime();
	double end=start+(double)runtime;
	double total_idle=0.0;

	for (int64_t z=0;z<total_zeitscheiben;z++) {
		naechste_zeitscheibe+=Zeitscheibe;
		int64_t restscheiben=total_zeitscheiben-z;
		int64_t queries_pro_zeitscheibe=queries_rest/restscheiben;
		if (restscheiben==1)
			queries_pro_zeitscheibe=queries_rest;
		for (int64_t i=0;i<queries_pro_zeitscheibe;i++) {
			sendPacket();
		}

		queries_rest-=queries_pro_zeitscheibe;
		while ((now=getNsec())<naechste_zeitscheibe) {
			if (now<naechste_zeitscheibe) {
				total_idle+=naechste_zeitscheibe-now;
				ts.tv_sec=0;
				ts.tv_nsec=(naechste_zeitscheibe-now)*1000000000;
				nanosleep(&ts,NULL);
			}
		}
		if (now>next_checktime) {
			next_checktime=now+0.1;
			if (this->threadShouldStop()) break;
			if (ppl7::GetMicrotime()>=end) break;
			//printf ("Zeitscheiben rest: %llu\n", z);
		}
	}
	if (verbose) {
		printf ("total idle: %0.6f\n",total_idle);
	}
}


/*!\brief Wartet solange auf rückkehrende Pakete, bis der Timeout erreicht ist
 *
 * Diese Methode wird aufgerufen, nachdem das Senden der Pakete beendet wurde. Sie wartet
 * noch solange auf rückkehrende Pakete, bis der mittels SenderThread::setTimeout
 * eingestellte Timeout erreicht ist
 */
void UDPEchoSenderThread::waitForTimeout()
{
	double start=ppl7::GetMicrotime();
	double end=start+(double)timeout;
	double now, next_checktime=start+0.1;
	while ((now=ppl7::GetMicrotime())<end) {
		if (now>next_checktime) {
			next_checktime=now+0.1;
			if (this->threadShouldStop()) break;
		}
		ppl7::MSleep(10);
	}
}

/*!\brief Anzahl gesendeter Pakete auslesen
 *
 * @return Anzahl Pakete
 */
int64_t UDPEchoSenderThread::getPacketsSend() const
{
	return counter_send;
}

/*!\brief Anzahl empfangender Pakete auslesen
 *
 * @return Anzahl Pakete
 */
int64_t UDPEchoSenderThread::getPacketsReceived() const
{
	return receiver.getPacketsReceived();
}

/*!\brief Anzahl empfangender Bytes auslesen
 *
 * @return Anzahl Bytes
 */
int64_t UDPEchoSenderThread::getBytesReceived() const
{
	return receiver.getBytesReceived();
}

/*!\brief Anzahl beim Senden aufgetretener Fehler auslesen
 *
 * @return Anzahl Fehler
 */
int64_t UDPEchoSenderThread::getErrors() const
{
	return errors;
}

int64_t UDPEchoSenderThread::getCounter0Bytes() const
{
	return counter_0bytes;
}

int64_t UDPEchoSenderThread::getCounterErrorCode(int err) const
{
	if (err < 255) return counter_errorcodes[err];
	return 0;}


/*!\brief Tatsächliche Laufzeit des Tests auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoSenderThread::getDuration() const
{
	return duration;
}

/*!\brief Durchschnittliche Paketlaufzeit auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoSenderThread::getRoundTripTimeAverage() const
{
	return receiver.getRoundTripTimeAverage();
}

/*!\brief Minimale Paketlaufzeit auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoSenderThread::getRoundTripTimeMin() const
{
	return receiver.getRoundTripTimeMin();
}

/*!\brief Maximale Paketlaufzeit auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoSenderThread::getRoundTripTimeMax() const
{
	return receiver.getRoundTripTimeMax();
}

