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



PayloadFile::PayloadFile()
{
	validLinesInQueryFile=0;
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
void PayloadFile::openQueryFile(const ppl7::String &Filename)
{
	if (Filename.isEmpty()) throw InvalidQueryFile("File not given");
	QueryFile.open(Filename,ppl7::File::READ);
	if (QueryFile.size()==0) {
		throw InvalidQueryFile("File is empty [%s]", (const char*)Filename);
	}
	validLinesInQueryFile=0;
	// Test if we have valid queries in this file
	ppl7::String buffer;
	getQuery(buffer);
	QueryFile.seek(0);
	validLinesInQueryFile=0;
}


void PayloadFile::getQuery(ppl7::String &buffer)
{
	QueryMutex.lock();
	while (1) {
		try {
			if (QueryFile.eof()) throw ppl7::EndOfFileException();
			QueryFile.gets(buffer,1024);
			buffer.trim();
			if (buffer.isEmpty()) continue;
			if (buffer.c_str()[0]=='#') continue;
			break;

		} catch (const ppl7::EndOfFileException &) {
			//printf ("Valid lines: %d\n",validLinesInQueryFile);
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

