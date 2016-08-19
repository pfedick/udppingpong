#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <dnsperftest_communicator.h>


static void help()
{

}

int main(int argc, char**argv)
{
	Communicator comm;
	comm.connect("patrickf-xm2.office.denic.de",40000);
	comm.ping();

	return 0;
}
