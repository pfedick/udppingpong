#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>

#include "dnsperftest_sensor.h"

static void sampleCpuUsage(SystemStat::Cpu &stat)
{
	FILE *fp = fopen("/proc/stat","r");
    if (5 != fscanf(fp,"%*s %d %d %d %d %d",&stat.user, &stat.nice, &stat.system, &stat.idle, &stat.iowait)) {

    	fclose(fp);
    }
    fclose(fp);
}

static void sampleSysinfo(SystemStat::Sysinfo &stat)
{
    struct sysinfo info;
    if (0 != sysinfo(&info)) {

    }
    stat.uptime=info.uptime;
    stat.freeswap=info.freeswap*info.mem_unit;
    stat.freeram=info.freeram*info.mem_unit;
    stat.bufferram=info.bufferram*info.mem_unit;
    stat.totalram=info.totalram*info.mem_unit;
    stat.totalswap=info.totalswap*info.mem_unit;
    stat.sharedram=info.sharedram*info.mem_unit;
}

static void sampleNetwork(SystemStat::Network &receive, SystemStat::Network &transmit)
{
	receive.clear();
	transmit.clear();

	ppl7::String buffer;
	ppl7::File ff("/proc/net/dev");
	while (!ff.eof()) {
		ff.gets(buffer,2048);
		buffer.trim();
		if (buffer.left(3)=="eth") {
			buffer.replace("\t"," ");
			ppl7::Array tok=ppl7::StrTok(buffer," ");
			receive.bytes+=tok[1].toUnsignedLong();
			receive.packets+=tok[2].toUnsignedLong();
			receive.errs+=tok[3].toUnsignedLong();
			receive.drop+=tok[4].toUnsignedLong();
			transmit.bytes+=tok[9].toUnsignedLong();
			transmit.packets+=tok[10].toUnsignedLong();
			transmit.errs+=tok[11].toUnsignedLong();
			transmit.drop+=tok[12].toUnsignedLong();
		}
	}
}

void sampleSensorData(SystemStat &stat)
{
	stat.sampleTime=ppl7::GetMicrotime();
	sampleCpuUsage(stat.cpu);
	sampleSysinfo(stat.sysinfo);
	sampleNetwork(stat.net_receive, stat.net_transmit);
}

double SystemStat::Cpu::getUsage(const SystemStat::Cpu &sample1,const SystemStat::Cpu &sample2)
{
	return 100.0 * (double)((sample2.user+sample2.nice+sample2.system) -
			(sample1.user+sample1.nice+sample1.system)) /
			(double)((sample2.user+sample2.nice+sample2.system+sample2.idle) -
					(sample1.user+sample1.nice+sample1.system+sample1.idle));

}

static unsigned long delta_with_overflow(unsigned long sample1, unsigned long sample2)
{
	if (sample2>=sample1) return sample2-sample1;
	return ULONG_MAX - sample1 + sample2;

}

SystemStat::Network SystemStat::Network::getDelta(const SystemStat::Network &sample1,const SystemStat::Network &sample2)
{
	return SystemStat::Network(delta_with_overflow(sample1.bytes, sample2.bytes),
			delta_with_overflow(sample1.packets, sample2.packets),
			delta_with_overflow(sample1.errs, sample2.errs),
			delta_with_overflow(sample1.drop, sample2.drop));
}
