#include <ppl7.h>
#include <ppl7-inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __FreeBSD__
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <paths.h>
#include <fcntl.h>
#include <kvm.h>
#include <ifaddrs.h>
#include <net/if.h>

#elif defined __linux__
#include <sys/sysinfo.h>
#endif

#include <limits.h>

#include "dnsperftest_sensor.h"

// ########################################################### Linux specific ####################################
#ifdef __linux__
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
        return;
    }
    stat.uptime=info.uptime;
    stat.freeswap=info.freeswap*info.mem_unit;
    stat.freeram=info.freeram*info.mem_unit;
    stat.bufferram=info.bufferram*info.mem_unit;
    stat.totalram=info.totalram*info.mem_unit;
    stat.totalswap=info.totalswap*info.mem_unit;
    stat.sharedram=info.sharedram*info.mem_unit;
    stat.procs=info.procs;
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

#elif defined __FreeBSD__
static kvm_t *kd=NULL;
#define GETSYSCTL(name, var) getsysctl(name, &(var), sizeof(var))

static void
getsysctl(const char *name, void *ptr, size_t len)
{
        size_t nlen = len;
        if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
            throw SystemCallFailed("sysctlbyname(%s...) failed: %s\n", name,
                    strerror(errno));
        }
        if (nlen != len) {
            throw SystemCallFailed("sysctlbyname(%s...) expected %lu, got %lu\n",
                    name, (unsigned long)len, (unsigned long)nlen);
        }
}


static void sampleCpuUsage(SystemStat::Cpu &stat)
{
    size_t cp_size = sizeof(long) * CPUSTATES*8;
    long *cp_times = (long*)malloc(cp_size);
    if (sysctlbyname("kern.cp_time", cp_times, &cp_size, NULL, 0) < 0) {
       perror("sysctlbyname");
       free(cp_times);
    }
    stat.user=(int)cp_times[0];
    stat.nice=(int)cp_times[1];
    stat.system=(int)cp_times[2];
    stat.iowait=(int)cp_times[3];
    stat.idle=(int)cp_times[4];


}

static int
swapmode(long *retavail, long *retfree)
{
        int n;
        int pagesize = getpagesize();
        struct kvm_swap swapary[1];

        *retavail = 0;
        *retfree = 0;

#define CONVERT(v)      ((quad_t)(v) * pagesize )

        n = kvm_getswapinfo(kd, swapary, 1, 0);
        if (n < 0 || swapary[0].ksw_total == 0)
                return (0);

        *retavail = CONVERT(swapary[0].ksw_total);
        *retfree = CONVERT(swapary[0].ksw_total - swapary[0].ksw_used);

        n = (int)(swapary[0].ksw_used * 100.0 / swapary[0].ksw_total);
        return (n);
}


static void sampleSysinfo(SystemStat::Sysinfo &stat)
{
    struct timespec uptime;
    int pagesize = getpagesize();
    if (clock_gettime(CLOCK_UPTIME, &uptime) == 0)
        stat.uptime=uptime.tv_sec;
    swapmode(&stat.totalswap, &stat.freeswap);
    int tmp;
    long tmp_l;
    GETSYSCTL("vm.stats.vm.v_free_count", tmp);
    stat.freeram=tmp*pagesize;
    GETSYSCTL("hw.physmem", tmp_l);
    stat.totalram=tmp_l;

}

static void sampleNetwork(SystemStat::Network &receive, SystemStat::Network &transmit)
{
   receive.clear();
   transmit.clear();
#define IFA_STAT(s)     (((struct if_data *)ifa->ifa_data)->ifi_ ## s)

   struct ifaddrs *ifap=NULL;
   if (getifaddrs(&ifap)!=0) {
       throw SystemCallFailed("FreeBSD, getifaddrs: %s", strerror(errno));
   }
   for (struct ifaddrs *ifa=ifap; ifa; ifa = ifa->ifa_next) {
       if (ifa->ifa_addr->sa_family != AF_LINK) continue;
       receive.bytes+=IFA_STAT(ibytes);
       receive.packets+=IFA_STAT(ipackets);
       receive.errs+=IFA_STAT(ierrors);
       receive.drop+=IFA_STAT(iqdrops);
       transmit.bytes+=IFA_STAT(obytes);
       transmit.packets+=IFA_STAT(opackets);
       transmit.errs+=IFA_STAT(oerrors);
       transmit.drop+=IFA_STAT(oqdrops);
   }
   freeifaddrs(ifap);
}

#endif

void sampleSensorData(SystemStat &stat)
{
#ifdef __FreeBSD__
    if (!kd) {
        kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
        if (!kd) throw KernelAccessFailed("FreeBSD kvm_open failed");
    }
#endif
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


