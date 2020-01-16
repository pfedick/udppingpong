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

#include "sensor.h"
#include "udpecho.h"

/*!@file
 * \ingroup GroupBouncer
 */

/*!\brief Stop-Flag
 *
 * Wird vom Signal-Handler sighandler gesetzt, wenn das Programm beendet werden soll.
 *
 */
bool stopFlag=false;

/*!\brief Signal-Handler
 *
 * Wird aufgerufen, wenn dem Programm ein Kill-Signal geschickt wird oder in der Konsole
 * Ctrl-C gedrückt wird. Er setzt die globale Variable stopFlag auf True, wodurch
 * Hauptthread und Workerthreads sich beenden.
 *
 * @param sig Art des Signals
 */
void sighandler(int)
{
	stopFlag=true;
}

/*!\brief Hilfe anzeigen
 *
 * Zeigt die Hilfe für die Konsolenparameter an.
 */
void help()
{
	printf ("Usage:\n"
			"  -h           zeigt diese Hilfe an\n"
			"  -s HOST:PORT Hostname oder IP und Port, an den sich der Echo-Server binden soll\n"
			"  -n #         Anzahl Worker-Threads (Default=1)\n"
			"  -q           quiet, es wird nichts auf stdout ausgegeben\n"
			"  -p #         Groesse der Antwortpakete (Default=so gross wie eingehendes Paket)\n"
			"  --noecho     Es werden keine Antworten zurueckgeschickt\n"
			"\n");

}


/*!\brief Gibt solange sekündlich eine Statusmeldung aus, bis das Programm gestoppt wird
 *
 * Gibt solange sekündlich eine Statusmeldung aus, bis das Programm gestoppt wird.
 */
void run(UDPEchoBouncer &bouncer, bool quiet)
{
	SystemStat stat_start;
	SystemStat stat_end;
	sampleSensorData(stat_start);
	double start = ppl7::GetMicrotime();
	double end = start + 1;

	while (stopFlag == false) {
		ppl7::MSleep(100);
		if (!quiet) {
			if (ppl7::GetMicrotime() >= end) {
				UDPEchoCounter counter=bouncer.getCounter();
				sampleSensorData(stat_end);
				printf("Packets per second: %10lu, Durchsatz: %10lu Mbit, RX: %8lu, TX: %8lu\n",
						counter.packets_received, counter.bytes_received*8/(1024*1024),
						stat_end.net_total.receive.packets-stat_start.net_total.receive.packets,
						stat_end.net_total.transmit.packets-stat_start.net_total.transmit.packets
						);
				stat_start=stat_end;
				end += 1.0;
			}
		}
	}
}

/*!\brief Hauptfunktion
 *
 * Wertet die Kommandozeilenparameter aus, setzt die Signal-Handles, startet die Workerthreads
 * und wartet, bis das programm beendet wird.
 *
 * @param argc Anzahl Kommandozeilenparameter
 * @param argv Liste mit char-Pointern auf die Kommandozeilenparameter
 * @return Gibt 0 zurück, wenn alles in Ordnung war, 1 wenn ein Fehler aufgetreten ist
 */
int main (int argc, char **argv)
{
	if (ppl7::HaveArgv(argc,argv,"-h") || ppl7::HaveArgv(argc,argv,"--help")) {
		help();
		return 0;
	}
	ppl7::String Server=ppl7::GetArgv(argc,argv,"-s");
	if (Server.isEmpty()) {
		help();
		return 1;
	}
	ppl7::Array a(Server,":");
	if (a.size()!=2) {
		printf ("ERROR: Invalid Hostname:Port [%s]\n",(const char*)Server);
		return 1;
	}
	ppl7::String Hostname=a[0];
	int Port=a[1].toInt();

	UDPEchoBouncer bouncer;
	bouncer.setInterface(Hostname, Port);


	bool quiet=ppl7::HaveArgv(argc,argv,"-q");
	bouncer.disableResponses(ppl7::HaveArgv(argc,argv,"--noecho"));
	int ThreadCount = ppl7::GetArgv(argc,argv,"-n").toInt();
	if (!ThreadCount) ThreadCount=1;

	size_t packetSize=ppl7::GetArgv(argc,argv,"-p").toInt();
	if (packetSize>0) {
		if (packetSize<32 || packetSize>4096) {
			printf ("ERROR: Paketgroesse muss zwischen 32 und 4096 Bytes liegen [%d]\n",(int)packetSize);
			return 1;
		}
		bouncer.setFixedResponsePacketSize(packetSize);
	}
	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);

	// Start Bouncer in his own Thread
	bouncer.start(ThreadCount);
	run(bouncer, quiet);

	if (!quiet)
		printf("Stoppe und loesche Worker-Threads\n");
	bouncer.stop();
	if (!quiet)
		printf("Bouncer stopped\n");
	return 0;
}


