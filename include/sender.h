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

#ifndef SENDER_H_
#define SENDER_H_

#include <ppl7.h>
#include <ppl7-inet.h>
#include "udpecho.h"


class UDPSender
{
	private:

		class Results
		{
			public:
				int			queryrate;
				ppluint64	counter_send;
				ppluint64	counter_received;
				ppluint64	bytes_received;
				ppluint64	counter_errors;
				ppluint64	packages_lost;
				ppluint64   counter_0bytes;
				ppluint64   counter_errorcodes[255];
				double		duration;
				double		rtt_total;
				double		rtt_min;
				double		rtt_max;
		};
		ppl7::ThreadPool threadpool;
		ppl7::String Ziel;
		ppl7::String Quelle;
		ppl7::File CSVFile;
		ppl7::Array SourceIpList;
		int Packetsize;
		int Laufzeit;
		int Timeout;
		int ThreadCount;
		float Zeitscheibe;
		bool ignoreResponses;
		bool alwaysRandomize;

		void openCSVFile(const ppl7::String Filename);
		void run(int queryrate);
		void presentResults(const UDPSender::Results &result);
		void saveResultsToCsv(const UDPSender::Results &result);
		void prepareThreads();
		void getResults(UDPSender::Results &result);
		ppl7::Array getQueryRates(const ppl7::String &QueryRates);
		void readSourceIPList(const ppl7::String &filename);

	public:
		UDPSender();
		void help();
		int main(int argc, char**argv);
};




#endif /* SENDER_H_ */
