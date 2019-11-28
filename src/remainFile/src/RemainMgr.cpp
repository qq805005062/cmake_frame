
#include <mutex>

#include "Incommon.h"
#include "../RemainMgr.h"
#include "WriteFileMgr.h"
#include "ReadDirMgr.h"

namespace REMAIN_MGR
{

static UserIdFileMgrMap userFileMgrMap_;
static std::mutex mapMutex_;

RemainMgr::RemainMgr()
    :isExit_(0)
    ,isInit_(0)
    ,gateNo_(0)
    ,maxCount_(0)
    ,outSecond_(0)
    ,sharePath_()
    ,localPath_()
{
    INFO("RemainMgr init");
}

RemainMgr::~RemainMgr()
{
    ERROR("~RemainMgr exit");
}

void RemainMgr::remainMgrExit()
{
    INFO("RemainMgr remainMgrExit");
    isExit_ = 1;

    std::lock_guard<std::mutex> lock(mapMutex_);
    for(UserIdFileMgrMapIter iter = userFileMgrMap_.begin(); iter != userFileMgrMap_.end(); iter++)
    {
        iter->second->exitClose();
    }

    UserIdFileMgrMap ().swap(userFileMgrMap_);
}

int RemainMgr::remainMgrInit(uint32_t gateNo, int maxCount, int outSeconds, const std::string& sharePath, const std::string& localPath)
{
    gateNo_ = gateNo;
    maxCount_ = maxCount;
    outSecond_ = outSeconds;

    sharePath_.assign(sharePath);
    if(localPath.empty())
    {
        localPath_ = locakExePath();
    }else{
        localPath_.assign(localPath);
    }
    DEBUG("RemainMgr remainMgrInit gateNo_ %d maxCount_ %d outSecond_ %d sharePath_ %s localPath_ %s", gateNo_, maxCount_, outSecond_, sharePath_.c_str(), localPath_.c_str());
    isInit_ = 1;
    return 0;
}

int RemainMgr::writeRemainFile(int bitFlag, const std::string& fragFolder, const char *data, size_t dataSize)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainMgr writeRemainFile no init already");
        return -1314;
    }

    WriteFileMgrPtr fileMgrP(nullptr);
    {
        std::lock_guard<std::mutex> lock(mapMutex_);

        if(isExit_)
        {
            WARN("RemainMgr writeRemainFile exit flag had been set");
            return -131;
        }

        UserIdFileMgrMapIter iter = userFileMgrMap_.find(fragFolder);
        if(iter == userFileMgrMap_.end())
        {
            fileMgrP.reset(new WriteFileMgr(gateNo_, maxCount_, outSecond_, sharePath_, localPath_, fragFolder));
            if(fileMgrP == nullptr)
            {
                WARN("RemainMgr writeRemainFile WriteFileMgrPtr new nullptr");
                return -132;
            }
            userFileMgrMap_.insert(UserIdFileMgrMap::value_type(fragFolder, fileMgrP));
        }else{
            fileMgrP = iter->second;
        }
    }
    if(fileMgrP == nullptr)
    {
        WARN("RemainMgr writeRemainFile WriteFileMgrPtr new nullptr");
        ret = -1312;
        return ret;
    }
    switch(bitFlag)
    {
        case REMAIN_MGR_RE_FLAG:
            {
                ret = fileMgrP->writeToFile(RE_REMAIN_FLAG, data, dataSize);
                INFO("fileMgrP->writeToFile ret %d::%d", RE_REMAIN_FLAG, ret);
                break;
            }
        case REMAIN_MGR_MO_FLAG:
            {
                ret = fileMgrP->writeToFile(MO_REMAIN_FLAG, data, dataSize);
                INFO("fileMgrP->writeToFile ret %d::%d", MO_REMAIN_FLAG, ret);
                break;
            }
        case REMAIN_MGR_MT_FLAG:
            {
                ret = fileMgrP->writeToFile(MT_REMAIN_FLAG, data, dataSize);
                INFO("fileMgrP->writeToFile ret %d::%d", MT_REMAIN_FLAG, ret);
                break;
            }
        default:
            {
                WARN("RemainMgr writeRemainFile bitFlag %d", bitFlag);
                ret = -133;
                break;
            }
    }

    return ret;
}

int RemainMgr::writeRemainFileBak(int bitFlag, const std::string& fragFolder, const char *data, size_t dataSize)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainMgr writeRemainFileBak no init already");
        return -1314;
    }

    WriteFileMgrPtr fileMgrP(nullptr);
    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        if(isExit_)
        {
            WARN("RemainMgr writeRemainFileBak exit flag had been set");
            return -131;
        }

        UserIdFileMgrMapIter iter = userFileMgrMap_.find(fragFolder);
        if(iter == userFileMgrMap_.end())
        {
            fileMgrP.reset(new WriteFileMgr(gateNo_, maxCount_, outSecond_, sharePath_, localPath_, fragFolder));
            if(fileMgrP == nullptr)
            {
                WARN("RemainMgr writeRemainFileBak WriteFileMgrPtr new nullptr");
                return -135;
            }
            userFileMgrMap_.insert(UserIdFileMgrMap::value_type(fragFolder, fileMgrP));
        }else{
            fileMgrP = iter->second;
        }
    }
    if(fileMgrP == nullptr)
    {
        WARN("RemainMgr writeRemainFileBak WriteFileMgrPtr new nullptr");
        ret = -1312;
        return ret;
    }
    switch(bitFlag)
    {
        case REMAIN_MGR_RE_FLAG:
            {
                ret = fileMgrP->writeToFile(RE_REMAIN_BAK_FLAG, data, dataSize);
                INFO("fileMgrP->writeToFile ret %d::%d", RE_REMAIN_BAK_FLAG, ret);
                break;
            }
        case REMAIN_MGR_MO_FLAG:
            {
                ret = fileMgrP->writeToFile(MO_REMAIN_BAK_FLAG, data, dataSize);
                INFO("fileMgrP->writeToFile ret %d::%d", MO_REMAIN_BAK_FLAG, ret);
                break;
            }
        case REMAIN_MGR_MT_FLAG:
            {
                ret = fileMgrP->writeToFile(MT_REMAIN_BAK_FLAG, data, dataSize);
                INFO("fileMgrP->writeToFile ret %d::%d", MT_REMAIN_BAK_FLAG, ret);
                break;
            }
        default:
            {
                WARN("RemainMgr writeRemainFileBak bitFlag %d", bitFlag);
                ret = -136;
                break;
            }
    }

    return ret;
}

int RemainMgr::readRemainFile(int bitFlag, const std::string& fragFolder, std::string& remainData)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainMgr readRemainFile no init already");
        return -1314;
    }

    if(isExit_)
    {
        WARN("RemainMgr readRemainFile exit flag had been set");
        return -131;
    }

    switch(bitFlag)
    {
        case REMAIN_MGR_RE_FLAG:
            {
                ReadDirMgrPtr dirRead(new ReadDirMgr(RE_REMAIN_FLAG, gateNo_, fragFolder, sharePath_, localPath_));
                if(dirRead)
                {
                    ret = dirRead->readReaminData(remainData);
                    INFO("dirRead->readReaminData ret %d", ret);
                }else{
                    WARN("RemainMgr readRemainFile ReadDirMgrPtr nullptr");
                    ret = -138;
                }
                break;
            }
        case REMAIN_MGR_MO_FLAG:
            {
                ReadDirMgrPtr dirRead(new ReadDirMgr(MO_REMAIN_FLAG, gateNo_, fragFolder, sharePath_, localPath_));
                if(dirRead)
                {
                    ret = dirRead->readReaminData(remainData);
                    INFO("dirRead->readReaminData ret %d", ret);
                }else{
                    WARN("RemainMgr readRemainFile ReadDirMgrPtr nullptr");
                    ret = -139;
                }
                break;
            }
        case REMAIN_MGR_MT_FLAG:
            {
                ReadDirMgrPtr dirRead(new ReadDirMgr(MT_REMAIN_FLAG, gateNo_, fragFolder, sharePath_, localPath_));
                if(dirRead)
                {
                    ret = dirRead->readReaminData(remainData);
                    INFO("dirRead->readReaminData ret %d", ret);
                }else{
                    WARN("RemainMgr readRemainFile ReadDirMgrPtr nullptr");
                    ret = -1310;
                }
                break;
            }
        default:
            {
                WARN("RemainMgr readRemainFile bitFlag %d", bitFlag);
                ret = -1311;
                break;
            }
    }

    return ret;
}

int RemainMgr::statisticsRemainCount(int bitFlag, const std::string& fragFolder, int& localCount, int& shareCount)
{
    if(isInit_ == 0)
    {
        WARN("RemainMgr statisticsRemainCount no init already");
        return -1314;
    }

    if(isExit_)
    {
        WARN("RemainMgr statisticsRemainCount exit flag had been set");
        return -131;
    }

    localCount = 0;
    shareCount = 0;
    return 0;
}

void RemainMgr::everySecondCheck(uint64_t second)
{
    if(second == 0)
    {
        second = secondSinceEpoch();
    }

    std::lock_guard<std::mutex> lock(mapMutex_);
    if(isExit_)
    {
        WARN("RemainMgr exit flag had been set");
        return;
    }

    for(UserIdFileMgrMapIter iter = userFileMgrMap_.begin(); iter != userFileMgrMap_.end(); iter++)
    {
        iter->second->everySecondCheck(second);
    }
}

void RemainMgr::updateGenFreq(int freq)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    if(isExit_)
    {
        WARN("RemainMgr exit flag had been set");
        return;
    }

    for(UserIdFileMgrMapIter iter = userFileMgrMap_.begin(); iter != userFileMgrMap_.end(); iter++)
    {
        iter->second->updateGenFreq(freq);
    }
}

void RemainMgr::updateMaxFileSize(int size)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    if(isExit_)
    {
        WARN("RemainMgr exit flag had been set");
        return;
    }

    for(UserIdFileMgrMapIter iter = userFileMgrMap_.begin(); iter != userFileMgrMap_.end(); iter++)
    {
        iter->second->updateMaxFileSize(size);
    }
}

}
