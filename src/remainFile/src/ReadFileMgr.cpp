
#include "Incommon.h"
#include "ReadFileMgr.h"

namespace REMAIN_MGR
{

ReadFileMgr::ReadFileMgr(int gateNo, const std::string& fileName)
	:gateNo_(gateNo)
	,fileName_(fileName)
	,bakFileName_()
	,errFileName()
{
	size_t offset = 0;
	char readName[1024] = {0}, *pchar = nullptr;

	sprintf(readName, "%s", fileName.c_str());
	offset = strlen(readName);
	offset -= TAIL_OKFILE_STRLEN;//.ok
	pchar = readName + offset;
	sprintf(pchar, "_%d.bak", gateNo_);
	bakFileName_.assign(readName);

	memset(readName, 0 , 1024);
	sprintf(readName, "%s", fileName.c_str());
	offset = strlen(readName);
	offset -= TAIL_OKFILE_STRLEN;//.ok
	pchar = readName + offset;
	sprintf(pchar, "_%d.err", gateNo_);
	errFileName.assign(readName);
	
	INFO("ReadFileMgr init gateNo_ %d fileName_ %s bakFileName_ %s errFileName %s", gateNo_, fileName_.c_str(), bakFileName_.c_str(), errFileName.c_str());
}

ReadFileMgr::~ReadFileMgr()
{
	ERROR("~ReadFileMgr exit");
}

size_t ReadFileMgr::readFileLen()
{
	int ret = 0;
	size_t fileSize = 0;
	struct stat fileAttr;
	ret = ::stat(bakFileName_.c_str(), &fileAttr);
	if(ret == 0)
	{
		fileSize = static_cast<size_t>(fileAttr.st_size);
	}
	DEBUG("ReadFileMgr readFileLen %ld", fileSize);
	return fileSize;
}

int ReadFileMgr::readFromFile(std::string& readData)
{
	int ret = 0, trytime = 0;
	size_t nTrueRead = 0, fileLen = 0;
	char szCurName[1024] = {0}, szBakName[1024] = {0};

	snprintf(szCurName, sizeof(szCurName), "%s", fileName_.c_str());
	snprintf(szBakName, sizeof(szBakName), "%s", bakFileName_.c_str());
	DEBUG("ReadFileMgr readFromFile szCurName %s szBakName %s", szCurName, szBakName);
	
	ret = renameFile(szCurName, szBakName);
	DEBUG("ReadFileMgr readFromFile renameFile %d", ret);
	if(ret)
	{
		WARN("ReadFileMgr readFromFile renameFile %d::%d", ret, errno);
		ret = -111;
		return ret;
	}

	fileLen = readFileLen();
	DEBUG("ReadFileMgr readFromFile readFileLen %ld", fileLen);
	if(fileLen == 0)
	{
		WARN("ReadFileMgr readFromFile readFileLen %ld::%d", fileLen, errno);
		ret = -112;
		renameErrorFile();
		return ret;
	}
	
	FILE* pFile = fopen(szBakName, "rb");
	if(pFile == nullptr)
	{
		WARN("ReadFileMgr readFromFile readFileLen %ld::%d", fileLen, errno);
		renameFile(szBakName, szCurName);
		ret = -113;
		return ret;
	}

	void *readBuf = malloc(fileLen);
	if(readBuf == nullptr)
	{
		WARN("ReadFileMgr readFromFile malloc nullptr");
        fclose(pFile);
		renameFile(szBakName, szCurName);
		ret = -114;
		return ret;
	}

	do{
		size_t tmpLen = 0;
		char* tmpBuf = static_cast<char*>(readBuf);
		tmpBuf += nTrueRead;
		tmpLen = fread(tmpBuf, sizeof(char), fileLen, pFile);
		nTrueRead += tmpLen;
		fileLen -= tmpLen;
		trytime++;
		if(trytime >= 3)
		{
			break;
		}
	}while(fileLen > 0);
	DEBUG("ReadFileMgr readFromFile fileLen %ld", fileLen);
	if(fileLen > 0)
	{
        fclose(pFile);
		renameErrorFile();
		ret = -114;
		free(readBuf);
		return ret;
	}

	readData.assign(static_cast<const char*>(readBuf), nTrueRead);
	ret = 0;
	free(readBuf);
    fclose(pFile);
	deleteReadFile();
	return ret;
}

void ReadFileMgr::deleteReadFile()
{
	char filePath[1024] = {0};

	memcpy(filePath, bakFileName_.c_str(), bakFileName_.length());

	if(access(filePath, F_OK) == 0 )
	{
		remove(filePath);
	}
}

void ReadFileMgr::renameErrorFile()
{
	char szCurName[1024] = {0}, szBakName[1024] = {0};

	snprintf(szCurName, sizeof(szCurName), "%s", bakFileName_.c_str());
	snprintf(szBakName, sizeof(szBakName), "%s", errFileName.c_str());
	DEBUG("ReadFileMgr renameErrorFile szCurName %s szBakName %s", szCurName, szBakName);

	renameFile(szCurName, szBakName);
}

}

