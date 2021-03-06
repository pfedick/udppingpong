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
#include <time.h>
#include <sys/time.h>


#include "udpecho.h"


/*!@file
 * \ingroup GroupSender
 */

/*!\class ReceiverThread
 * \ingroup GroupSender
 * \brief Empfangen und zählen der Antwort-Pakete
 *
 *
 */


/*!\brief Konstruktor
 *
 * Reserviert Speicher fuer die UDP-Pakete und initialisiert interne Variablen
 */
UDPEchoReceiverThread::UDPEchoReceiverThread()
{
	recbuffer.malloc(4096);
	sockfd=0;
	resetCounter();
}

/*!\brief Destruktor
 */
UDPEchoReceiverThread::~UDPEchoReceiverThread()
{

}

/*!\brief Socket Deskriptor setzen
 *
 * Übermittelt den Deskriptor eines bereits angelegten Sockets an den Receiver-Thread.
 *
 * @param sockfd Socket Deskriptor
 */
void UDPEchoReceiverThread::setSocketDescriptor(int sockfd)
{
	this->sockfd=sockfd;
}

/*!\brief Counter auf 0 setzen
 *
 * Alle Counter werden auf 0 gesetzt.
 */
void UDPEchoReceiverThread::resetCounter()
{
	bytes_received=0;
	counter_received=0;
	rtt_total=0.0;
	rtt_min=0.0;
	rtt_max=0.0;
}

void UDPEchoReceiverThread::countPacket(const PACKET *p, ssize_t bytes)
{
	counter_received++;
	bytes_received+=bytes;
	double rtt=ppl7::GetMicrotime()-p->time;
	rtt_total+=rtt;
	if (rtt_min==0) rtt_min=rtt;
	else if (rtt<rtt_min) rtt_min=rtt;
	if (rtt>rtt_max) rtt_max=rtt;
}

/*!\brief Hauptthread des Receivers
 *
 * Liest in einer Endlosschleife Pakete aus dem UDP-Buffer. Die Schleife wird nur dann
 * beendet, wenn dem Thread ein Signal zum Stoppen gegeben wurde.
 */
void UDPEchoReceiverThread::run()
{
	threadSetName("UDPEchoReceiverThread");
	struct timespec timeout;
	timeout.tv_sec=0;
	timeout.tv_nsec=10*1000000;
	fd_set rset;
	resetCounter();
	PACKET *p=(PACKET*)recbuffer.adr();
	time_t start = time(NULL);
	time_t next_check = start +1;
	while(1) {
		ssize_t n=::recv(sockfd,(void*)recbuffer.adr(),4096,0);
		if (n >= 0) {
			countPacket(p,n);
		} else {
			FD_ZERO(&rset);
			FD_SET(sockfd,&rset);
			pselect(sockfd+1,&rset,NULL,NULL,&timeout,NULL);
		}
		if (time(NULL) >= next_check) {
			next_check += 1;
			if (this->threadShouldStop())
				break;
		}
	}
}

/*!\brief Anzahl empfangender Pakete auslesen
 *
 * @return Anzahl Pakete
 */
int64_t UDPEchoReceiverThread::getPacketsReceived() const
{
	return counter_received;
}

/*!\brief Anzahl empfangender Bytes auslesen
 *
 * @return Anzahl Bytes
 */
int64_t UDPEchoReceiverThread::getBytesReceived() const
{
	return bytes_received;
}

/*!\brief Durchschnittliche Paketlaufzeit auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoReceiverThread::getRoundTripTimeAverage() const
{
	return rtt_total/(double)counter_received;
}

/*!\brief Minimale Paketlaufzeit auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoReceiverThread::getRoundTripTimeMin() const
{
	return rtt_min;
}

/*!\brief Maximale Paketlaufzeit auslesen
 *
 * @return Laufzeit in Sekunden, mit mikrosekundengenauen Nachkommastellen
 */
double UDPEchoReceiverThread::getRoundTripTimeMax() const
{
	return rtt_max;
}

