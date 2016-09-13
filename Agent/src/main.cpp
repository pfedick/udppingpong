#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "../include/dnsperftest_agent.h"


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
	AgentDaemon Daemon;
	return Daemon.main(argc,argv);
}
