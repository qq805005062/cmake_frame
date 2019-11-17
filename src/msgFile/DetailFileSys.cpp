
#include <common/Base64.h>

#include "../Incommon.h"

#include "DetailFileSys.h"

typedef std::shared_ptr<std::string> StdStringPtr;

namespace SMSFWD
{

class DetailInfo
{
public:
    DetailInfo(const StdStringPtr& info, DETAILTYPE t)
        :type(t)
        ,detail(info)
    {
        PDEBUG("DetailInfo init");
    }

    ~DetailInfo()
    {
        PERROR("~DetailInfo exit");
    }

    void setDetailType(DETAILTYPE t)
    {
        type = t;
    }

    void setDetailStr(const StdStringPtr& info)
    {
        detail = info;
    }

    DETAILTYPE detailType()
    {
        return type;
    }

    StdStringPtr detailStr()
    {
        return detail;
    }
private:
    DETAILTYPE type;
    StdStringPtr detail;
};

typedef std::shared_ptr<DetailInfo> DetailInfoPtr;

class DetailQueue : public COMMON::noncopyable
{
public:
    DetailQueue()
        :isExit_(0)
        ,maxSize_(10000)
        ,mutex_()
        ,notEmpty_(mutex_)
        ,queue_()
    {
        PDEBUG("DetailQueue init");
    }

    ~DetailQueue()
    {
        PERROR("~DetailQueue exit");
    }

    static DetailQueue& instance() { return COMMON::Singleton<DetailQueue>::instance();}

    void setMaxQueueSize(int maxSize) { maxSize_ = maxSize; }
    
    void detailQueueExit()
    {
        isExit_ = 1;
        notEmpty_.notifyAll();
    }

    int detailInsert(DetailInfoPtr& detail)
    {
        int ret = 0;
        COMMON::SafeMutexLock lock(mutex_);
        if(isFull())
        {
            ret = 1;
        }
        queue_.push_back(detail);
        notEmpty_.notify();
        return ret;
    }

    DetailInfoPtr detailWrite()
    {
        DetailInfoPtr detail(nullptr);
        COMMON::SafeMutexLock lock(mutex_);
        while(queue_.empty())
        {
            if(isExit_)
            {
                return detail;
            }
            notEmpty_.wait();
        }

        detail = queue_.front();
        queue_.pop_front();

        return detail;
    }

    size_t queueSize()
    {
        COMMON::SafeMutexLock lock(mutex_);
        if(queue_.size() == 0)
        {
            std::deque<DetailInfoPtr> ().swap(queue_);
        }
        return queue_.size();
    }

private:

    bool isFull() const
    {
        return maxSize_ > 0 && queue_.size() >= maxSize_;
    }

    int isExit_;

    size_t maxSize_;
    COMMON::MutexLock mutex_;
    COMMON::Condition notEmpty_;
    std::deque<DetailInfoPtr> queue_;
};

DetailFileSys::DetailFileSys()
    :isExit_(0)
    ,maxFileSize_(20)
    ,genFreq_(60)
    ,msgFiles_(MessageFilesFactory::New())
    ,writeIoPoolPtr_()
{
    PDEBUG("DetailFileSys init");
}

DetailFileSys::~DetailFileSys()
{
    PERROR("~DetailFileSys exit");
}

int DetailFileSys::detailFileSysInit(int threadNum, const std::string& ptCodeSt, int gateNo, int maxFileSize, int genFreq)
{
    int ret = 0;

    if(msgFiles_ == nullptr)
    {
        TCPSVR_LOG_ERROR("message file system new nullptr");
        return -1;
    }

    maxFileSize < 1 ? maxFileSize = 20 : 1;
    maxFileSize > 1000 ? maxFileSize = 20 : 1;

    genFreq < 1 ? genFreq = 60 : 1;
    genFreq > 1800 ? genFreq = 60 : 1;

    if(msgFiles_->InitParams(ptCodeSt, gateNo) == false)
    {
        TCPSVR_LOG_ERROR("message file system InitParams false");
        return -1;
    }

    ret = msgFiles_->InitFwdMsgDetailsFile(ptCodeSt, gateNo, FWD_MSG_DETAIL, maxFileSize, genFreq);
    if(ret < 0)
    {
        TCPSVR_LOG_ERROR("message file system InitFwdMsgDetailsFile %d", ret);
        return -1;
    }

    ret = msgFiles_->InitFwdMsgOkDetailsFile(ptCodeSt, gateNo, FWD_MSGOK_DETAIL, maxFileSize, genFreq);
    if(ret < 0)
    {
        TCPSVR_LOG_ERROR("message file system InitFwdMsgOkDetailsFile %d", ret);
        return -1;
    }

    ret = msgFiles_->InitReDetailsFile(ptCodeSt, gateNo, FWD_RE_DETAIL, maxFileSize, genFreq);
    if(ret < 0)
    {
        TCPSVR_LOG_ERROR("message file system InitReDetailsFile %d", ret);
        return -1;
    }

    ret = msgFiles_->InitReOkDetailsFile(ptCodeSt, gateNo, FWD_REOK_DETAIL, maxFileSize, genFreq);
    if(ret < 0)
    {
        TCPSVR_LOG_ERROR("message file system InitReOkDetailsFile %d", ret);
        return -1;
    }

    ret = msgFiles_->InitMoDetailsFile(ptCodeSt, gateNo, FWD_MO_DETAIL, maxFileSize, genFreq);
    if(ret < 0)
    {
        TCPSVR_LOG_ERROR("message file system InitMoDetailsFile %d", ret);
        return -1;
    }

    ret = msgFiles_->InitMoOkDetailsFile(ptCodeSt, gateNo, FWD_MOOK_DETAIL, maxFileSize, genFreq);
    if(ret < 0)
    {
        TCPSVR_LOG_ERROR("message file system InitMoOkDetailsFile %d", ret);
        return -1;
    }

    writeIoPoolPtr_.reset(new MWTHREADPOOL::ThreadPool("detail"));
    if(writeIoPoolPtr_ == nullptr)
    {
        TCPSVR_LOG_ERROR("detail write Thread malloc empty");
        return -1;
    }

    writeIoPoolPtr_->StartThreadPool(threadNum);
    for(int i = 0; i < threadNum; i++)
    {
        writeIoPoolPtr_->RunTask(std::bind(&DetailFileSys::writeDetailThread, this));
    }
    maxFileSize_ = maxFileSize;
    genFreq_ = genFreq;
    
    common::Monitor::instance().runEvery(1.0 , std::bind(&DetailFileSys::everySecondCheck, this));
    return ret;
}

void DetailFileSys::detailFileSysExit()
{
    isExit_ = 1;
    DetailQueue::instance().detailQueueExit();
    while(DetailQueue::instance().queueSize() > 0)
    {
        usleep(100);
    }

    if(writeIoPoolPtr_)
    {
        writeIoPoolPtr_->StopThreadPool();
        writeIoPoolPtr_.reset();
    }

    msgFiles_->CloseAll();
    return;
}

void DetailFileSys::writeMoinfoMsgfile(const MoProtocolPtr& mo, int flag)
{
    std::string baseResult;
    char strBuf[32] = { 0 };
    std::string msgDetail;
    msgDetail.reserve(2048);
    
    if(mo->moProtocolFlag() == MO_PROTOCOL_REPORT_TYPE)
    {
        unsigned char ptMsgIdChar[9] = {0};
        memcpy(ptMsgIdChar, mo->moProtocolMsgId().c_str(), PTMSGID_MEMORY_LEN);

        int64_t ptMsgId = TranMsgIdCharToI64(ptMsgIdChar);
        memset(strBuf, 0, 32);
        sprintf(strBuf, "%ld", ptMsgId);
        msgDetail.assign(strBuf).append(",");

        msgDetail.append(mo->moProtocolUserId()).append(",");

        baseResult.assign("");
        common::Base64::Encode(mo->moProtocolStrPhone(), &baseResult);
        msgDetail.append(baseResult).append(",");

        msgDetail.append(mo->moProtocolSpgateNo()).append(",");

        msgDetail.append(mo->moProtocolExNo()).append(",");

        baseResult.assign("");
        common::Base64::Encode(mo->moProtocolCustid(), &baseResult);
        msgDetail.append(baseResult).append(",");

        baseResult.assign("");
        common::Base64::Encode(mo->moProtocolUserExdata(), &baseResult);
        msgDetail.append(baseResult).append(",");

        msgDetail.append(mo->moProtocolState()).append(",");

        msgDetail.append(mo->moProtocolSendTime()).append(",");

        msgDetail.append(mo->moProtocolRecvTime()).append(",");

        memset(strBuf, 0, 32);
        sprintf(strBuf, "%d", mo->moProtocolPkNum());
        msgDetail.append(strBuf).append(",");

        memset(strBuf, 0, 32);
        sprintf(strBuf, "%d", mo->moProtocolPkTotal());
        msgDetail.append(strBuf).append("\r\n");

        if(flag == 0)
        {
            this->writeFileSys(msgDetail, recvRe);
        }else{
            this->writeFileSys(msgDetail, sendRe);
        }
    }else if(mo->moProtocolFlag() == MO_PROTOCOL_SMS_TYPE)
    {
        unsigned char ptMsgIdChar[9] = {0};
        memcpy(ptMsgIdChar, mo->moProtocolMsgId().c_str(), 8);

        int64_t ptMsgId = TranMsgIdCharToI64(ptMsgIdChar);
        memset(strBuf, 0, 32);
        sprintf(strBuf, "%ld", ptMsgId);
        msgDetail.assign(strBuf).append(",");

        msgDetail.append(mo->moProtocolUserId()).append(",");

        baseResult.assign("");
        common::Base64::Encode(mo->moProtocolStrPhone(), &baseResult);
        msgDetail.append(baseResult).append(",");

        msgDetail.append(mo->moProtocolSpgateNo()).append(",");

        msgDetail.append(mo->moProtocolExNo()).append(",");

        msgDetail.append(mo->moProtocolSendTime()).append(",");

        msgDetail.append(mo->moProtocolRecvTime()).append(",");

        memset(strBuf, 0, 32);
        sprintf(strBuf, "%d", mo->moProtocolMsgFmt());
        msgDetail.append(strBuf).append(",");

        baseResult.assign("");
        common::Base64::Encode(mo->moProtocolMsgContent(), &baseResult);
        msgDetail.append(baseResult).append("\r\n");

        if(flag == 0)
        {
            this->writeFileSys(msgDetail, recvMo);
        }else{
            this->writeFileSys(msgDetail, sendMo);
        }
    }else{
        TCPSVR_LOG_WARN("MoProtocolPtr flag %d unknow can't be write", mo->moProtocolFlag());
    }
    
}

void DetailFileSys::writeMtinfoMsgfile(const MtProtocolPtr& mt, int flag)
{
    std::string baseResult;
    char strBuf[32] = { 0 };
    std::string msgDetail;
    msgDetail.reserve(2048);

    unsigned char ptMsgIdChar[9] = {0};
    memcpy(ptMsgIdChar, mt->mtProtocolMsgId().c_str(), 8);

    int64_t ptMsgId = TranMsgIdCharToI64(ptMsgIdChar);
    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", ptMsgId);
    msgDetail.assign(strBuf).append(",");

    msgDetail.append(mt->mtProtocolUserId()).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", mt->mtProtocolPhoneCount());
    msgDetail.append(strBuf).append(",");

    baseResult.assign("");
    common::Base64::Encode(mt->mtProtocolStrPhone(), &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(mt->mtProtocolSpgateNo()).append(",");

    msgDetail.append(mt->mtProtocolStrExNo()).append(",");

    baseResult.assign("");
    common::Base64::Encode(mt->mtProtocolCustid(), &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    common::Base64::Encode(mt->mtProtocolUserExdata(), &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(mt->mtProtocolUserIpaddr()).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", mt->mtProtocolMsgFmt());
    msgDetail.append(strBuf).append(",");

    baseResult.assign("");
    common::Base64::Encode(mt->mtProtocolMsgContent(), &baseResult);
    msgDetail.append(baseResult).append("\r\n");

    if(flag == 0)
    {
        this->writeFileSys(msgDetail, recvMsg);
    }else{
        this->writeFileSys(msgDetail, mtMsg);
    }
}

void DetailFileSys::writeFileSys(const std::string& detail, DETAILTYPE type)
{
    if(isExit_)
    {
        return;
    }
    StdStringPtr infoStr(new std::string(detail));
    if(infoStr)
    {
        DetailInfoPtr detailInfo(new DetailInfo(infoStr, type));
        if(detailInfo)
        {
            int ret = DetailQueue::instance().detailInsert(detailInfo);
            if(ret)
            {
                TCPSVR_LOG_WARN("detail write queue full may be io too slow or something other wrong");
                common::Monitor::instance().writeFileErrorMsg();
            }
            return;
        }
    }
    TCPSVR_LOG_WARN("detail new nullptr may be drop %s", detail.c_str());
    return;
}

size_t DetailFileSys::detailQueueSize()
{
    return DetailQueue::instance().queueSize();
}

void DetailFileSys::updateMaxFileSize(int size)
{
    size < 1 ? size = 20 : 1;
    size > 1000 ? size = 20 : 1;
    
    if(size != maxFileSize_)
    {
        if (msgFiles_)
        {
            msgFiles_->updateMaxFileSize(size);
        }
        maxFileSize_ = size;
    }
}

void DetailFileSys::updateMaxgenFreq(int freq)
{
    freq < 1 ? freq = 60 : 1;
    freq > 1800 ? freq = 60 : 1;
    
    if(genFreq_ != freq)
    {
        if (msgFiles_)
        {
            msgFiles_->updateGenFreq(freq);
        }
        genFreq_ = freq;
    }
}

void DetailFileSys::everySecondCheck()
{
    if (msgFiles_)
    {
        msgFiles_->SwitchMessageFiles();
    }
}

void DetailFileSys::writeDetailThread()
{
    TCPSVR_LOG_INFO("detail write thread entry");
    while(1)
    {
        START_CATCH_EX();
        DetailInfoPtr detailInfo = DetailQueue::instance().detailWrite();
        if((detailInfo == nullptr) && isExit_)
        {
            break;
        }

        switch(detailInfo->detailType())
        {
            case recvMsg:
                { 
                    StdStringPtr info = detailInfo->detailStr();
                    if (msgFiles_)
                    {
                        int ret = msgFiles_->WriteFile(FWD_MSG_DETAIL, *info);
                        TCPSVR_LOG_DEBUG("recv msg detail %s:%d", info->c_str(),ret);
                        if(ret < 0)
                        {
                            TCPSVR_LOG_WARN("msgFiles_->WriteFile ret %d", ret);
                            common::Monitor::instance().writeFileErrorMsg();
                        }
                    }else{
                        TCPSVR_LOG_WARN("detail new nullptr may be drop %s", info->c_str());
                    }
                    break;
                }
            case mtMsg:
                {
                    StdStringPtr info = detailInfo->detailStr();
                    if (msgFiles_)
                    {
                        int ret = msgFiles_->WriteFile(FWD_MSGOK_DETAIL, *info);
                        TCPSVR_LOG_DEBUG("send msg detail %s:%d", info->c_str(), ret);
                        if(ret < 0)
                        {
                            TCPSVR_LOG_WARN("msgFiles_->WriteFile ret %d", ret);
                            common::Monitor::instance().writeFileErrorMsg();
                        }
                    }else{
                        TCPSVR_LOG_WARN("detail new nullptr may be drop %s", info->c_str());
                    }
                    break;
                }
            case recvRe:
                {
                    StdStringPtr info = detailInfo->detailStr();
                    if (msgFiles_)
                    {
                        int ret = msgFiles_->WriteFile(FWD_RE_DETAIL, *info);
                        TCPSVR_LOG_DEBUG("recv re detail %s:%d", info->c_str(), ret);
                        if(ret < 0)
                        {
                            TCPSVR_LOG_WARN("msgFiles_->WriteFile ret %d", ret);
                            common::Monitor::instance().writeFileErrorMsg();
                        }
                    }else{
                        TCPSVR_LOG_WARN("detail new nullptr may be drop %s", info->c_str());
                    }
                    break;
                }
            case sendRe:
                {
                    StdStringPtr info = detailInfo->detailStr();
                    if (msgFiles_)
                    {
                        int ret = msgFiles_->WriteFile(FWD_REOK_DETAIL, *info);
                        TCPSVR_LOG_DEBUG("send re detail %s:%d", info->c_str(), ret);
                        if(ret < 0)
                        {
                            TCPSVR_LOG_WARN("msgFiles_->WriteFile ret %d", ret);
                            common::Monitor::instance().writeFileErrorMsg();
                        }
                    }else{
                        TCPSVR_LOG_WARN("detail new nullptr may be drop %s", info->c_str());
                    }
                    break;
                }
            case recvMo:
                {
                    StdStringPtr info = detailInfo->detailStr();
                    if (msgFiles_)
                    {
                        int ret = msgFiles_->WriteFile(FWD_MO_DETAIL, *info);
                        TCPSVR_LOG_DEBUG("recv mo detail %s:%d", info->c_str(), ret);
                        if(ret < 0)
                        {
                            TCPSVR_LOG_WARN("msgFiles_->WriteFile ret %d", ret);
                            common::Monitor::instance().writeFileErrorMsg();
                        }
                    }else{
                        TCPSVR_LOG_WARN("detail new nullptr may be drop %s", info->c_str());
                    }
                    break;
                }
            case sendMo:
                {
                    StdStringPtr info = detailInfo->detailStr();
                    if (msgFiles_)
                    {
                        int ret = msgFiles_->WriteFile(FWD_MOOK_DETAIL, *info);
                        TCPSVR_LOG_DEBUG("send mo detail %s:%d", info->c_str(), ret);
                        if(ret < 0)
                        {
                            TCPSVR_LOG_WARN("msgFiles_->WriteFile ret %d", ret);
                            common::Monitor::instance().writeFileErrorMsg();
                        }
                    }else{
                        TCPSVR_LOG_WARN("detail new nullptr may be drop %s", info->c_str());
                    }
                    break;
                }
            default:
                {
                    StdStringPtr info = detailInfo->detailStr();
                    TCPSVR_LOG_WARN("detail unknow type %d may be drop %s", detailInfo->detailType(), info->c_str());
                    common::Monitor::instance().systemDecodeErrorMsg();
                    break;
                }
        }
        END_CATCH_EX();
    }
    TCPSVR_LOG_INFO("detail write thread exit");
    return;
}

}

