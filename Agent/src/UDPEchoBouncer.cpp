#include <ppl7.h>
#include <ppl7-exceptions.h>
#include <ppl7-inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <queue>

#include "../include/udpecho.h"

/*!\class UDPEchoBouncer
 * \ingroup GroupBouncer
 * \brief Hauptmodul des Bouncers
 *
 */


/*!\brief Hilfe anzeigen
 *
 * Zeigt die Hilfe für die Konsolenparameter an.
 */
void UDPEchoBouncer::help()
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

/*!\brief Konstruktor
 *
 * Einige interne Variablen werden mit dem Default-Wert befüllt.
 */
UDPEchoBouncer::UDPEchoBouncer()
{
	noEcho=false;
	sockfd=0;
	packetSize=0;
	// Socket anlegen
	sockfd=::socket(AF_INET, SOCK_DGRAM, 0);
	if (!sockfd) throw ppl7::CouldNotOpenSocketException("Could not create Socket");
	const int trueValue = 1;
	// Wir erlauben anderen Threads/Programmen sich auf das gleichen Socket zu binden
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
#ifdef SO_REUSEPORT
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &trueValue, sizeof(trueValue));
#endif
}


UDPEchoBouncer::~UDPEchoBouncer()
{
	stop();
	if (sockfd) {
		::close(sockfd);
	}
}

/*!\brief Hostname auflösen und IP in Socket-Datenstruktur zurueckgeben
 *
 * Falls ein Hostname angegeben wurde, wird dieser aufgelöst. Die daraus resultierende
 * IP-Adresse wird zusammen mit dem Port in eine Socket-Datenstruktur geschrieben.
 *
 * @param Hostname Hostname oder IP-Adresse (v4 und v6 wird unterstützt)
 * @param Port Portnummer, auf den der Bouncer gebunden werden soll.
 * @return Socket-Datenstruktur
 */
ppl7::SockAddr UDPEchoBouncer::getSockAddr(const ppl7::String &Hostname, int Port)
{
	std::list<ppl7::IPAddress> result;
	size_t hostcount=ppl7::GetHostByName(Hostname, result);
	if (hostcount<1) {
		throw ppl7::HostNotFoundException("Hostname nicht aufloesbar [%s]",(const char*)Hostname);
	} else if (hostcount>1) {
		throw ppl7::ResolverException("ERROR: Hostname nicht eindeutig, er loest mit %td IP-Adressen auf\n",hostcount);
	}
	ppl7::IPAddress &address=result.front();
	address.sockaddr.setPort(Port);
	return address.sockaddr;
}

/*!\brief An einen Socket binden
 *
 * Der Bouncer wird an eine spezifische Adresse gebunden
 *
 * @param sockaddr Socket-Struktur mit der IP und dem Port, an den sich der Bouncer binden soll
 */
void UDPEchoBouncer::bind(const ppl7::SockAddr &sockaddr)
{
	memset(&servaddr,0,sizeof(servaddr));
	memcpy(&servaddr,sockaddr.addr(),sockaddr.size());
	// Socket an die IP-Adresse und den Port binden
	if (0 != ::bind(sockfd,(const struct sockaddr *)&servaddr, sizeof(servaddr))) {
		int e=errno;
		throw ppl7::CouldNotBindToInterfaceException("%s:%d, %s",
				(const char*)sockaddr.toString(),
				sockaddr.port(),
				strerror(e));
	}
	// Der Socket soll nicht blockieren, wenn keine Daten anstehen
	fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0)|O_NONBLOCK);
}


/*!\brief Worker-Threads erstellen und starten
 *
 * Erstellt die gewünschte Anzahl Worker-Threads, stellt sie in den Pool
 * UDPEchoBouncer::threadpool und startet sie.
 *
 * @param ThreadCount Anzahl Worker-Threads
 */
void UDPEchoBouncer::startBouncerThreads(size_t ThreadCount)
{
	for (size_t i = 0; i < ThreadCount; i++) {
		UDPEchoBouncerThread* thread = new UDPEchoBouncerThread();
		thread->setNoEcho(noEcho);
		thread->setSocketDescriptor(sockfd);
		thread->setPacketSize(packetSize);
		threadpool.addThread(thread);
	}
	threadpool.startThreads();
	//ppl7::MSleep(500);
}

/*!\brief Gibt solange sekündlich eine Statusmeldung aus, bis das Programm gestoppt wird
 *
 * Gibt solange sekündlich eine Statusmeldung aus, bis das Programm gestoppt wird.
 */
void UDPEchoBouncer::run()
{
	double start = ppl7::GetMicrotime();
	double end = start + 1;
	while (!threadShouldStop()) {
		ppl7::MSleep(100);
		if (ppl7::GetMicrotime() >= end) {
			UDPEchoBouncerThread::Counter counter;
			counter.count=0;
			counter.bytes=0;
			ppl7::ThreadPool::const_iterator it;
			for (it = threadpool.begin(); it != threadpool.end(); ++it) {
				UDPEchoBouncerThread::Counter c=((UDPEchoBouncerThread*) (*it))->getAndClearCounter();
				//printf ("   Thread %llu: %llu\n",(*it)->threadGetID(),c.count);
				counter.count += c.count;
				counter.bytes += c.bytes;
			}
			printf("Packets per second: %10llu, Durchsatz: %10llu Mbit\n", counter.count, counter.bytes*8/(1024*1024));
			end += 1.0;
		}

	}
}

void UDPEchoBouncer::setFixedResponsePacketSize(size_t size)
{
	if (size>0) {
		if (size<32 || size>4096) {
			throw ppl7::InvalidArgumentsException("UDPEchoBouncer::setFixedResponsePacketSize");
		}
	}
	packetSize=size;
}

void UDPEchoBouncer::setInterface(const ppl7::String &InterfaceName, int Port)
{
	ppl7::SockAddr sockaddr=getSockAddr(InterfaceName,Port);
	bind(sockaddr);
}

void UDPEchoBouncer::disableResponses(bool flag)
{
	noEcho=flag;
}

void UDPEchoBouncer::start(size_t num_threads)
{
	if (!num_threads) num_threads=1;
	startBouncerThreads(num_threads);
	this->threadStart();

}

void UDPEchoBouncer::stop()
{
	this->threadSignalStop();
	threadpool.destroyAllThreads();
	this->threadStop();
}


