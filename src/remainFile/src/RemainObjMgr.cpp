
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
	switch(obj->remainBitFlag())
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
				WARN("writeRemainObject obj->remainBitFlag() %d", obj->remainBitFlag());
				ret = -145;
				break;
			}
	}
	if(ret == 0)
	{
		ret = RemainMgr::instance().writeRemainFile(obj->remainBitFlag(), obj->remainUserId(), msgDetail.c_str(), msgDetail.length());
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
	switch(obj->remainBitFlag())
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
				WARN("writeRemainObject obj->remainBitFlag() %d", obj->remainBitFlag());
				ret = -144;
				break;
			}
	}
	if(ret == 0)
	{
		ret = RemainMgr::instance().writeRemainFileBak(obj->remainBitFlag(), obj->remainUserId(), msgDetail.c_str(), msgDetail.length());
	}
	return ret;
}

int RemainObjMgr::readRemainObject(int bitFlag, const std::string& userId, RemainObjPtrVect& objVect)
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
	ret = RemainMgr::instance().readRemainFile(bitFlag, userId, msgDetail);
	DEBUG("readRemainObject msgDetail %d::%s", ret, msgDetail.c_str());
	if(ret < 0)
	{
		return ret;
	}

	ret = transStringToObject(bitFlag, userId, msgDetail, objVect);
    DEBUG("transStringToObject %d:%ld", ret, objVect.size());
	
	return ret;
}

int RemainObjMgr::statisticsRemainCount(int bitFlag, const std::string& userId, int& localCount, int& shareCount)
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
	
	ret = RemainMgr::instance().statisticsRemainCount(bitFlag, userId, localCount, shareCount);
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
	msgDetail.reserve(2048);

	unsigned char ptMsgIdChar[9] = {0};
	memcpy(ptMsgIdChar, obj->remainPtMsgId().c_str(), 8);
	
	int64_t ptMsgId = TranMsgIdCharToI64(ptMsgIdChar);
	memset(strBuf, 0, 32);
	sprintf(strBuf, "%ld", ptMsgId);
	msgDetail.assign(strBuf).append(",");
	
	msgDetail.append(obj->remainUserId()).append(",");

    baseResult.assign("");
	Base64::Encode(obj->remainStrPhone(), &baseResult);
	msgDetail.append(baseResult).append(",");

	msgDetail.append(obj->remainSpgateNo()).append(",");

	msgDetail.append(obj->remainExNo()).append(",");

    baseResult.assign("");
    Base64::Encode(obj->remainUserCustid(), &baseResult);
	msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->remainUserExData(), &baseResult);
	msgDetail.append(baseResult).append(",");
	
	msgDetail.append(obj->remainState()).append(",");

	msgDetail.append(obj->remainSendTime()).append(",");

	msgDetail.append(obj->remainRecvTime()).append(",");

	memset(strBuf, 0, 32);
	sprintf(strBuf, "%d", obj->remainPkNum());
	msgDetail.append(strBuf).append(",");

    memset(strBuf, 0, 32);
	sprintf(strBuf, "%d", obj->remainPkTotal());
	msgDetail.append(strBuf).append("\r\n");
	return msgDetail;
}

int RemainObjMgr::transStringToRe(const char* pBuf, RemainObjPtrVect& objVect)
{
    std::string baseResult;
	size_t bufLen = 0;
	unsigned char strBuf[32] = { 0 };
	const char *pComma = NULL;
	std::string tmpString;

	RemainObjPtr reObj(new RemainObj());
	if(reObj == nullptr)
	{
		return -1421;
	}

	reObj->setRemainBitFlag(REMAIN_OBJMGR_RE_FLAG);
	
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	int64_t srcMsgid = strtoul((const char *)(pBuf), NULL, 10);
	TranMsgIdI64ToChar(strBuf, srcMsgid);
	reObj->setRemainPtMsgId((const char*)(strBuf), sizeof(int64_t));

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainUserId(tmpString);

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
	reObj->setRemainStrPhone(baseResult);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainSpgateNo(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainExNo(tmpString);

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
	reObj->setRemainUserCustid(baseResult);

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
	reObj->setRemainUserExData(baseResult);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainState(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainSendTime(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainRecvTime(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	int type = atoi(pBuf);
	reObj->setRemainPkNum(type);

    pBuf = pComma + 1;
    type = atoi(pBuf);
	reObj->setRemainPkTotal(type);
	
	objVect.push_back(reObj);
	return 0;
}

std::string RemainObjMgr::transMoToString(const RemainObjPtr& obj)
{
    std::string baseResult;
	char strBuf[32] = { 0 };
	std::string msgDetail;
	msgDetail.reserve(2048);

	unsigned char ptMsgIdChar[9] = {0};
	memcpy(ptMsgIdChar, obj->remainPtMsgId().c_str(), 8);
	
	int64_t ptMsgId = TranMsgIdCharToI64(ptMsgIdChar);
	memset(strBuf, 0, 32);
	sprintf(strBuf, "%ld", ptMsgId);
	msgDetail.assign(strBuf).append(",");

	msgDetail.append(obj->remainUserId()).append(",");

    baseResult.assign("");
    Base64::Encode(obj->remainStrPhone(), &baseResult);
	msgDetail.append(baseResult).append(",");

	msgDetail.append(obj->remainSpgateNo()).append(",");

	msgDetail.append(obj->remainExNo()).append(",");

	msgDetail.append(obj->remainSendTime()).append(",");
	
	msgDetail.append(obj->remainRecvTime()).append(",");

	memset(strBuf, 0, 32);
	sprintf(strBuf, "%d", obj->remainMsgFmt());
	msgDetail.append(strBuf).append(",");

	baseResult.assign("");
    Base64::Encode(obj->remainMsgContent(), &baseResult);
	msgDetail.append(baseResult).append("\r\n");
	return msgDetail;
}

int RemainObjMgr::transStringToMo(const char* pBuf, RemainObjPtrVect& objVect)
{
    std::string baseResult;
	size_t bufLen = 0;
	unsigned char strBuf[32] = { 0 };
	const char *pComma = NULL;
	std::string tmpString;

	RemainObjPtr reObj(new RemainObj());
	if(reObj == nullptr)
	{
		return -1423;
	}

	reObj->setRemainBitFlag(REMAIN_OBJMGR_MO_FLAG);
	
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1424;
	}
	int64_t srcMsgid = strtoul((const char *)(pBuf), NULL, 10);
	TranMsgIdI64ToChar(strBuf, srcMsgid);
	reObj->setRemainPtMsgId((const char*)(strBuf), sizeof(int64_t));

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1424;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainUserId(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1424;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
	reObj->setRemainStrPhone(baseResult);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1424;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainSpgateNo(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1424;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainExNo(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainSendTime(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1422;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
	reObj->setRemainRecvTime(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1424;
	}
	unsigned char msgFmt = static_cast<unsigned char>(atoi(pBuf));
	reObj->setRemainMsgFmt(msgFmt);

	pBuf = pComma + 1;
    const char* pLine = strFirstConstStr(pBuf, "\r\n");
    if(pLine)
    {
        bufLen = pLine - pBuf;
    }else{
        bufLen = strlen(pBuf);
    }
	tmpString.assign(pBuf, bufLen);
	baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
	reObj->setRemainMsgContent(baseResult);
	
	objVect.push_back(reObj);
	return 0;
}

std::string RemainObjMgr::transMtToString(const RemainObjPtr& obj)
{
    std::string baseResult;
	char strBuf[32] = { 0 };
	std::string msgDetail;
	msgDetail.reserve(2048);

	unsigned char ptMsgIdChar[9] = {0};
	memcpy(ptMsgIdChar, obj->remainPtMsgId().c_str(), 8);
	
	int64_t ptMsgId = TranMsgIdCharToI64(ptMsgIdChar);
	memset(strBuf, 0, 32);
	sprintf(strBuf, "%ld", ptMsgId);
	msgDetail.assign(strBuf).append(",");

	msgDetail.append(obj->remainUserId()).append(",");

	memset(strBuf, 0, 32);
	sprintf(strBuf, "%d", obj->remainPhoneCount());
	msgDetail.append(strBuf).append(",");

    baseResult.assign("");
    Base64::Encode(obj->remainStrPhone(), &baseResult);
	msgDetail.append(baseResult).append(",");

	msgDetail.append(obj->remainSpgateNo()).append(",");

	msgDetail.append(obj->remainExNo()).append(",");

    baseResult.assign("");
    Base64::Encode(obj->remainUserCustid(), &baseResult);
	msgDetail.append(baseResult).append(",");

    baseResult.assign("");
    Base64::Encode(obj->remainUserExData(), &baseResult);
	msgDetail.append(baseResult).append(",");
	
	msgDetail.append(obj->remainUserIpaddr()).append(",");

	memset(strBuf, 0, 32);
	sprintf(strBuf, "%d", obj->remainMsgFmt());
	msgDetail.append(strBuf).append(",");
	
    baseResult.assign("");
    Base64::Encode(obj->remainMsgContent(), &baseResult);
	msgDetail.append(baseResult).append("\r\n");
	return msgDetail;
}

int RemainObjMgr::transStringToMt(const char* pBuf, RemainObjPtrVect& objVect)
{
    std::string baseResult;
	size_t bufLen = 0;
	unsigned char strBuf[32] = { 0 };
	const char *pComma = NULL;
	std::string tmpString;

    DEBUG("transStringToMt %s", pBuf);
	RemainObjPtr reObj(new RemainObj());
	if(reObj == nullptr)
	{
		return -1425;
	}

	reObj->setRemainBitFlag(REMAIN_OBJMGR_MT_FLAG);
	
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	int64_t srcMsgid = strtoul((const char *)(pBuf), NULL, 10);
    DEBUG("srcMsgid %ld", srcMsgid);
	TranMsgIdI64ToChar(strBuf, srcMsgid);
	reObj->setRemainPtMsgId((const char*)(strBuf), sizeof(int64_t));

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    DEBUG("userid %s", tmpString.c_str());
	reObj->setRemainUserId(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	int phoneCount = atoi(pBuf);
    DEBUG("phoneCount %d", phoneCount);
	reObj->setRemainPhoneCount(phoneCount);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    DEBUG("phone %s : %s", tmpString.c_str(), baseResult.c_str());
	reObj->setRemainStrPhone(baseResult);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    DEBUG("setRemainSpgateNo %s", tmpString.c_str());
	reObj->setRemainSpgateNo(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    DEBUG("setRemainExNo %s", tmpString.c_str());
	reObj->setRemainExNo(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    DEBUG("setRemainUserCustid %s %s", tmpString.c_str(), baseResult.c_str());
	reObj->setRemainUserCustid(baseResult);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    DEBUG("setRemainUserExData %s :%s", tmpString.c_str(), baseResult.c_str());
	reObj->setRemainUserExData(baseResult);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	bufLen = pComma - pBuf;
	tmpString.assign(pBuf, bufLen);
    DEBUG("setRemainUserIpaddr %s", tmpString.c_str());
	reObj->setRemainUserIpaddr(tmpString);

	pBuf = pComma + 1;
	pComma = strFristConstChar(pBuf, ',');
	if(pComma == nullptr)
	{
		return -1426;
	}
	unsigned char msgFmt = static_cast<unsigned char>(atoi(pBuf));
    DEBUG("setRemainMsgFmt %s : %d", pBuf, msgFmt);
	reObj->setRemainMsgFmt(msgFmt);

	pBuf = pComma + 1;
    const char* pLine = strFirstConstStr(pBuf, "\r\n");
    if(pLine)
    {
        bufLen = pLine - pBuf;
    }else{
        bufLen = strlen(pBuf);
    }
	tmpString.assign(pBuf, bufLen);
    baseResult.assign("");
    Base64::Decode(tmpString, &baseResult);
    DEBUG("setRemainMsgContent %s : %s", tmpString.c_str(), baseResult.c_str());
	reObj->setRemainMsgContent(baseResult);
	
	objVect.push_back(reObj);
	return 0;
}

int RemainObjMgr::transStringToObject(int bigFlag, const std::string& userId, const std::string& remainData, RemainObjPtrVect& objVect)
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
							RemainMgr::instance().writeRemainFileBak(REMAIN_OBJMGR_RE_FLAG, userId, tmpStr.c_str(), tmpStr.length());//TODO
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
							RemainMgr::instance().writeRemainFileBak(REMAIN_OBJMGR_MO_FLAG, userId, tmpStr.c_str(), tmpStr.length());//TODO
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
							RemainMgr::instance().writeRemainFileBak(REMAIN_OBJMGR_MT_FLAG, userId, tmpStr.c_str(), tmpStr.length());//TODO
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
