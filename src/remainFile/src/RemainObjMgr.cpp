
#include "Incommon.h"
#include "../RemainObjMgr.h"
#include "../RemainMgr.h"
#include "Base64.h"

namespace REMAIN_MGR
{

RemainObjMgr::RemainObjMgr()
    :isExit_(0)
    ,isInit_(0)
    ,gateNo_(0)
    ,maxCount_(0)
    ,outSecond_(0)
    ,sharePath_()
    ,localPath_()
{
    INFO("RemainObjMgr init");
}

RemainObjMgr::~RemainObjMgr()
{
    ERROR("~RemainObjMgr exit");
}

void RemainObjMgr::remainObjMgrExit()
{
    INFO("RemainObjMgr remainObjMgrExit");

    isExit_ = 1;
    RemainMgr::instance().remainMgrExit();
}

int RemainObjMgr::remainObjMgrInit(uint32_t gateNo, int maxCount, int outSeconds, const std::string& sharePath, const std::string& localPath)
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

    RemainMgr::instance().remainMgrInit(gateNo, maxCount, outSeconds, sharePath, localPath);
    isInit_ = 1;
    DEBUG("remainObjMgrInit gateNo_ %d maxCount_ %d outSecond_ %d sharePath_ %s localPath_ %s", gateNo_, maxCount_, outSecond_, sharePath_.c_str(), localPath.c_str());
    return 0;
}

int RemainObjMgr::writeRemainObject(const RemainObjPtr& obj)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainObjMgr writeRemainObject no init already");
        return -1429;
    }

    if(isExit_)
    {
        WARN("RemainObjMgr writeRemainObject exit flag had been set");
        return -141;
    }

    std::string msgDetail;
    switch(obj->bitFlag_)
    {
        case REMAIN_OBJMGR_RE_FLAG:
            {
                msgDetail = transReToString(obj);
                DEBUG("writeRemainObject msgDetail %s", msgDetail.c_str());
                if(msgDetail.empty())
                {
                    ret = -1414;
                }
                break;
            }
        case REMAIN_OBJMGR_MO_FLAG:
            {
                msgDetail = transMoToString(obj);
                DEBUG("writeRemainObject msgDetail %s", msgDetail.c_str());
                if(msgDetail.empty())
                {
                    ret = -1415;
                }
                break;
            }
        case REMAIN_OBJMGR_MT_FLAG:
            {
                msgDetail = transMtToString(obj);
                DEBUG("writeRemainObject msgDetail %s", msgDetail.c_str());
                if(msgDetail.empty())
                {
                    ret = -1416;
                }
                break;
            }
        default:
            {
                WARN("writeRemainObject obj->remainBitFlag() %d", obj->bitFlag_);
                ret = -145;
                break;
            }
    }
    if(ret == 0)
    {
        ret = RemainMgr::instance().writeRemainFile(obj->bitFlag_, obj->m_strFragFolder, msgDetail.c_str(), msgDetail.length());
    }
    return ret;
}

int RemainObjMgr::writeRemainObjectBak(const RemainObjPtr& obj)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainObjMgr writeRemainObject no init already");
        return -1429;
    }

    if(isExit_)
    {
        WARN("RemainObjMgr writeRemainObject exit flag had been set");
        return -141;
    }

    std::string msgDetail;
    switch(obj->bitFlag_)
    {
        case REMAIN_OBJMGR_RE_FLAG:
            {
                msgDetail = transReToString(obj);
                DEBUG("writeRemainObjectBak msgDetail %s", msgDetail.c_str());
                if(msgDetail.empty())
                {
                    ret = -1417;
                }
                break;
            }
        case REMAIN_OBJMGR_MO_FLAG:
            {
                msgDetail = transMoToString(obj);
                DEBUG("writeRemainObjectBak msgDetail %s", msgDetail.c_str());
                if(msgDetail.empty())
                {
                    ret = -1418;
                }
                break;
            }
        case REMAIN_OBJMGR_MT_FLAG:
            {
                msgDetail = transMtToString(obj);
                DEBUG("writeRemainObjectBak msgDetail %s", msgDetail.c_str());
                if(msgDetail.empty())
                {
                    ret = -1419;
                }
                break;
            }
        default:
            {
                WARN("writeRemainObject obj->remainBitFlag() %d", obj->bitFlag_);
                ret = -144;
                break;
            }
    }
    if(ret == 0)
    {
        ret = RemainMgr::instance().writeRemainFileBak(obj->bitFlag_, obj->m_strFragFolder, msgDetail.c_str(), msgDetail.length());
    }
    return ret;
}

int RemainObjMgr::readRemainObject(int bitFlag, RemainObjPtrVect& objVect, const std::string& fragFolder)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainObjMgr writeRemainObject no init already");
        return -1429;
    }

    if(isExit_)
    {
        WARN("RemainObjMgr writeRemainObject exit flag had been set");
        return -141;
    }

    std::string msgDetail;
    ret = RemainMgr::instance().readRemainFile(bitFlag, fragFolder, msgDetail);
    DEBUG("readRemainObject msgDetail %d::%s", ret, msgDetail.c_str());
    if(ret < 0)
    {
        return ret;
    }

    ret = transStringToObject(bitFlag, fragFolder, msgDetail, objVect);
    DEBUG("transStringToObject %d:%ld", ret, objVect.size());

    return ret;
}

int RemainObjMgr::statisticsRemainCount(int bitFlag, const std::string& fragFolder, int& localCount, int& shareCount)
{
    int ret = 0;
    if(isInit_ == 0)
    {
        WARN("RemainObjMgr writeRemainObject no init already");
        return -1429;
    }

    if(isExit_)
    {
        WARN("RemainObjMgr writeRemainObject exit flag had been set");
        return -141;
    }

    ret = RemainMgr::instance().statisticsRemainCount(bitFlag, fragFolder, localCount, shareCount);
    return ret;
}

void RemainObjMgr::everySecondCheck(uint64_t second)
{
    if(isExit_)
    {
        WARN("RemainObjMgr writeRemainObject exit flag had been set");
        return;
    }

    if(second == 0)
    {
        second = secondSinceEpoch();
    }

    RemainMgr::instance().everySecondCheck(second);
}

void RemainObjMgr::updateGenFreq(int freq)
{
    RemainMgr::instance().updateGenFreq(freq);
}

void RemainObjMgr::updateMaxFileSize(int size)
{
    RemainMgr::instance().updateMaxFileSize(size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string RemainObjMgr::transReToString(const RemainObjPtr& obj)
{
    std::string baseResult;
    char strBuf[32] = { 0 };
    std::string msgDetail;
    msgDetail.reserve(4096);

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nPtMsgId);
    msgDetail.assign(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nSuppMsgId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nInitMgsId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgFmt);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->bitFlag_);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPkNumber);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPkTotal);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPhoneCount);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgLevel);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nEcid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPreNodeid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nLocalNodeid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nGateId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nBatchType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nRptFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nRptFrom);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nTpudhi);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nTppid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nLongMsgSeq);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeNum);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nSignPos);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nFileSize);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMmstype);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileCountry);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileArea);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileCity);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nFeeFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeRate);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgSeqId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nDttype);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nSendFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPassThrough);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nCreateTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRecvTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRespTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nInsertTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nSubmitTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDoneTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRecvRptTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nTranTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDealTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDeliverTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nAttime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nValidtime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nMsgHashCode);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nMwBatchId);
    msgDetail.append(strBuf).append(",");
///--------------------------------------------------------------------------
    baseResult.assign("");
    Base64::Encode(obj->m_strSpMsgId, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strPtcode).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserBatchId, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSvrType, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignature, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMsgHash256, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strCustName, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAccNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strBankNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strTransSeq, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAccInfo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strChangeMark, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strExitSign, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strCustNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strDepartCode, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignOrgcode, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strReserve, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSubmitter, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strReviewer, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignStatus, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strMsgSrcIp).append(",");

    msgDetail.append(obj->m_strSpip).append(",");

    msgDetail.append(obj->m_strUserId).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strPhone, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMsgContent, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserCustid, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserExData, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strSpNumber).append(",");

    msgDetail.append(obj->m_strSpGate).append(",");

    msgDetail.append(obj->m_strCpno).append(",");

    msgDetail.append(obj->m_strExno).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strServiceNo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAuthenInfo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strFileName, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSeqNo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strTplid, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMmsTitle, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMmsMsgPath, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strErrorCode).append(",");

    msgDetail.append(obj->m_strErrorCode2).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strErrorMsg, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strSubmitDate).append(",");
    
    msgDetail.append(obj->m_strDoneDate).append("\r\n");
    return msgDetail;
}

int RemainObjMgr::transStringToRe(const char* pBuf, RemainObjPtrVect& objVect)
{
    std::string baseResult;
    size_t bufLen = 0;
    const char *pComma = NULL;
    std::string tmpString;

    RemainObjPtr reObj(new RemainObj());
    if(reObj == nullptr)
    {
        return -1421;
    }

    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nPtMsgId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nSuppMsgId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nInitMgsId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgFmt = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->bitFlag_ = atoi(pBuf);
    if(reObj->bitFlag_ !=REMAIN_OBJMGR_RE_FLAG)
    {
        return -1426;
    }

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPkNumber = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPkTotal = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPhoneCount = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgLevel = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nEcid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPreNodeid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nLocalNodeid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nGateId = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nBatchType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nRptFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nRptFrom = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nTpudhi = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nTppid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nLongMsgSeq = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeNum = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nSignPos = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nFileSize = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMmstype = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileCountry = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileArea = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileCity = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nFeeFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeRate = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgSeqId = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nDttype = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nSendFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPassThrough = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nCreateTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRecvTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRespTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nInsertTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nSubmitTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDoneTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRecvRptTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nTranTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDealTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDeliverTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nAttime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nValidtime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nMsgHashCode = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMwBatchId = strtoul((const char *)(pBuf), NULL, 10);
//----------------------------------------------
    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSpMsgId = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strPtcode.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserBatchId = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSvrType = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignature = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMsgHash256 = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strCustName = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAccNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strBankNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strTransSeq = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAccInfo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strChangeMark = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strExitSign = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strCustNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strDepartCode = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignOrgcode = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strReserve = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSubmitter = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strReviewer = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignStatus = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strMsgSrcIp.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpip.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strUserId.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strPhone = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMsgContent = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserCustid = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserExData = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpNumber.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpGate.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strCpno.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strExno.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strServiceNo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAuthenInfo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strFileName = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSeqNo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strTplid = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMmsTitle = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMmsMsgPath = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strErrorCode.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strErrorCode2.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strErrorMsg = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSubmitDate.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    const char* pLine = strFirstConstStr(pBuf, "\r\n");
    if(pLine)
    {
        bufLen = pLine - pBuf;
    }else{
        bufLen = strlen(pBuf);
    }
    tmpString.assign(pBuf, bufLen);
    reObj->m_strDoneDate = tmpString;

    objVect.push_back(reObj);
    return 0;
}

std::string RemainObjMgr::transMoToString(const RemainObjPtr& obj)
{
    std::string baseResult;
    char strBuf[32] = { 0 };
    std::string msgDetail;
    msgDetail.reserve(2048);

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nPtMsgId);
    msgDetail.assign(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nSuppMsgId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nInitMgsId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgFmt);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->bitFlag_);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPkNumber);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPkTotal);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPhoneCount);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgLevel);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nEcid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPreNodeid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nLocalNodeid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nGateId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nBatchType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nRptFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nRptFrom);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nTpudhi);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nTppid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nLongMsgSeq);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeNum);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nSignPos);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nFileSize);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMmstype);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileCountry);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileArea);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileCity);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nFeeFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeRate);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgSeqId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nDttype);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nSendFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPassThrough);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nCreateTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRecvTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRespTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nInsertTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nSubmitTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDoneTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRecvRptTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nTranTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDealTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDeliverTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nAttime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nValidtime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nMsgHashCode);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nMwBatchId);
    msgDetail.append(strBuf).append(",");
///--------------------------------------------------------------------------
    baseResult.assign("");
    Base64::Encode(obj->m_strSpMsgId, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strPtcode).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserBatchId, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSvrType, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignature, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMsgHash256, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strCustName, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAccNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strBankNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strTransSeq, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAccInfo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strChangeMark, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strExitSign, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strCustNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strDepartCode, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignOrgcode, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strReserve, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSubmitter, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strReviewer, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignStatus, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strMsgSrcIp).append(",");

    msgDetail.append(obj->m_strSpip).append(",");

    msgDetail.append(obj->m_strUserId).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strPhone, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMsgContent, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserCustid, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserExData, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strSpNumber).append(",");

    msgDetail.append(obj->m_strSpGate).append(",");

    msgDetail.append(obj->m_strCpno).append(",");

    msgDetail.append(obj->m_strExno).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strServiceNo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAuthenInfo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strFileName, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSeqNo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strTplid, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMmsTitle, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMmsMsgPath, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strErrorCode).append(",");

    msgDetail.append(obj->m_strErrorCode2).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strErrorMsg, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strSubmitDate).append(",");
    
    msgDetail.append(obj->m_strDoneDate).append("\r\n");
    return msgDetail;
}

int RemainObjMgr::transStringToMo(const char* pBuf, RemainObjPtrVect& objVect)
{
    std::string baseResult;
    size_t bufLen = 0;
    const char *pComma = NULL;
    std::string tmpString;

    RemainObjPtr reObj(new RemainObj());
    if(reObj == nullptr)
    {
        return -1423;
    }

    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nPtMsgId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nSuppMsgId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nInitMgsId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgFmt = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->bitFlag_ = atoi(pBuf);
    if(reObj->bitFlag_ != REMAIN_OBJMGR_MO_FLAG)
    {
        return -1426;
    }

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPkNumber = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPkTotal = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPhoneCount = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgLevel = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nEcid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPreNodeid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nLocalNodeid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nGateId = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nBatchType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nRptFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nRptFrom = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nTpudhi = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nTppid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nLongMsgSeq = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeNum = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nSignPos = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nFileSize = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMmstype = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileCountry = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileArea = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileCity = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nFeeFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeRate = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgSeqId = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nDttype = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nSendFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPassThrough = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nCreateTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRecvTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRespTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nInsertTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nSubmitTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDoneTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRecvRptTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nTranTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDealTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDeliverTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nAttime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nValidtime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nMsgHashCode = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMwBatchId = strtoul((const char *)(pBuf), NULL, 10);
//----------------------------------------------
    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSpMsgId = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strPtcode.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserBatchId = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSvrType = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignature = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMsgHash256 = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strCustName = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAccNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strBankNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strTransSeq = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAccInfo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strChangeMark = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strExitSign = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strCustNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strDepartCode = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignOrgcode = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strReserve = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSubmitter = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strReviewer = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignStatus = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strMsgSrcIp.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpip.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strUserId.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strPhone = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMsgContent = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserCustid = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserExData = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpNumber.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpGate.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strCpno.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strExno.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strServiceNo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAuthenInfo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strFileName = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSeqNo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strTplid = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMmsTitle = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMmsMsgPath = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strErrorCode.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strErrorCode2.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strErrorMsg = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSubmitDate.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    const char* pLine = strFirstConstStr(pBuf, "\r\n");
    if(pLine)
    {
        bufLen = pLine - pBuf;
    }else{
        bufLen = strlen(pBuf);
    }
    tmpString.assign(pBuf, bufLen);
    reObj->m_strDoneDate = tmpString;

    objVect.push_back(reObj);
    return 0;
}

std::string RemainObjMgr::transMtToString(const RemainObjPtr& obj)
{
    std::string baseResult;
    char strBuf[32] = { 0 };
    std::string msgDetail;
    msgDetail.reserve(2048);

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nPtMsgId);
    msgDetail.assign(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nSuppMsgId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nInitMgsId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgFmt);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->bitFlag_);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPkNumber);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPkTotal);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPhoneCount);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgLevel);
    msgDetail.append(strBuf).append(",");
    
    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nEcid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPreNodeid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nLocalNodeid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nGateId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nBatchType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nRptFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nRptFrom);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nTpudhi);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nTppid);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nLongMsgSeq);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeNum);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nSignPos);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nFileSize);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMmstype);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileCountry);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileArea);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileCity);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMobileType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nFeeFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeType);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nChargeRate);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nMsgSeqId);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nDttype);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nSendFlag);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%d", obj->m_nPassThrough);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nCreateTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRecvTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRespTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nInsertTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nSubmitTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDoneTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nRecvRptTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nTranTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDealTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nDeliverTime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nAttime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nValidtime);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nMsgHashCode);
    msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
    sprintf(strBuf, "%ld", obj->m_nMwBatchId);
    msgDetail.append(strBuf).append(",");
///--------------------------------------------------------------------------
     baseResult.assign("");
    Base64::Encode(obj->m_strSpMsgId, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strPtcode).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserBatchId, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSvrType, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignature, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMsgHash256, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strCustName, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAccNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strBankNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strTransSeq, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAccInfo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strChangeMark, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strExitSign, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strCustNumber, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strDepartCode, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignOrgcode, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strReserve, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSubmitter, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strReviewer, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSignStatus, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strMsgSrcIp).append(",");

    msgDetail.append(obj->m_strSpip).append(",");

    msgDetail.append(obj->m_strUserId).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strPhone, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMsgContent, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserCustid, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strUserExData, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strSpNumber).append(",");

    msgDetail.append(obj->m_strSpGate).append(",");

    msgDetail.append(obj->m_strCpno).append(",");

    msgDetail.append(obj->m_strExno).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strServiceNo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strAuthenInfo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strFileName, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strSeqNo, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strTplid, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMmsTitle, &baseResult);
    msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strMmsMsgPath, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strErrorCode).append(",");

    msgDetail.append(obj->m_strErrorCode2).append(",");

    baseResult.assign("");
    Base64::Encode(obj->m_strErrorMsg, &baseResult);
    msgDetail.append(baseResult).append(",");

    msgDetail.append(obj->m_strSubmitDate).append(",");
    
    msgDetail.append(obj->m_strDoneDate).append("\r\n");
    return msgDetail;
}

int RemainObjMgr::transStringToMt(const char* pBuf, RemainObjPtrVect& objVect)
{
    std::string baseResult;
    size_t bufLen = 0;
    const char *pComma = NULL;
    std::string tmpString;

    RemainObjPtr reObj(new RemainObj());
    if(reObj == nullptr)
    {
        return -1425;
    }

    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nPtMsgId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nSuppMsgId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nInitMgsId = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgFmt = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->bitFlag_ = atoi(pBuf);
    if(reObj->bitFlag_ != REMAIN_OBJMGR_MT_FLAG)
    {
        return -1426;
    }

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPkNumber = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPkTotal = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPhoneCount = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgLevel = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nEcid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPreNodeid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nLocalNodeid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nGateId = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nBatchType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nRptFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nRptFrom = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nTpudhi = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nTppid = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nLongMsgSeq = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeNum = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nSignPos = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nFileSize = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMmstype = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileCountry = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileArea = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileCity = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMobileType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nFeeFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeType = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nChargeRate = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMsgSeqId = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nDttype = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nSendFlag = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nPassThrough = atoi(pBuf);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nCreateTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRecvTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRespTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nInsertTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nSubmitTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDoneTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nRecvRptTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nTranTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDealTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nDeliverTime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nAttime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nValidtime = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    reObj->m_nMsgHashCode = strtoul((const char *)(pBuf), NULL, 10);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1424;
    }
    reObj->m_nMwBatchId = strtoul((const char *)(pBuf), NULL, 10);
//----------------------------------------------
    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSpMsgId = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strPtcode.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserBatchId = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSvrType = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignature = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMsgHash256 = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strCustName = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAccNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strBankNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strTransSeq = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAccInfo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strChangeMark = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strExitSign = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strCustNumber = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strDepartCode = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignOrgcode = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strReserve = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSubmitter = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strReviewer = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSignStatus = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strMsgSrcIp.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpip.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strUserId.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strPhone = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMsgContent = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserCustid = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strUserExData = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpNumber.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSpGate.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strCpno.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strExno.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strServiceNo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strAuthenInfo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strFileName = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strSeqNo = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strTplid = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMmsTitle = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strMmsMsgPath = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strErrorCode.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strErrorCode2.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    reObj->m_strErrorMsg = baseResult;

    pBuf = pComma + 1;
    pComma = strFristConstChar(pBuf, ',');
    if(pComma == nullptr)
    {
        return -1422;
    }
    bufLen = pComma - pBuf;
    reObj->m_strSubmitDate.assign(pBuf, bufLen);

    pBuf = pComma + 1;
    const char* pLine = strFirstConstStr(pBuf, "\r\n");
    if(pLine)
    {
        bufLen = pLine - pBuf;
    }else{
        bufLen = strlen(pBuf);
    }
    tmpString.assign(pBuf, bufLen);
    reObj->m_strDoneDate = tmpString;

    objVect.push_back(reObj);
    return 0;
}

int RemainObjMgr::transStringToObject(int bigFlag, const std::string& fragFolder, const std::string& remainData, RemainObjPtrVect& objVect)
{
    int ret = 0;
    size_t bufLen = 0;
    char lineData[3072] = {0};
    const char* pLine = NULL, *pBegin = remainData.c_str();
    if(remainData.empty())
    {
        return -1420;
    }

    switch(bigFlag)
    {
        case REMAIN_OBJMGR_RE_FLAG:
            {
                do{
                    if(pLine)
                    {
                        bufLen = pLine - pBegin;
                        DEBUG("bufLen %ld", bufLen);
                        if(bufLen < 3072)
                        {
                            memset(lineData, 0 , 3072);
                            memcpy(lineData, pBegin, bufLen);
                            DEBUG("transStringToRe %s:%ld", lineData, bufLen);
                            ret = transStringToRe(lineData, objVect);
                            DEBUG("transStringToRe %d:%ld", ret, objVect.size());
                        }else{
                            ret = -1428;
                        }
                        if(ret < 0)
                        {
                            std::string tmpStr(pBegin, bufLen);
                            RemainMgr::instance().writeRemainFileBak(REMAIN_OBJMGR_RE_FLAG, fragFolder, tmpStr.c_str(), tmpStr.length());//TODO
                        }
                        pBegin = pLine + 2;
                    }	
                    pLine = strFirstConstStr(pBegin, "\r\n");
                    DEBUG("strFirstConstStr %p", pLine);
                }while(pLine);
                break;
            }
        case REMAIN_OBJMGR_MO_FLAG:
            {
                do{
                    if(pLine)
                    {
                        bufLen = pLine - pBegin;
                        if(bufLen < 3072)
                        {
                            memset(lineData, 0 , 3072);
                            memcpy(lineData, pBegin, bufLen);
                            ret = transStringToMo(lineData, objVect);
                            DEBUG("transStringToMo %d:%ld", ret, objVect.size());
                        }else{
                            ret = -1428;
                        }
                        if(ret < 0)
                        {
                            std::string tmpStr(pBegin, bufLen);
                            RemainMgr::instance().writeRemainFileBak(REMAIN_OBJMGR_MO_FLAG, fragFolder, tmpStr.c_str(), tmpStr.length());//TODO
                        }
                        pBegin = pLine + 2;
                    }	
                    pLine = strFirstConstStr(pBegin, "\r\n");
                }while(pLine);
                break;
            }
        case REMAIN_OBJMGR_MT_FLAG:
            {
                do{
                    if(pLine)
                    {
                        bufLen = pLine - pBegin;
                        DEBUG("bufLen %ld", bufLen);
                        if(bufLen < 3072)
                        {
                            memset(lineData, 0 , 3072);
                            memcpy(lineData, pBegin, bufLen);
                            DEBUG("transStringToMt %s:%ld", lineData, bufLen);
                            ret = transStringToMt(lineData, objVect);
                            DEBUG("transStringToMt %d:%ld", ret, objVect.size());
                        }else{
                            ret = -1428;
                        }
                        if(ret < 0)
                        {
                            std::string tmpStr(pBegin, bufLen);
                            RemainMgr::instance().writeRemainFileBak(REMAIN_OBJMGR_MT_FLAG, fragFolder, tmpStr.c_str(), tmpStr.length());//TODO
                        }
                        pBegin = pLine + 2;
                    }	
                    pLine = strFirstConstStr(pBegin, "\r\n");
                    DEBUG("strFirstConstStr %p", pLine);
                }while(pLine);
                break;
            }
        default:
            {
                ret = -1427;
                break;
            }
    }

    return ret;
}


}
