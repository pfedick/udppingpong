#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <list>


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

unsigned short getQueryTimestamp()
{
	struct timeval tp;
	if (gettimeofday(&tp,NULL)==0) {
		return (tp.tv_sec%60)*1000+(tp.tv_usec/1000);
	}
	return 0;
}

unsigned short getQueryRTT(unsigned short start)
{
	unsigned short now=getQueryTimestamp();
	if (now<start) return 60000-start+now;
	return now-start;
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
	//return oldmain(argc, argv);
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
			"  -h            Zeigt diese Hilfe an\n"
			"  -q HOST       Hostname oder IP der Quelle, sofern nicht gespooft\n"
			"                werden soll (siehe -s)\n"
			"  -s NETWORK    Spoofe den Absender. Random-IP aus dem gegebenen Netz\n"
			"                (Beispiel: 192.168.0.0/16). Erfordert root-Rechte!\n"
			"  -z HOST:PORT  Hostname oder IP und Port des Zielservers\n"
			"  -p FILE       Datei mit Queries/Payload\n"
			"  -l #          Laufzeit in Sekunden (Default=10 Sekunden)\n"
			"  -t #          Timeout in Sekunden (Default=5 Sekunden)\n"
			"  -n #          Anzahl Worker-Threads (Default=1)\n"
			"  -r #          Queryrate (Default=soviel wie geht)\n"
			"                Kann auch eine Kommaseparierte Liste sein (rate,rate,...) oder eine\n"
			"                Range (von rate - bis rate, Schrittweite)\n"
			"  -d #          DNSSEC-Anteil in Prozent von 0-100 (Default=0)\n"
			"  -i #          Länge einer Zeitscheibe in Millisekunden (nur in Kombination mit -r, Default=1)\n"
			"                Wert muss zwischen 1 und 1000 liegen und \"Wert/1000\" muss aufgehen\n"
			"  -c FILE       CSV-File fuer Ergebnisse\n"
			"  --ignore      Ignoriere die Antworten\n"
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
	DnssecRate=0;
	TargetPort=53;
	spoofingEnabled=false;
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

void DNSSender::getTarget(int argc, char**argv)
{
	if (!ppl7::HaveArgv(argc,argv,"-z")) {
		throw MissingCommandlineParameter("Ziel-IP/Hostname oder Port nicht angegeben (-z IP:PORT)");
	}
	ppl7::String Tmp=ppl7::GetArgv(argc,argv,"-z");
	ppl7::Array Tok(Tmp,":");
	if (Tok.size()!=2) {
		if (Tok.size()!=1) throw InvalidCommandlineParameter("-z IP:PORT");
		TargetPort=53;
	} else {
		TargetPort=Tok[1].toInt();
	}
	if (TargetPort<1 || TargetPort>65535) throw InvalidCommandlineParameter("-z IP:PORT, Invalid Port");
	std::list<ppl7::IPAddress> Result;
	size_t num=ppl7::GetHostByName(Tok[0], Result,ppl7::af_inet);
	if (!num) throw InvalidCommandlineParameter("-z IP:PORT, Invalid IP or could not resolve Hostname");
	TargetIP=Result.front();
	//printf ("num=%d, %s\n",num, (const char*)TargetIP.toString());
}

void DNSSender::getSource(int argc, char**argv)
{
	if (ppl7::HaveArgv(argc,argv,"-s")) {
		SourceNet.set(ppl7::GetArgv(argc,argv,"-s"));
		if (SourceNet.family()!=ppl7::IPAddress::IPv4) throw UnsupportedIPFamily("only IPv4 works");
		spoofingEnabled=true;
	} else {
		ppl7::String Tmp=ppl7::GetArgv(argc,argv,"-q");
		std::list<ppl7::IPAddress> Result;
		size_t num=ppl7::GetHostByName(Tmp, Result,ppl7::af_inet);
		if (!num) throw InvalidCommandlineParameter("-q HOST, Invalid IP or could not resolve Hostname");
		SourceIP=Result.front();
		if (SourceIP.family()!=ppl7::IPAddress::IPv4) throw UnsupportedIPFamily("only IPv4 works");
		spoofingEnabled=false;
	}
}

int DNSSender::getParameter(int argc, char**argv)
{
	if (ppl7::HaveArgv(argc,argv,"-q") && ppl7::HaveArgv(argc,argv,"-s")) {
		printf ("ERROR: Parameter koennen nicht gleichzeitig verwendet werden: -q -s\n\n");
		help();
		return 1;
	}
	if ((!ppl7::HaveArgv(argc,argv,"-q")) && (!ppl7::HaveArgv(argc,argv,"-s"))) {
		printf ("ERROR: Quell IP/Host oder gespooftes Netz muss angegeben werden (-q IP | -s NETZ)\n\n");
		help();
		return 1;
	}

	try {
		getTarget(argc, argv);
		getSource(argc, argv);
	} catch (const ppl7::Exception &e) {
		printf ("ERROR: Feghlender oder fehlerhafter Parameter\n");
		e.print();
		printf ("\n");
		help();
		return 1;
	}

	Laufzeit = ppl7::GetArgv(argc,argv,"-l").toInt();
	Timeout = ppl7::GetArgv(argc,argv,"-t").toInt();
	ThreadCount = ppl7::GetArgv(argc,argv,"-n").toInt();
	ppl7::String QueryRates = ppl7::GetArgv(argc,argv,"-r");
	Zeitscheibe = ppl7::GetArgv(argc,argv,"-i").toFloat();
	CSVFileName = ppl7::GetArgv(argc,argv,"-c");
	QueryFilename = ppl7::GetArgv(argc,argv,"-p");
	ignoreResponses=ppl7::HaveArgv(argc,argv,"--ignore");
	if (ppl7::HaveArgv(argc,argv,"-d")) {
		DnssecRate=ppl7::GetArgv(argc,argv,"-d").toInt();
		if (DnssecRate<0 || DnssecRate>100) {
			printf ("ERROR: DNSSEC-Rate muss zwischen 0 und 100 liegen (-d #)\n\n");
			help();
			return 1;
		}
	}
	if (!ThreadCount) ThreadCount=1;
	if (!Laufzeit) Laufzeit=10;
	if (!Timeout) Timeout=5;
	if (QueryFilename.isEmpty()) {
		printf ("ERROR: Payload-File ist nicht angegeben (-p FILENAME)\n\n");
		help();
		return 1;
	}
	if (Zeitscheibe==0.0f) Zeitscheibe=1.0f;
	rates = getQueryRates(QueryRates);
	return 0;
}


int DNSSender::openFiles()
{
	if (CSVFileName.notEmpty()) {
		try {
			openCSVFile(CSVFileName);
		} catch (const ppl7::Exception &e) {
			printf ("ERROR: CSV-File konnte nicht geoeffnet werden\n");
			e.print();
			return 1;
		}
	}
	try {
		payload.openQueryFile(QueryFilename);
	} catch (const ppl7::Exception &e) {
		printf ("ERROR: Payload-File konnte nicht geoeffnet werden oder enthaelt keine Queries\n");
		e.print();
		return 1;
	}
	return 0;
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
	if (getParameter(argc,argv)!=0) return 1;
	if (openFiles()!=0) return 1;

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
	for (int i=0;i<ThreadCount;i++) {
		DNSSenderThread *thread=new DNSSenderThread();
		thread->setDestination(TargetIP,TargetPort);
		thread->setRuntime(Laufzeit);
		thread->setTimeout(Timeout);
		thread->setZeitscheibe(Zeitscheibe);
		thread->setDNSSECRate(DnssecRate);
		thread->setVerbose(true);
		thread->setPayload(payload);
		if (spoofingEnabled) {
			thread->setSourceNet(SourceNet);
		} else {
			thread->setSourceIP(SourceIP);
		}
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
void DNSSender::openCSVFile(const ppl7::String &Filename)
{
	CSVFile.open(Filename,ppl7::File::APPEND);
	if (CSVFile.size()==0) {
		CSVFile.putsf ("#QPS Send; QPS Received; QPS Errors; Lostrate; "
				"rtt_avg; rtt_min; rtt_max;"
				"\n");
	}
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
	for (it=threadpool.begin();it!=threadpool.end();++it) {
		((DNSSenderThread*)(*it))->setQueryRate(queryrate/ThreadCount);
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
void DNSSender::getResults(DNSSender::Results &result)
{
	ppl7::ThreadPool::iterator it;
	result.counter_send=0;
	result.counter_received=0;
	result.bytes_send=0;
	result.bytes_received=0;
	result.counter_errors=0;
	result.counter_0bytes=0;
	result.duration=0.0;
	result.rtt_total=0.0f;
	result.rtt_min=0.0f;
	result.rtt_max=0.0f;
	for (int i=0;i<255;i++) result.counter_errorcodes[i]=0;

	for (it=threadpool.begin();it!=threadpool.end();++it) {
		result.counter_send+=((DNSSenderThread*)(*it))->getPacketsSend();
		result.bytes_send+=((DNSSenderThread*)(*it))->getBytesSend();
		//result.counter_received+=((DNSSenderThread*)(*it))->getPacketsReceived();
		//result.bytes_received+=((DNSSenderThread*)(*it))->getBytesReceived();
		result.counter_errors+=((DNSSenderThread*)(*it))->getErrors();
		result.counter_0bytes+=((DNSSenderThread*)(*it))->getCounter0Bytes();
		//result.duration+=((DNSSenderThread*)(*it))->getDuration();
		//result.rtt_total+=((DNSSenderThread*)(*it))->getRoundTripTimeAverage();
		/*
		double rtt=((DNSSenderThread*)(*it))->getRoundTripTimeMin();
		if (result.rtt_min==0) result.rtt_min=rtt;
		else if (rtt<result.rtt_min) result.rtt_min=rtt;
		rtt=((DNSSenderThread*)(*it))->getRoundTripTimeMax();
		if (rtt>result.rtt_max) result.rtt_max=rtt;
		*/
		for (int i=0;i<255;i++) result.counter_errorcodes[i]+=((DNSSenderThread*)(*it))->getCounterErrorCode(i);
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
	ppluint64 bps_send=(ppluint64)((double)result.bytes_send/result.duration);
	ppluint64 qps_received=(ppluint64)((double)result.counter_received/result.duration);
	ppluint64 bytes_received=(ppluint64)((double)result.bytes_received/result.duration);
	printf ("Bytes send:       %10llu, Durchsatz: %10llu MBit\n",result.bytes_send,
			bps_send/(1024*1024));

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

/*!\class DNSSender::Results
 * \brief Datenobjekt zur Aufnahme der Ergebnisse eines Lasttests
 */

int oldmain(int argc, char **argv)
{
	DNSSender Sender;

	return 0;
}

#ifdef OLD

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
	pkt.setUdpId(1234);

	for (int i=0;i<1000000;i++) {
		pkt.setSource("10.122.65.210",0x8000);
		pkt.setPayloadDNSQuery("pfp.de SOA");
		pkt.setUdpId(1234);

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
		pkt.setUdpId(1234);
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

#endif