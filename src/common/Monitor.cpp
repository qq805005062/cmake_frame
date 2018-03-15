#include <time.h>

#include <common/Timestamp.h>
#include <common/Version.h>

#include "process_stat_ex.h"
#include "Monitor.h"
namespace common
{

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)

static const char monStr[] = "<EVENT><EVTID>%d</EVTID><EVTTYPE>%d</EVTTYPE><EVTTM>%s</EVTTM><CREATETM>%s</CREATETM><VERSION>V5.2</VERSION><EVTCONT>%s</EVTCONT></EVENT>\n";

Monitor::Monitor()
	:cycleSec(30)
	,lastCycleSecond(0)
	,localPath()
	,monPath()
	,todayDate()
	,who(nullptr)
	,timeLoop()
{
}

Monitor::~Monitor()
{
	PERROR("Monitor exit\n");
}

int Monitor::setCycleSecond(int second)
{
	if(second % 10 == 0)
		cycleSec = second;
	else
		return -1;
	return 0;
}

void Monitor::processStart(const char *who,bool start)
{
	char buf[MONITOR_BUFF_SIZE] = {0};

	GetCurrentPath();
	this->who = who;
	if(start)
		snprintf(buf,MONITOR_BUFF_SIZE,"{\"STATUS\":\"0\",\"VER\":\"%s\",\"PATH\":\"%s\",\"INFO\":\"process %s start success\"}",MRCS_OUT_VERSION,localPath.c_str(),who);
	else
		snprintf(buf,MONITOR_BUFF_SIZE,"{\"STATUS\":\"0\",\"VER\":\"%s\",\"PATH\":\"%s\",\"INFO\":\"process %s server stop\"}",MRCS_OUT_VERSION,localPath.c_str(),who);

	if(start)
		writeMonMsg(PROCESS_START_NUM,19,buf);
	else
		writeMonMsg(PROCESS_START_NUM,10,buf);

	if(start)
		timeLoop.RunEvery(cycleSec,std::bind(&common::Monitor::processCyCleMsg,this));
}

void Monitor::processCyCleMsg()
{
	static int lastCpu = 0,lastDiskFree = 0;
	static uint64_t lastMem = 0,lastVmem = 0;
	
	int cpu = get_cpu_usage();
	if(cpu < 0)
	{
		PERROR("CPU usage error\n");
		cpu = lastCpu;
	}else if(cpu == 0){
		cpu = lastCpu;
	}else{
		lastCpu = cpu;
	}

	uint64_t mem = 0,vmem = 0;
	int ret = get_memory_usage(&mem,&vmem);
	if(ret < 0)
	{
		PERROR("memory usage error\n");
		mem = lastMem;
		vmem = lastVmem;
	}else{
		lastMem = mem;
		lastVmem = vmem;
	}
	mem = mem / (1000 * 1000);
	vmem = vmem / (1000 * 1000);
	
	int disk = get_disk_free_space(localPath.c_str());
	if(disk < 0)
	{
		PERROR("disk free space error\n");
		disk = lastDiskFree;
	}else{
		lastDiskFree = disk;
	}

	disk = disk / 1000;
	
	char buf[MONITOR_BUFF_SIZE] = {0};
	snprintf(buf,MONITOR_BUFF_SIZE,"{\"VER\":\"%s\",\"CPU\":\"%d\",\"MEM\":\"%lu\",\"VMEM\":\"%lu\",\"DISKFREE\":\"%d\",\"STATUS\":\"0\",\"MSG\":\"good\"}",MRCS_OUT_VERSION,cpu,mem,vmem,disk);

	writeMonMsg(CYCLE_MSG_NUM,99,buf);
}

void Monitor::writeRedisErrorMsg()
{
	static time_t last = 0;
	time_t now = time(NULL);
	if(now - last < 10)
		return;
	last = now;
	
	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"REDIS read or write ERROR\"}",REDIS_ERROR_CODE,this->who,REDIS_ERROR_VALUE);
	writeMonMsg(PUBLIC_MSG_NUM,10,buf);
}

void Monitor::writeKafkaErrorMsg()
{
	static time_t last = 0;
	time_t now = time(NULL);
	if(now - last < 10)
		return;
	last = now;

	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"KAFKA read or write ERROR\"}",KAFKA_ERROR_CODE,this->who,KAFKA_ERROR_VALUE);
	writeMonMsg(PUBLIC_MSG_NUM,10,buf);
}

void Monitor::writeMysqlErrorMsg()
{
	static time_t last = 0;
	time_t now = time(NULL);
	if(now - last < 10)
		return;
	last = now;
	
	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"MYSQL read or write ERROR\"}",MYSQL_ERROR_CODE,this->who,MYSQL_ERROR_VALUE);
	writeMonMsg(PUBLIC_MSG_NUM,10,buf);
}

void Monitor::systemMallocErrorMsg()
{
	static time_t last = 0;
	time_t now = time(NULL);
	if(now - last < 10)
		return;
	last = now;
	
	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"SYSTEM MALLOC ERROR\"}",MALLOC_ERROR_CODE,this->who,MALLOC_ERROR_VALUE);
	writeMonMsg(PUBLIC_MSG_NUM,10,buf);
}

void Monitor::processExceptionErrorMsg()
{
	static time_t last = 0;
	time_t now = time(NULL);
	if(now - last < 10)
		return;
	last = now;
	
	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"EXCEPTION OUT ERROR\"}",EXCEPTION_ERROR_CODE,this->who,EXCEPTION_ERROR_VALUE);
	writeMonMsg(PUBLIC_MSG_NUM,20,buf);
}

void Monitor::fileOutimeNoUp(int num)
{
	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"!!! 发现 %d 个文件超过10分钟仍未被上传!!!\"}",FILEOUTIME_NOUP_CODE,this->who,FILEOUTIME_NOUP_VALUE,num);
	writeMonMsg(PUBLIC_MSG_NUM,20,buf);
}

void Monitor::writeCephErrorMsg()
{
	char buf[MONITOR_BUFF_SIZE] = {0};

	snprintf(buf,MONITOR_BUFF_SIZE,"{\"MODCLS\":\"%d\",\"TYPE\":\"0\",\"WHO\":\"%s\",\"VALUE\":\"%s\",\"MSG\":\"CEPH ERROR please check\"}",CEPH_ERROR_CODE,this->who,CEPH_ERROR_VALUE);
	writeMonMsg(PUBLIC_MSG_NUM,10,buf);
}


void Monitor::writeMonMsg(int msgNum,int msgType,const std::string& msg)
{
	int ret = 0;
	char writeBuf[MONITOR_BUFF_SIZE] = {0},*pBuf = writeBuf;
	std::string time,timeAlign;

	if(msgNum == CYCLE_MSG_NUM)
	{
		time = transTimeFormat();
		timeAlign = transTimeFormatAlign();
		ret = snprintf(writeBuf,MONITOR_BUFF_SIZE,monStr,msgNum,msgType,timeAlign.c_str(),time.c_str(),msg.c_str());
	}
	else
	{
		time = transTimeFormat();
		ret = snprintf(writeBuf,MONITOR_BUFF_SIZE,monStr,msgNum,msgType,time.c_str(),time.c_str(),msg.c_str());
	}
	
	
	if(ret > MONITOR_BUFF_SIZE)
	{
		PERROR("motion msg too long more than buff size\n");
		return;
	}
	FILE* pFile = nullptr;
	if(msgNum == PROCESS_START_NUM)
	{
		std::string fileName = monPath + todayDate + "/10_mon_log.txt";
		pFile = fopen(fileName.c_str(),"a+");
	}else if(msgNum == CYCLE_MSG_NUM)
	{
		std::string fileName = monPath + todayDate + "/90_mon_log.txt";
		pFile = fopen(fileName.c_str(),"a+");
	}else if(msgNum == PUBLIC_MSG_NUM)
	{
		std::string fileName = monPath + todayDate + "/10_mon_log.txt";
		pFile = fopen(fileName.c_str(),"a+");
	}
	if(pFile == NULL)
	{
		PERROR("open write file error\n");
		return;
	}
	size_t nWri = ret;
	fwrite(pBuf,1,nWri,pFile);
	fclose(pFile);
	return;
	
}

void Monitor::stopMonitor()
{
	processStart(this->who,false);
}

void Monitor::runAfter(double delay, const CALLBACK_FUNCTION& callfunc)
{
	timeLoop.RunAfter(delay,callfunc);
}

void Monitor::runEvery(double interval, const CALLBACK_FUNCTION& callfunc)
{
	timeLoop.RunEvery(interval,callfunc);
}

std::string Monitor::transTimeFormat()
{
	common::Timestamp nowTime(common::Timestamp::now());
	int64_t millSecond = nowTime.microSecondsSinceEpoch() / 1000;

	int32_t milliS = static_cast<int32_t>(millSecond % 1000);
	time_t seconds = static_cast<time_t>(millSecond / 1000);

	struct tm tm_time;
	localtime_r(&seconds, &tm_time);

	char buf[128] = {0};
	sprintf(buf,"%04d-%02d-%02d %02d:%02d:%02d.%03d",(tm_time.tm_year + 1900),(tm_time.tm_mon + 1),tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,milliS);

	char date[128] = {0};
	sprintf(date,"%04d%02d%02d",(tm_time.tm_year + 1900),(tm_time.tm_mon + 1),tm_time.tm_mday);
	
	if(todayDate.empty())
	{	
		todayDate.append(date); 
		std::string dirPath = monPath + todayDate;
		CreateFolder(dirPath);
	}else{
		std::string dateStr(date);
		if(dateStr.compare(todayDate))
		{
			todayDate = dateStr;
			std::string dirPath = monPath + todayDate;
			CreateFolder(dirPath);
		}
	}
	
	return std::string(buf);
}


std::string Monitor::transTimeFormatAlign()
{
	common::Timestamp nowTime(common::Timestamp::now());
	int64_t millSecond = nowTime.microSecondsSinceEpoch() / 1000;

	int32_t milliS = static_cast<int32_t>(millSecond % 1000);
	time_t seconds;
	
	if(lastCycleSecond == 0)
	{
		seconds = static_cast<time_t>(millSecond / 1000);
		
		lastCycleSecond = seconds % cycleSec;
		lastCycleSecond = seconds - lastCycleSecond;
		seconds = lastCycleSecond;
	}else{
		lastCycleSecond += cycleSec;
		seconds = lastCycleSecond;
	}
	
	struct tm tm_time;
	localtime_r(&seconds, &tm_time);

	char buf[128] = {0};
	sprintf(buf,"%04d-%02d-%02d %02d:%02d:%02d.%03d",(tm_time.tm_year + 1900),(tm_time.tm_mon + 1),tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,milliS);

	return std::string(buf);
}

void Monitor::GetCurrentPath()
{
	if(localPath.empty())
	{
		char szPath[128] = {0};
    	GetModuleFileName(szPath, 128);
		std::string strPath(szPath);
		size_t found = strPath.find_last_of("/");
		localPath.append(szPath,found);

		monPath = localPath + "/mon_data/";
		CreateFolder(monPath);
	}
}

void Monitor::CreateFolder(std::string& path)
{
	char cmd[256] = {0};
	sprintf(cmd,"mkdir -p %s",path.c_str());
	system(cmd);
}

void Monitor::GetModuleFileName(char* szPath, int nMaxSize)
{
	readlink("/proc/self/exe", szPath, nMaxSize);
}


}

