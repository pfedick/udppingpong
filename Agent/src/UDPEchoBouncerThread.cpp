#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

#include "../include/udpecho.h"


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



/*!\brief Socket Deskriptor setzen
 *
 * Übermittelt den Deskriptor eines bereits angelegten Sockets an den Worker-Thread.
 *
 * @param sockfd Socket Deskriptor
 */
void UDPEchoBouncerThread::setSocketDescriptor(int sockfd)
{
	this->sockfd=sockfd;
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

/*!\brief Thread des Workerthreads
 *
 * Diese Methode wird in einem separaten Thread gestartet und wartet in einer Endlos-
 * schleife auf eingehende Pakete. Sobald ein Paket eingeht, wird es an den Absender
 * zurückgeschickt, sofern dies nicht vorher mit UDPEchoBouncerThread::setNoEcho abgeschaltet wurde.
 */
void UDPEchoBouncerThread::run()
{
	struct sockaddr_in cliaddr;
	struct timespec timeout;
	timeout.tv_sec=0;
	timeout.tv_nsec=10*1000000;
	fd_set rset;
	double start = ppl7::GetMicrotime();
	double next_check = start + 0.2;
	while (1) {
		socklen_t clilen = sizeof(cliaddr);
		ssize_t n = ::recvfrom(sockfd, pBuffer, 4096, 0, (struct sockaddr*) (&cliaddr), &clilen);
		if (n >= 0) {
			// Paket zurueck an Absender schicken
			if (!noEcho) {
				counter.packets_send++;
				if (!packetSize) {
					counter.bytes_send+=n;
					::sendto(sockfd, (void*) pBuffer, n, 0, (struct sockaddr*) (&cliaddr), clilen);
				} else {
					counter.bytes_send+=packetSize;
					::sendto(sockfd, (void*) pBuffer, packetSize, 0, (struct sockaddr*) (&cliaddr), clilen);
				}
			}
			mutex.lock();
			counter.bytes_received+=n;
			counter.packets_received++;
			mutex.unlock();
		} else {
			// Auf naechstes Paket warten, maximal 10 ms
			FD_ZERO(&rset);
			FD_SET(sockfd,&rset);
			pselect(sockfd+1,&rset,NULL,NULL,&timeout,NULL);
		}
		if (ppl7::GetMicrotime() >= next_check) {
			next_check += 0.2;
			if (this->threadShouldStop())
				break;
		}
	}
}
