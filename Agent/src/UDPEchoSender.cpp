#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "../include/udpecho.h"
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
	UDPEchoSender Sender;
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
void UDPEchoSender::help()
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
			"\n");
			//"  -m Messe Laufzeiten (Default=keine Zeitmessung)\n"
}




/*!\brief Konstruktor
 *
 * Einige interne Variablen werden mit dem Default-Wert befüllt.
 */
UDPEchoSender::UDPEchoSender()
{
	Packetsize=512;
	Laufzeit=10;
	Timeout=5;
	ThreadCount=1;
	Zeitscheibe=1;
	ignoreResponses=false;
}

/*!\brief Liste der zu testenden Queryrates erstellen
 *
 * In abhängigkeit des Wertes des Kommandozeilenparameters -r wird eine Liste
 * der zu testenden Queryraten erstellt.
 *
 * @param QueryRates String mit Wert des Kommandozeilenparameters -r
 * @return Liste mit den Queryraten
 */
ppl7::Array UDPEchoSender::getQueryRates(const ppl7::String &QueryRates)
{
	ppl7::Array rates;
	if (QueryRates.isEmpty()) {
		rates.add("0");
	} else {
		ppl7::Array matches;
		if (QueryRates.pregMatch("/^([0-9]+)-([0-9]+),([0-9]+)$", matches)) {
			for (ppluint64 i = matches[1].toUnsignedInt64(); i <= matches[2].toUnsignedInt64(); i += matches[3].toUnsignedInt64()) {
				rates.addf("%llu", i);
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
int UDPEchoSender::main(int argc, char**argv)
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
	Zeitscheibe = ppl7::GetArgv(argc,argv,"-i").toInt();
	ppl7::String Filename = ppl7::GetArgv(argc,argv,"-c");
	ignoreResponses=ppl7::HaveArgv(argc,argv,"--ignore");
	if (!ThreadCount) ThreadCount=1;
	if (!Packetsize) Packetsize=512;
	if (Packetsize<(int)sizeof(PACKET)) Packetsize=(int)sizeof(PACKET);
	if (!Laufzeit) Laufzeit=10;
	if (!Timeout) Timeout=5;
	if (Ziel.isEmpty()) {
		help();
		return 1;
	}
	if (Zeitscheibe<1) Zeitscheibe=1;
	if (Zeitscheibe>1000 || (1000%Zeitscheibe)>0) {
		printf ("ERROR: Ungueltige Zeitscheibe\n");
		help();
		return 1;
	}
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

	try {
		prepareThreads();
		for (size_t i=0;i<rates.size();i++) {
			run(rates[i].toInt());
			UDPEchoSender::Results results;
			results.queryrate=rates[i].toInt();
			getResults(results);
			presentResults(results);
			saveResultsToCsv(results);
		}
		threadpool.destroyAllThreads();
	} catch (const ppl7::Exception &e) {
		e.print();
		return 1;
	}
	return 0;
}

/*!\brief Workerthreads erstellen und konfigurieren
 *
 * Die gewünschte Anzahl Workerthreads werden erstellt, konfiguriert und in
 * den ThreadPool gestellt.
 */
void UDPEchoSender::prepareThreads()
{
	for (int i=0;i<ThreadCount;i++) {
		UDPEchoSenderThread *thread=new UDPEchoSenderThread();
		thread->setDestination(Ziel);
		thread->setPacketsize(Packetsize);
		thread->setRuntime(Laufzeit);
		thread->setTimeout(Timeout);
		thread->setZeitscheibe(Zeitscheibe);
		thread->setIgnoreResponses(ignoreResponses);
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
void UDPEchoSender::openCSVFile(const ppl7::String Filename)
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
void UDPEchoSender::run(int queryrate)
{

	printf ("# Start Session with Packetsize: %d, Threads: %d, Queryrate: %d\n",
			Packetsize, ThreadCount,queryrate);
	ppl7::ThreadPool::iterator it;
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		((UDPEchoSenderThread*)(*it))->setQueryRate(queryrate/ThreadCount);
	}
	threadpool.startThreads();
	ppl7::MSleep(500);
	while (threadpool.running()==true && stopFlag==false) {
		ppl7::MSleep(100);
	}
	if (stopFlag==true) {
		threadpool.stopThreads();
		throw ppl7::OperationInterruptedException("Lasttest wurde abgebrochen");
	}
}

/*!\brief Ergebnisse sammeln und berechnen
 *
 * Sammelt die Ergebnisse der Workerthreads und berechnet das Gesamtergebnis für einen
 * Testlauf.
 *
 * @param result Datenobjekt zur Aufnahme der Ergebniswerte
 */
void UDPEchoSender::getResults(UDPEchoSender::Results &result)
{
	ppl7::ThreadPool::iterator it;
	result.counter_send=0;
	result.counter_received=0;
	result.bytes_received=0;
	result.counter_errors=0;
	result.duration=0.0;
	result.rtt_total=0.0f;
	result.rtt_min=0.0f;
	result.rtt_max=0.0f;
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		result.counter_send+=((UDPEchoSenderThread*)(*it))->getPacketsSend();
		result.counter_received+=((UDPEchoSenderThread*)(*it))->getPacketsReceived();
		result.bytes_received+=((UDPEchoSenderThread*)(*it))->getBytesReceived();
		result.counter_errors+=((UDPEchoSenderThread*)(*it))->getErrors();
		result.duration+=((UDPEchoSenderThread*)(*it))->getDuration();
		result.rtt_total+=((UDPEchoSenderThread*)(*it))->getRoundTripTimeAverage();
		double rtt=((UDPEchoSenderThread*)(*it))->getRoundTripTimeMin();
		if (result.rtt_min==0) result.rtt_min=rtt;
		else if (rtt<result.rtt_min) result.rtt_min=rtt;
		rtt=((UDPEchoSenderThread*)(*it))->getRoundTripTimeMax();
		if (rtt>result.rtt_max) result.rtt_max=rtt;
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
void UDPEchoSender::saveResultsToCsv(const UDPEchoSender::Results &result)
{

	if (CSVFile.isOpen()) {
		CSVFile.putsf ("%llu;%llu;%llu;%0.3f;%0.4f;%0.4f;%0.4f;\n",
				(ppluint64)((double)result.counter_send/result.duration),
				(ppluint64)((double)result.counter_received/result.duration),
				(ppluint64)((double)result.counter_errors/result.duration),
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
void UDPEchoSender::presentResults(const UDPEchoSender::Results &result)
{
	ppluint64 qps_send=(ppluint64)((double)result.counter_send/result.duration);
	ppluint64 qps_received=(ppluint64)((double)result.counter_received/result.duration);
	ppluint64 bytes_received=(ppluint64)((double)result.bytes_received/result.duration);
	printf ("Packets send:     %10llu, Qps: %10llu, Durchsatz: %10llu MBit\n",result.counter_send,
			qps_send,
			qps_send*Packetsize*8/(1024*1024));
	printf ("Packets received: %10llu, Qps: %10llu, Durchsatz: %10llu MBit\n",result.counter_received,
			qps_received,
			bytes_received*8/(1024*1024));
	printf ("Packets lost:     %10llu = %0.3f %%\n",result.packages_lost,
			(double)result.packages_lost*100.0/(double)result.counter_send);

	printf ("Errors:           %10llu, Qps: %10llu\n",result.counter_errors,
			(ppluint64)((double)result.counter_errors/result.duration));

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
