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


void SystemStat::exportToArray(ppl7::AssocArray &data) const
{
	data.setf("sampleTime","%0.6f",sampleTime);
	data.setf("net_receive/bytes","%lu",net_receive.bytes);
	data.setf("net_receive/packets","%lu",net_receive.packets);
	data.setf("net_receive/errs","%lu",net_receive.errs);
	data.setf("net_receive/drop","%lu",net_receive.drop);
	data.setf("net_transmit/bytes","%lu",net_transmit.bytes);
	data.setf("net_transmit/packets","%lu",net_transmit.packets);
	data.setf("net_transmit/errs","%lu",net_transmit.errs);
	data.setf("net_transmit/drop","%lu",net_transmit.drop);

	data.setf("cpu/user","%d",cpu.user);
	data.setf("cpu/nice","%d",cpu.nice);
	data.setf("cpu/system","%d",cpu.system);
	data.setf("cpu/idle","%d",cpu.idle);
	data.setf("cpu/iowait","%d",cpu.iowait);

	data.setf("sysinfo/uptime","%ld",sysinfo.uptime);
	data.setf("sysinfo/freeswap","%ld",sysinfo.freeswap);
	data.setf("sysinfo/totalswap","%ld",sysinfo.totalswap);
	data.setf("sysinfo/freeram","%ld",sysinfo.freeram);
	data.setf("sysinfo/bufferram","%ld",sysinfo.bufferram);
	data.setf("sysinfo/totalram","%ld",sysinfo.totalram);
	data.setf("sysinfo/sharedram","%ld",sysinfo.sharedram);
	data.setf("sysinfo/procs","%d",sysinfo.procs);
}

void SystemStat::importFromArray(const ppl7::AssocArray &data)
{
	sampleTime=data.getString("sampleTime").toDouble();
	net_receive.bytes=data.getString("net_receive/bytes").toUnsignedLong();
	net_receive.packets=data.getString("net_receive/packets").toUnsignedLong();
	net_receive.errs=data.getString("net_receive/errs").toUnsignedLong();
	net_receive.drop=data.getString("net_receive/drop").toUnsignedLong();
	net_transmit.bytes=data.getString("net_transmit/bytes").toUnsignedLong();
	net_transmit.packets=data.getString("net_transmit/packets").toUnsignedLong();
	net_transmit.errs=data.getString("net_transmit/errs").toUnsignedLong();
	net_transmit.drop=data.getString("net_transmit/drop").toUnsignedLong();

	cpu.user=data.getString("cpu/user").toInt();
	cpu.nice=data.getString("cpu/nice").toInt();
	cpu.system=data.getString("cpu/system").toInt();
	cpu.idle=data.getString("cpu/idle").toInt();
	cpu.iowait=data.getString("cpu/iowait").toInt();

	sysinfo.uptime=data.getString("sysinfo/uptime").toLong();
	sysinfo.freeswap=data.getString("sysinfo/freeswap").toLong();
	sysinfo.totalswap=data.getString("sysinfo/totalswap").toLong();
	sysinfo.freeram=data.getString("sysinfo/freeram").toLong();
	sysinfo.bufferram=data.getString("sysinfo/bufferram").toLong();
	sysinfo.totalram=data.getString("sysinfo/totalram").toLong();
	sysinfo.sharedram=data.getString("sysinfo/sharedram").toLong();
	sysinfo.procs=data.getString("sysinfo/procs").toInt();
}

void SystemStat::print() const
{
	ppl7::AssocArray a;
	exportToArray(a);
	a.list();
}


