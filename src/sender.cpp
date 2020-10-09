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
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "sender.h"
#include "sensor.h"
#include <set>

/*!@file
 * \ingroup GroupSender
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
void sighandler(int sig)
{
	stopFlag=true;
	printf ("Stopping...\n");
}

/*!\brief Hauptfunktion
 *
 * Einsprungspunkt des Programms. Erstellt eine Instanz von UDPSender und ruft
 * UDPSender::main auf.
 *
 * @param argc Anzahl Kommandozeilenparameter
 * @param argv Liste mit char-Pointern auf die Kommandozeilenparameter
 * @return Gibt 0 zurück, wenn alles in Ordnung war, 1 wenn ein Fehler aufgetreten ist
 */
int main(int argc, char**argv)
{
	UDPSender Sender;
	return Sender.main(argc,argv);
}


/*!\class UDPSender
 * \ingroup GroupSender
 * \brief Hauptmodul des Senders
 *
 */

/*!\brief Hilfe anzeigen
 *
 * Zeigt die Hilfe für die Konsolenparameter an.
 */
void UDPSender::help()
{
	printf ("Usage:\n"
			"  -h            zeigt diese Hilfe an\n"
			"  -z HOST:PORT  Hostname oder IP und Port des Zielservers\n"
			"  -p #          Paketgroesse (Default=512 Byte)\n"
			"  -l #          Laufzeit in Sekunden (Default=10 Sekunden)\n"
			"  -t #          Timeout in Sekunden (Default=5 Sekunden)\n"
			"  -n #          Anzahl Worker-Threads (Default=1)\n"
			"  -r #          Queryrate (Default=soviel wie geht)\n"
			"                Kann auch eine Kommaseparierte Liste sein (rate,rate,...) oder eine\n"
			"                Range (von rate - bis rate, Schrittweite)\n"
			"  -i #          Länge einer Zeitscheibe in Millisekunden (nur in Kombination mit -r, Default=1)\n"
			"                Wert muss zwischen 1 und 1000 liegen und \"Wert/1000\" muss aufgehen\n"
			"  -c FILE       CSV-File fuer Ergebnisse\n"
			"  --ignore      Ignoriere die Antworten\n"
			"  -b ADR,ADR... Optional: Liste von Quelladressen\n"
			"  --bl FILE     Optional: Datei mit Liste von Quelladressen\n"
			"  --ar          Optional: Payload immer randomisieren\n"
			"\n");
			//"  -m Messe Laufzeiten (Default=keine Zeitmessung)\n"
}


/*!\brief Konstruktor
 *
 * Einige interne Variablen werden mit dem Default-Wert befüllt.
 */
UDPSender::UDPSender()
{
	Packetsize=512;
	Laufzeit=10;
	Timeout=5;
	ThreadCount=1;
	Zeitscheibe=1.0f;
	ignoreResponses=false;
	alwaysRandomize=false;
}

/*!\brief Liste der zu testenden Queryrates erstellen
 *
 * In abhängigkeit des Wertes des Kommandozeilenparameters -r wird eine Liste
 * der zu testenden Queryraten erstellt.
 *
 * @param QueryRates String mit Wert des Kommandozeilenparameters -r
 * @return Liste mit den Queryraten
 */
ppl7::Array UDPSender::getQueryRates(const ppl7::String &QueryRates)
{
	ppl7::Array rates;
	if (QueryRates.isEmpty()) {
		rates.add("0");
	} else {
		ppl7::Array matches;
		if (QueryRates.pregMatch("/^([0-9]+)-([0-9]+),([0-9]+)$", matches)) {
			for (uint64_t i = matches[1].toUnsignedInt64(); i <= matches[2].toUnsignedInt64(); i += matches[3].toUnsignedInt64()) {
				rates.addf("%lu", i);
			}

		} else {
			rates.explode(QueryRates, ",");
		}
	}
	return rates;
}

/*!\brief Hauptfunktion
 *
 * Wertet die Kommandozeilenparameter aus, bereitet die Workerthreads vor und
 * arbeitet die gewünschten Laststufen ab.
 *
 * @param argc Anzahl Kommandozeilenparameter
 * @param argv Liste mit char-Pointern auf die Kommandozeilenparameter
 * @return Gibt 0 zurück, wenn alles in Ordnung war, 1 wenn ein Fehler aufgetreten ist
 */
int UDPSender::main(int argc, char**argv)
{
	if (ppl7::HaveArgv(argc,argv,"-h") || ppl7::HaveArgv(argc,argv,"--help")) {
		help();
		return 0;
	}
	Ziel=ppl7::GetArgv(argc,argv,"-z");
	Quelle=ppl7::GetArgv(argc,argv,"-q");
	Packetsize = ppl7::GetArgv(argc,argv,"-p").toInt();
	Laufzeit = ppl7::GetArgv(argc,argv,"-l").toInt();
	Timeout = ppl7::GetArgv(argc,argv,"-t").toInt();
	ThreadCount = ppl7::GetArgv(argc,argv,"-n").toInt();
	ppl7::String QueryRates = ppl7::GetArgv(argc,argv,"-r");
	Zeitscheibe = ppl7::GetArgv(argc,argv,"-i").toFloat();
	ppl7::String Filename = ppl7::GetArgv(argc,argv,"-c");
	ignoreResponses=ppl7::HaveArgv(argc,argv,"--ignore");
	if (ppl7::HaveArgv(argc,argv,"-b")) {
		SourceIpList.explode(ppl7::GetArgv(argc,argv,"-b"),",");
	}
	if (ppl7::HaveArgv(argc,argv,"--bl")) {
		readSourceIPList(ppl7::GetArgv(argc,argv,"--bl"));
	}
	if (ppl7::HaveArgv(argc,argv,"--ar")) {
		alwaysRandomize=true;
	}
	if (!ThreadCount) ThreadCount=1;
	if (!Packetsize) Packetsize=512;
	if (Packetsize<(int)sizeof(PACKET)) Packetsize=(int)sizeof(PACKET);
	if (!Laufzeit) Laufzeit=10;
	if (!Timeout) Timeout=5;
	if (Ziel.isEmpty()) {
		help();
		return 1;
	}
	if (Zeitscheibe==0.0f) Zeitscheibe=1.0f;
	/*
	if (Zeitscheibe>1000.0 || (1000%(int)Zeitscheibe)>0) {
		printf ("ERROR: Ungueltige Zeitscheibe\n");
		help();
		return 1;
	}
	*/
	ppl7::Array rates = getQueryRates(QueryRates);
	if (Filename.notEmpty()) {
		try {
			openCSVFile(Filename);
		} catch (const ppl7::Exception &e) {
			e.print();
			return 1;
		}
	}
	signal(SIGINT,sighandler);
	signal(SIGKILL,sighandler);

	UDPSender::Results results;
	try {
		prepareThreads();
		for (size_t i=0;i<rates.size();i++) {
			results.queryrate=rates[i].toInt();
			run(rates[i].toInt());
			getResults(results);
			presentResults(results);
			saveResultsToCsv(results);
		}
		threadpool.destroyAllThreads();
	} catch (ppl7::OperationInterruptedException &) {
		getResults(results);
		presentResults(results);
		saveResultsToCsv(results);
	} catch (const ppl7::Exception &e) {
		e.print();
		return 1;
	}
	return 0;
}


void UDPSender::readSourceIPList(const ppl7::String &filename)
{
	ppl7::File ff(filename);
	try {
		while (!ff.eof()) {
			ppl7::String ip=ff.gets().trimmed();
			if (ip.notEmpty()==true && ip[0]!='#')
				SourceIpList.add(ip);
		}
	} catch (ppl7::EndOfFileException &) {

	}
}

/*!\brief Workerthreads erstellen und konfigurieren
 *
 * Die gewünschte Anzahl Workerthreads werden erstellt, konfiguriert und in
 * den ThreadPool gestellt.
 */
void UDPSender::prepareThreads()
{
	size_t si=0;
	for (int i=0;i<ThreadCount;i++) {
		UDPEchoSenderThread *thread=new UDPEchoSenderThread();
		thread->setPacketsize(Packetsize);
		thread->setRuntime(Laufzeit);
		thread->setTimeout(Timeout);
		thread->setZeitscheibe(Zeitscheibe);
		thread->setIgnoreResponses(ignoreResponses);
		thread->setVerbose(false);
		thread->setAlwaysRandomize(alwaysRandomize);
		if (SourceIpList.size()>0) {
			thread->setSourceIP(SourceIpList[si]);
			si++;
			if (si>=SourceIpList.size()) si=0;
		}
		thread->connect(Ziel);
		threadpool.addThread(thread);
	}
}

/*!\brief CSV-File öffnen oder anlegen
 *
 * Öffnet eine Datei, in die das Testergebnis als Kommaseparierte Liste
 * geschrieben werden soll. Falls diese Datei noch nicht vorhanden war, wird
 * sie angelegt und ein Header mit der Beschreibung der Spalten hineingeschrieben.
 * @param Filename String mit dem Dateinamen
 * @exception Diverse Es können diverse Exceptions georfen werden, falls die Datei
 * nicht geöffnet doer beschrieben werden kann.
 */
void UDPSender::openCSVFile(const ppl7::String Filename)
{
	CSVFile.open(Filename,ppl7::File::APPEND);
	if (CSVFile.size()==0) {
		CSVFile.putsf ("#QPS Send; QPS Received; QPS Errors; Lostrate; "
					"rtt_avg; rtt_min; rtt_max;"
					"\n");
	}

	return;
	ppl7::DateTime now=ppl7::DateTime::currentTime();
	CSVFile.putsf ("# %s, Packetsize: %d, Threads: %d, Ziel: %s\n",
			(const char*) now.toString(),
			Packetsize,
			ThreadCount,
			(const char*)Ziel
			);
}

/*!\brief Last generieren
 *
 * Konfiguriert die Workerthreads mit der gewünschten Last \p queryrate, startet sie und wartet,
 * bis sie sich wieder beendet haben.
 * @param queryrate gewünschte Queryrate
 */
void UDPSender::run(int queryrate)
{

	printf ("# Start Session with Packetsize: %d, Threads: %d, Queryrate: %d\n",
			Packetsize, ThreadCount,queryrate);
	SystemStat stat_start;
	SystemStat stat_end;
	sampleSensorData(stat_start);
	double start = ppl7::GetMicrotime();
	double end = start + 1;

	UDPEchoCounter previous_counter;
	previous_counter.clear();

	ppl7::ThreadPool::iterator it;
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		((UDPEchoSenderThread*)(*it))->setQueryRate(queryrate/ThreadCount);
	}
	threadpool.startThreads();
	ppl7::MSleep(500);
	while (threadpool.running()==true && stopFlag==false) {
		ppl7::MSleep(100);
		if (ppl7::GetMicrotime() >= end) {
			sampleSensorData(stat_end);
			UDPEchoCounter counter=getCounter();

			printf("APP Packets TX: %8lu, RX: %8lu || NetIF TX: %8lu, RX: %8lu, ER: %8lu, DR: %8lu, MBit TX: %4lu, RX: %4lu || CPU: %0.2f\n",
					counter.packets_send-previous_counter.packets_send,
					counter.packets_received-previous_counter.packets_received,
					stat_end.net_total.transmit.packets-stat_start.net_total.transmit.packets,
					stat_end.net_total.receive.packets-stat_start.net_total.receive.packets,
					stat_end.net_total.receive.errs-stat_start.net_total.receive.errs,
					stat_end.net_total.receive.drop-stat_start.net_total.receive.drop,
					(stat_end.net_total.transmit.bytes-stat_start.net_total.transmit.bytes)>>17,
					(stat_end.net_total.receive.bytes-stat_start.net_total.receive.bytes)>>17,
					SystemStat::Cpu::getUsage(stat_end.cpu, stat_start.cpu)
					);

			stat_start=stat_end;
			previous_counter=counter;
			end += 1.0;
		}
	}
	if (stopFlag==true) {
		threadpool.stopThreads();
		throw ppl7::OperationInterruptedException("Lasttest wurde abgebrochen");
	}
}


UDPEchoCounter UDPSender::getCounter()
{
	UDPEchoCounter counter;
	counter.clear();
	counter.sampleTime=ppl7::GetMicrotime();
	ppl7::ThreadPool::iterator it;
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		counter.packets_send+=((UDPEchoSenderThread*)(*it))->getPacketsSend();
		counter.packets_received+=((UDPEchoSenderThread*)(*it))->getPacketsReceived();
		//counter.bytes_send+=((UDPEchoSenderThread*)(*it))->getBytesSend();
		//counter.bytes_received+=((UDPEchoSenderThread*)(*it))->getBytesReceived();
	}
	return counter;
}

/*!\brief Ergebnisse sammeln und berechnen
 *
 * Sammelt die Ergebnisse der Workerthreads und berechnet das Gesamtergebnis für einen
 * Testlauf.
 *
 * @param result Datenobjekt zur Aufnahme der Ergebniswerte
 */
void UDPSender::getResults(UDPSender::Results &result)
{
	ppl7::ThreadPool::iterator it;
	result.counter_send=0;
	result.counter_received=0;
	result.bytes_received=0;
	result.counter_errors=0;
	result.counter_0bytes=0;
	result.duration=0.0;
	result.rtt_total=0.0f;
	result.rtt_min=0.0f;
	result.rtt_max=0.0f;
	for (int i=0;i<255;i++) result.counter_errorcodes[i]=0;

	for (it=threadpool.begin();it!=threadpool.end();++it) {
		result.counter_send+=((UDPEchoSenderThread*)(*it))->getPacketsSend();
		result.counter_received+=((UDPEchoSenderThread*)(*it))->getPacketsReceived();
		result.bytes_received+=((UDPEchoSenderThread*)(*it))->getBytesReceived();
		result.counter_errors+=((UDPEchoSenderThread*)(*it))->getErrors();
		result.counter_0bytes+=((UDPEchoSenderThread*)(*it))->getCounter0Bytes();
		result.duration+=((UDPEchoSenderThread*)(*it))->getDuration();
		result.rtt_total+=((UDPEchoSenderThread*)(*it))->getRoundTripTimeAverage();
		double rtt=((UDPEchoSenderThread*)(*it))->getRoundTripTimeMin();
		if (result.rtt_min==0) result.rtt_min=rtt;
		else if (rtt<result.rtt_min) result.rtt_min=rtt;
		rtt=((UDPEchoSenderThread*)(*it))->getRoundTripTimeMax();
		if (rtt>result.rtt_max) result.rtt_max=rtt;
		for (int i=0;i<255;i++) result.counter_errorcodes[i]+=((UDPEchoSenderThread*)(*it))->getCounterErrorCode(i);
	}
	result.packages_lost=result.counter_send-result.counter_received;
	result.duration=result.duration/(double)ThreadCount;
}

/*!\brief Ergebnisse in eine Datei schreiben
 *
 * Prüft, ob eine CSV-Datei geöffnet ist und schreibt, wenn dies der Fall ist, die Werte
 * aus dem Ergebnisobjekt \p result als kommaseparierte Liste in die Datei.
 *
 * @param result Datenobjekt mit den Ergebniswerten
 */
void UDPSender::saveResultsToCsv(const UDPSender::Results &result)
{

	if (CSVFile.isOpen()) {
		CSVFile.putsf ("%lu;%lu;%lu;%0.3f;%0.4f;%0.4f;%0.4f;\n",
				(int64_t)((double)result.counter_send/result.duration),
				(int64_t)((double)result.counter_received/result.duration),
				(int64_t)((double)result.counter_errors/result.duration),
				(double)result.packages_lost*100.0/(double)result.counter_send,
				result.rtt_total*1000.0/(double)ThreadCount,
				result.rtt_min*1000.0,
				result.rtt_max*1000.0
		);
		CSVFile.flush();
	}
}

/*!\brief Ergebnisse auf der Konsole ausgeben
 *
 * Gibt die Werte aus dem Datenobjekt \p result auf der Konsole aus.
 *
 * @param result Datenobjekt mit den Ergebniswerten
 */
void UDPSender::presentResults(const UDPSender::Results &result)
{
	int64_t qps_send=(int64_t)((double)result.counter_send/result.duration);
	int64_t qps_received=(int64_t)((double)result.counter_received/result.duration);
	int64_t bytes_received=(int64_t)((double)result.bytes_received/result.duration);
	printf ("Packets send:     %10lu, Qps: %10lu, Durchsatz: %10lu MBit\n",result.counter_send,
			qps_send,
			qps_send*Packetsize*8/(1024*1024));
	printf ("Packets received: %10lu, Qps: %10lu, Durchsatz: %10lu MBit\n",result.counter_received,
			qps_received,
			bytes_received*8/(1024*1024));
	printf ("Packets lost:     %10lu = %0.3f %%\n",result.packages_lost,
			(double)result.packages_lost*100.0/(double)result.counter_send);

	printf ("Errors:           %10lu, Qps: %10lu\n",result.counter_errors,
			(int64_t)((double)result.counter_errors/result.duration));

	printf ("Errors 0Byte:     %10lu, Qps: %10lu\n",result.counter_0bytes,
			(int64_t)((double)result.counter_0bytes/result.duration));
	for (int i=0;i<255;i++) {
		if (result.counter_errorcodes[i]>0) {
			printf ("Errors %3d:       %10lu, Qps: %10lu [%s]\n",i, result.counter_errorcodes[i],
					(int64_t)((double)result.counter_errorcodes[i]/result.duration),
					strerror(i));

		}
	}

	printf ("rtt average: %0.4f ms\n"
			"rtt min:     %0.4f ms\n"
			"rtt max:     %0.4f ms\n",
			result.rtt_total*1000.0/(double)ThreadCount,
			result.rtt_min*1000.0,
			result.rtt_max*1000.0);
}


/*!\class UDPSender::Results
 * \brief Datenobjekt zur Aufnahme der Ergebnisse eines Lasttests
 */
