#include <ppl7.h>
#include "sensordaemon.h"

Config::Config()
{
	InterfacePort=0;
	UDPEchoInterfacePort=0;
}

void Config::loadFromFile(const ppl7::String &Filename)
{
	ppl7::ConfigParser parser(Filename);
	parser.selectSection("interface");
	InterfaceName=parser.get("Host");
	InterfacePort=parser.getInt("Port");
	parser.selectSection("interface");
	UDPEchoInterfaceName=parser.get("Host","");
	UDPEchoInterfacePort=parser.getInt("Port",0);
}

