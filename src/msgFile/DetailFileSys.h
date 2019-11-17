#ifndef __SMSFWD_DETAIL_FILESYS_H__
#define __SMSFWD_DETAIL_FILESYS_H__

#include <memory>
#include <deque>

#include "../MutexLock.h"
#include "../Condition.h"
#include "../noncopyable.h"
#include "../Singleton.h"

#include "../SmsMoProtocol.h"
#include "../SmsMtProtocol.h"

#include "MessageFilesImpl.h"

#include <mwnet_mt/util/MWThreadPool.h>

typedef enum DetailType
{
    recvMsg,
    mtMsg,
    recvRe,
    sendRe,
    recvMo,
    sendMo,
    unknow,
}DETAILTYPE;

#define DETAIL_RECV_FLAG                0
#define DETAIL_SENDOK_FLAG              1

namespace SMSFWD
{

class DetailFileSys : public COMMON::noncopyable
{
public:
    DetailFileSys();
    ~DetailFileSys();
    
    static DetailFileSys& instance() { return COMMON::Singleton<DetailFileSys>::instance(); }

    int detailFileSysInit(int threadNum, const std::string& ptCodeSt, int gateNo, int maxFileSize, int genFreq);

    void detailFileSysExit();

    void writeMoinfoMsgfile(const MoProtocolPtr& mo, int flag);

    void writeMtinfoMsgfile(const MtProtocolPtr& mt, int flag);

    void writeFileSys(const std::string& detail, DETAILTYPE type);

    size_t detailQueueSize();

    void updateMaxFileSize(int size);

    void updateMaxgenFreq(int freq);

    void everySecondCheck();

    void writeDetailThread();

private:

    int isExit_;
    int maxFileSize_;
    int genFreq_;
    std::unique_ptr<MessageFiles> msgFiles_;
    std::unique_ptr<MWTHREADPOOL::ThreadPool> writeIoPoolPtr_;

};

}
#endif
