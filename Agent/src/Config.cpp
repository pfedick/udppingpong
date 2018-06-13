#include <ppl7.h>

#include "../include/dnsperftest_agent.h"

Config::Config()
{
	InterfacePort=443;
}

void Config::loadFromFile(const std::list<ppl7::String> &FileList)
{
	std::list<ppl7::String>::const_iterator it;
	for (it=FileList.begin();it!=FileList.end();++it) {
		if (ppl7::File::exists((*it))) {
			loadFromFile((*it));
			return;
		}
	}
	throw NoConfigurationFileFound();
}

void Config::loadFromFile(const ppl7::String &Filename)
{
	ppl7::ConfigParser parser(Filename);
	parser.selectSection("interface");
	InterfaceName=parser.get("Host");
	InterfacePort=parser.getInt("Port",443);
	/*
	parser.selectSection("udpecho");
	UDPEchoInterfaceName=parser.get("Host","");
	UDPEchoInterfacePort=parser.getInt("Port",0);
	*/
}

