#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

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
ppluint64 PacketId=0;


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
}

/*!\brief Destruktor
 *
 * Stoppt den ReceiverThread (sofern er noch läuft) und schließt den Socket.
 */
UDPEchoSenderThread::~UDPEchoSenderThread()
{
	receiver.threadStop();
	Socket.disconnect();
}


/*!\brief Zieladresse setzen
 *
 * @param destination String mit Zieladresse und Port
 */
void UDPEchoSenderThread::setDestination(const ppl7::String &destination)
{
	this->destination=destination;
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
void UDPEchoSenderThread::setQueryRate(ppluint64 qps)
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

void UDPEchoSenderThread::setSourceIP(const ppl7::String &ip)
{
	Socket.setSource(ip);
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
	Socket.connect(destination);
	Socket.setBlocking(false);
	sockfd=Socket.getDescriptor();
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
	Socket.disconnect();
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
	ppluint64 total_zeitscheiben=runtime*1000/(Zeitscheibe*1000.0);
	ppluint64 queries_rest=runtime*queryrate;
	ppl7::SockAddr addr=Socket.getSockAddr();
	if (verbose) {
		printf ("Laufzeit: %d s, Dauer Zeitscheibe: %0.6f s, Zeitscheiben total: %llu, Qpzs: %llu, Source: %s:%d\n",
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

	for (ppluint64 z=0;z<total_zeitscheiben;z++) {
		naechste_zeitscheibe+=Zeitscheibe;
		ppluint64 restscheiben=total_zeitscheiben-z;
		ppluint64 queries_pro_zeitscheibe=queries_rest/restscheiben;
		if (restscheiben==1)
			queries_pro_zeitscheibe=queries_rest;
		for (ppluint64 i=0;i<queries_pro_zeitscheibe;i++) {
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
ppluint64 UDPEchoSenderThread::getPacketsSend() const
{
	return counter_send;
}

/*!\brief Anzahl empfangender Pakete auslesen
 *
 * @return Anzahl Pakete
 */
ppluint64 UDPEchoSenderThread::getPacketsReceived() const
{
	return receiver.getPacketsReceived();
}

/*!\brief Anzahl empfangender Bytes auslesen
 *
 * @return Anzahl Bytes
 */
ppluint64 UDPEchoSenderThread::getBytesReceived() const
{
	return receiver.getBytesReceived();
}

/*!\brief Anzahl beim Senden aufgetretener Fehler auslesen
 *
 * @return Anzahl Fehler
 */
ppluint64 UDPEchoSenderThread::getErrors() const
{
	return errors;
}

ppluint64 UDPEchoSenderThread::getCounter0Bytes() const
{
	return counter_0bytes;
}

ppluint64 UDPEchoSenderThread::getCounterErrorCode(int err) const
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

