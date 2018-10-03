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

int oldmain(int argc, char **argv);


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
	return oldmain(argc, argv);
	DNSSender Sender;
	return Sender.main(argc,argv);
}


/*!\brief Hilfe anzeigen
 *
 * Zeigt die Hilfe für die Konsolenparameter an.
 */
void DNSSender::help()
{
	printf ("Usage:\n"
			"  -h            zeigt diese Hilfe an\n"
			"  -q HOST       Hostname oder IP der Quelle, sofern nicht gespooft\n"
			"                werden soll (siehe -s)\n"
			"  -z HOST:PORT  Hostname oder IP und Port des Zielservers\n"
			"  -d FILE       Datei mit Queries\n"
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
			"  -s NETWORK    Spoofe den Absender. Random-IP aus dem gegebenen Netz\n"
			"                (Beispiel: 192.168.0.0/16). Erfordert root-Rechte!\n"
			"\n");
}


DNSSender::DNSSender()
{
	ppl7::InitSockets();
	Laufzeit=10;
	Timeout=5;
	ThreadCount=1;
	Zeitscheibe=1.0f;
	ignoreResponses=false;
	validLinesInQueryFile=0;
}

/*!\brief Liste der zu testenden Queryrates erstellen
 *
 * In abhängigkeit des Wertes des Kommandozeilenparameters -r wird eine Liste
 * der zu testenden Queryraten erstellt.
 *
 * @param QueryRates String mit Wert des Kommandozeilenparameters -r
 * @return Liste mit den Queryraten
 */
ppl7::Array DNSSender::getQueryRates(const ppl7::String &QueryRates)
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
int DNSSender::main(int argc, char**argv)
{
	if (ppl7::HaveArgv(argc,argv,"-h") || ppl7::HaveArgv(argc,argv,"--help")) {
		help();
		return 0;
	}
	Ziel=ppl7::GetArgv(argc,argv,"-z");
	Quelle=ppl7::GetArgv(argc,argv,"-q");
	Laufzeit = ppl7::GetArgv(argc,argv,"-l").toInt();
	Timeout = ppl7::GetArgv(argc,argv,"-t").toInt();
	ThreadCount = ppl7::GetArgv(argc,argv,"-n").toInt();
	ppl7::String QueryRates = ppl7::GetArgv(argc,argv,"-r");
	Zeitscheibe = ppl7::GetArgv(argc,argv,"-i").toFloat();
	ppl7::String CVSFile = ppl7::GetArgv(argc,argv,"-c");
	ppl7::String QueryFilename = ppl7::GetArgv(argc,argv,"-d");
	ignoreResponses=ppl7::HaveArgv(argc,argv,"--ignore");
	if (!ThreadCount) ThreadCount=1;
	if (!Laufzeit) Laufzeit=10;
	if (!Timeout) Timeout=5;
	if (Ziel.isEmpty()) {
		help();
		return 1;
	}
	if (QueryFilename.isEmpty()) {
		help();
		return 1;
	} else {
		try {
			openQueryFile(QueryFilename);
		} catch (const ppl7::Exception &e) {
			e.print();
			return 1;
		}
	}

	if (Zeitscheibe==0.0f) Zeitscheibe=1.0f;
	ppl7::Array rates = getQueryRates(QueryRates);
	if (CVSFile.notEmpty()) {
		try {
			openCSVFile(CVSFile);
		} catch (const ppl7::Exception &e) {
			e.print();
			return 1;
		}
	}
	signal(SIGINT,sighandler);
	signal(SIGKILL,sighandler);

	DNSSender::Results results;
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
	} catch (const ppl7::OperationInterruptedException &) {
		getResults(results);
		presentResults(results);
		saveResultsToCsv(results);
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
void DNSSender::prepareThreads()
{
	size_t si=0;
	for (int i=0;i<ThreadCount;i++) {
		/*
		UDPEchoSenderThread *thread=new UDPEchoSenderThread();
		thread->setDestination(Ziel);
		thread->setPacketsize(Packetsize);
		thread->setRuntime(Laufzeit);
		thread->setTimeout(Timeout);
		thread->setZeitscheibe(Zeitscheibe);
		thread->setIgnoreResponses(ignoreResponses);
		thread->setVerbose(true);
		thread->setAlwaysRandomize(alwaysRandomize);
		if (SourceIpList.size()>0) {
			thread->setSourceIP(SourceIpList[si]);
			si++;
			if (si>=SourceIpList.size()) si=0;
		}
		threadpool.addThread(thread);
		*/
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
void DNSSender::openCSVFile(const ppl7::String &Filename)
{
	CSVFile.open(Filename,ppl7::File::APPEND);
	if (CSVFile.size()==0) {
		CSVFile.putsf ("#QPS Send; QPS Received; QPS Errors; Lostrate; "
				"rtt_avg; rtt_min; rtt_max;"
				"\n");
	}

	return;
	ppl7::DateTime now=ppl7::DateTime::currentTime();
	CSVFile.putsf ("# %s, Threads: %d, Ziel: %s\n",
			(const char*) now.toString(),
			ThreadCount,
			(const char*)Ziel
	);
}

/*!\brief Query-File öffnen
 *
 * Öffnet eine Datei, in der die zu sendenden Queries stehen. Diese
 * müssen folgendes Format aufweisen:
 * * jeder Zeile enthält eine Abfrage
 * * Query beginnt mit den abzufragenden Daten, gefolgt von 1 x Space, gefolgt
 * vom Record-Typ
 *
 * Beispiel: denic.de NS
 *
 * @param Filename String mit dem Dateinamen
 * @exception Diverse Es können diverse Exceptions georfen werden, falls die Datei
 * nicht geöffnet werden kann.
 */
void DNSSender::openQueryFile(const ppl7::String &Filename)
{
	if (Filename.isEmpty()) throw InvalidQueryFile("File not given");
	QueryFile.open(Filename,ppl7::File::READ);
	if (QueryFile.size()==0) {
		throw InvalidQueryFile("File is empty [%s]", (const char*)Filename);
	}
	validLinesInQueryFile=0;
}

/*!\brief Last generieren
 *
 * Konfiguriert die Workerthreads mit der gewünschten Last \p queryrate, startet sie und wartet,
 * bis sie sich wieder beendet haben.
 * @param queryrate gewünschte Queryrate
 */
void DNSSender::run(int queryrate)
{

	printf ("# Start Session with Threads: %d, Queryrate: %d\n",
			ThreadCount,queryrate);
	ppl7::ThreadPool::iterator it;
	/*
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		((DNSSenderThread*)(*it))->setQueryRate(queryrate/ThreadCount);
	}
	threadpool.startThreads();
	ppl7::MSleep(500);
	while (threadpool.running()==true && stopFlag==false) {
		ppl7::MSleep(100);
	}
	*/
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
void DNSSender::getResults(DNSSender::Results &result)
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

	/*
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
	*/
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
void DNSSender::saveResultsToCsv(const DNSSender::Results &result)
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
void DNSSender::presentResults(const DNSSender::Results &result)
{
	ppluint64 qps_send=(ppluint64)((double)result.counter_send/result.duration);
	ppluint64 qps_received=(ppluint64)((double)result.counter_received/result.duration);
	ppluint64 bytes_received=(ppluint64)((double)result.bytes_received/result.duration);
	// TODO: Wir müssen die Paketgröße irgendwie addieren!
	printf ("Packets send:     %10llu, Qps: %10llu, Durchsatz: %10llu MBit\n",result.counter_send,
			qps_send,
			qps_send*1*8/(1024*1024));
	printf ("Packets received: %10llu, Qps: %10llu, Durchsatz: %10llu MBit\n",result.counter_received,
			qps_received,
			bytes_received*8/(1024*1024));
	printf ("Packets lost:     %10llu = %0.3f %%\n",result.packages_lost,
			(double)result.packages_lost*100.0/(double)result.counter_send);

	printf ("Errors:           %10llu, Qps: %10llu\n",result.counter_errors,
			(ppluint64)((double)result.counter_errors/result.duration));

	printf ("Errors 0Byte:     %10llu, Qps: %10llu\n",result.counter_0bytes,
			(ppluint64)((double)result.counter_0bytes/result.duration));
	for (int i=0;i<255;i++) {
		if (result.counter_errorcodes[i]>0) {
			printf ("Errors %3d:       %10llu, Qps: %10llu [%s]\n",i, result.counter_errorcodes[i],
					(ppluint64)((double)result.counter_errorcodes[i]/result.duration),
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

void DNSSender::getQuery(ppl7::String &buffer)
{
	QueryMutex.lock();
	while (1) {
		try {
			QueryFile.gets(buffer,1024);
			buffer.trim();
			if (buffer.isEmpty()) continue;
			if (buffer.c_str()[0]=='#') continue;
			break;

		} catch (const ppl7::EndOfFileException &) {
			if (validLinesInQueryFile==0) {
				QueryMutex.unlock();
				throw InvalidQueryFile("No valid Queries found in Queryfile");
			}
			QueryFile.seek(0);
			validLinesInQueryFile=0;
		} catch (...) {
			QueryMutex.unlock();
			throw;
		}
	}
	validLinesInQueryFile++;
	QueryMutex.unlock();
}

/*!\class DNSSender::Results
 * \brief Datenobjekt zur Aufnahme der Ergebnisse eines Lasttests
 */

int oldmain(int argc, char **argv)
{
	RawSocket sock;
	sock.setDestination(ppl7::IPAddress("148.251.94.99"),53);
	ppl7::SockAddr sadr=sock.getSockAddr();
	printf ("%s:%d\n",(const char*)sadr.toIPAddress().toString(), sadr.port());

	return 0;

	int sock_r;
	sock_r=socket(AF_PACKET,SOCK_RAW,htons(0x0800));
	if(sock_r<0)
	{
		printf("error in socket\n");
		return -1;
	}
	res_init();

	// For unknown reason, res_mkquery is much slower (factor 3) when not
	// setting the following options:
	_res.options|=RES_USE_EDNS0;
	_res.options|=RES_USE_DNSSEC;

	unsigned char *buffer = (unsigned char *) malloc(65536); //to receive data
	memset(buffer,0,65536);
	struct sockaddr saddr;
	int saddr_len = sizeof (saddr);

	double start=ppl7::GetMicrotime();
	Packet pkt;
	pkt.setDestination("148.251.94.99",53);
	pkt.setSource("10.122.65.210",0x8000);
	//pkt.setSource("148.251.94.99",0x8000);
	pkt.setId(1234);

	for (int i=0;i<1000000;i++) {
		pkt.setSource("10.122.65.210",0x8000);
		pkt.setPayloadDNSQuery("pfp.de SOA");
		pkt.setId(1234);

	}

	double duration=ppl7::GetMicrotime()-start;

	printf ("duration: %0.3f\n",duration);
	return 0;


	try {
		RawSocket sock;
		sock.setDestination(ppl7::IPAddress("148.251.94.99"),53);

		Packet pkt;
		pkt.setDestination("148.251.94.99",53);
		pkt.setSource("10.122.65.210",0x8000);
		//pkt.setSource("148.251.94.99",0x8000);
		pkt.setId(1234);
		pkt.setPayloadDNSQuery("pfp.de SOA");

		ppl7::HexDump(pkt.ptr(),pkt.size());

		int bytes=sock.send(pkt);
		printf ("%d bytes gesendet\n",bytes);

		int buflen=recvfrom(sock_r,buffer,65536,0,&saddr,(socklen_t *)&saddr_len);
		if(buflen<0)
		{
			printf("error in reading recvfrom function\n");
			return -1;
		}
		printf ("Antwort: %d\n",saddr.sa_family);
		ppl7::HexDump(buffer,buflen);


		ppl7::HexDump(&saddr,saddr_len);
		return 0;

	} catch (const ppl7::Exception &exp) {
		exp.print();
		return 1;
	}
}
