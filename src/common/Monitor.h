#ifndef __RCS_MONITOR_H__
#define __RCS_MONITOR_H__

#include <mwnet_mt/net/MWEventLoop.h>
#include <common/MutexLock.h>
#include <common/Singleton.h>
#include <common/noncopyable.h>

#define RDNGATESVR_NAME			"rdngatesvr"
#define RDNSYNCLI_NAME			"rdnsyncli"
#define RDNURISVR_NAME			"rdnurisvr"
#define RSC_NAME				"rsc"

namespace common
{
#define MONITOR_BUFF_SIZE		2048

#define PROCESS_START_NUM		1300
#define CYCLE_MSG_NUM			1303
#define PUBLIC_MSG_NUM			6000

#define REDIS_ERROR_CODE		10000
#define REDIS_ERROR_VALUE		"redis"

#define MYSQL_ERROR_CODE		10001
#define MYSQL_ERROR_VALUE		"mysql"

#define KAFKA_ERROR_CODE		10002
#define KAFKA_ERROR_VALUE		"kafka"

#define MALLOC_ERROR_CODE		10003
#define MALLOC_ERROR_VALUE		"malloc"

#define EXCEPTION_ERROR_CODE	10004
#define EXCEPTION_ERROR_VALUE	"exception"

#define FILEOUTIME_NOUP_CODE	10005
#define FILEOUTIME_NOUP_VALUE	"fileoutime"

#define CEPH_ERROR_CODE			10006
#define CEPH_ERROR_VALUE		"ceph"

class Monitor : public common::noncopyable
{
public:
	typedef std::function<void()> CALLBACK_FUNCTION;
	
	Monitor();
	~Monitor();

	static Monitor& instance() { return common::Singleton<Monitor>::instance(); }

	int setCycleSecond(int second);
	
	void processStart(const char *who,bool start);

	void writeRedisErrorMsg();

	void writeKafkaErrorMsg();

	void writeMysqlErrorMsg();

	void systemMallocErrorMsg();

	void processExceptionErrorMsg();

	void fileOutimeNoUp(int num);

	void writeCephErrorMsg();
	
	void writeMonMsg(int msgNum,int msgType,const std::string& msg);

	void stopMonitor();

	//after second delay run callfunc 
	void runAfter(double delay, const CALLBACK_FUNCTION& callfunc);
	
	//every second interval run callfunc 
	void runEvery(double interval, const CALLBACK_FUNCTION& callfunc);
private:

	void processCyCleMsg();
	
	std::string transTimeFormat();

	std::string transTimeFormatAlign();
	
	void GetCurrentPath();

	void CreateFolder(std::string& path);
	
	void GetModuleFileName(char* szPath, int nMaxSize);
	
	int cycleSec;
	int64_t lastCycleSecond;
	std::string localPath;
	std::string monPath;
	std::string todayDate;
	const char *who;
	MWEVENTLOOP::EventLoop timeLoop;
};

}
#endif