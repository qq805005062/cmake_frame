
#include "Incommon.h"
#include "WriteFileMgr.h"

namespace REMAIN_MGR
{

//fragFolder根目录下分文件夹的文件夹名称，传统是使用用户名分隔
ShareWriteFileMgr::ShareWriteFileMgr(int gateNo, int maxCout, int outSecond, int flag, const std::string& rootPath, const std::string& fragFolder)
    :isExit_(0)
    ,gateNo_(gateNo)
    ,seqId_(0)
    ,millSecond_(0)
    ,outSecond_(outSecond)
    ,maxCount_(maxCout)
    ,curCount_(0)
    ,creatSecond_(0)
    ,pFile_(nullptr)
    ,rootPath_(rootPath)
    ,sharePath_()
    ,fragFolder_(fragFolder)
    ,mutex_()
{
    if(!rootPath.empty())
    {
        char filePath[1024] = {0};
        switch(flag)
        {
            case RE_REMAIN_FLAG:
                {
                    snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), reportDir.c_str(), fragFolder.c_str());
                    sharePath_.assign(filePath);
                    break;
                }
            case RE_REMAIN_BAK_FLAG:
                {
                    snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), reportBakDir.c_str(), fragFolder.c_str());
                    sharePath_.assign(filePath);
                    break;
                }
            case MO_REMAIN_FLAG:
                {
                    snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), moDir.c_str(), fragFolder.c_str());
                    sharePath_.assign(filePath);
                    break;
                }
            case MO_REMAIN_BAK_FLAG:
                {
                    snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), moBakDir.c_str(), fragFolder.c_str());
                    sharePath_.assign(filePath);
                    break;
                }
            case MT_REMAIN_FLAG:
                {
                    snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), mtDir.c_str(), fragFolder.c_str());
                    sharePath_.assign(filePath);
                    break;
                }
            case MT_REMAIN_BAK_FLAG:
                {
                    snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), mtBak.c_str(), fragFolder.c_str());
                    sharePath_.assign(filePath);
                    break;
                }
            default:
                {
                    ERROR("ShareWriteFileMgr init flag %d", flag);
                    break;
                }
        }
    }
                DEBUG("ShareWriteFileMgr init rootPath_ %s sharePath_ %s", rootPath_.c_str(), sharePath_.c_str());
}

ShareWriteFileMgr::~ShareWriteFileMgr()
{
	ERROR("~ShareWriteFileMgr exit");
	exitClose();
}

void ShareWriteFileMgr::exitClose()
{
	isExit_ = 1;
	DEBUG("ShareWriteFileMgr exitClose");
	std::lock_guard<std::mutex> lock(mutex_);
	if(pFile_)
	{
		CloseCurFile();
		pFile_ = nullptr;
	}
}

int ShareWriteFileMgr::writeToFile(const char* content, size_t size)
{
	size_t wriSize = 0;
	int ret = 0, tryTime = 0;
	
	DEBUG("ShareWriteFileMgr writeToFile %ld", size);
	if(rootPath_.empty() || sharePath_.empty())
	{
		WARN("ShareWriteFileMgr writeToFile rootPath_ %s sharePath_ %s", rootPath_.c_str(), sharePath_.c_str());
		return -1050;
	}
	std::lock_guard<std::mutex> lock(mutex_);
	
	if(isExit_)
	{
		WARN("ShareWriteFileMgr writeToFile exit flag had been set");
		return -101;
	}

	if(pFile_ == nullptr || (curCount_ >= maxCount_))
	{
		ret = switchToNewFile();
		DEBUG("ShareWriteFileMgr switchToNewFile ret %d", ret);
		if(ret < 0)
		{
			WARN("ShareWriteFileMgr switchToNewFile ret %d", ret);
			return ret;
		}
	}else{
		ret = accessPath(rootPath_.c_str());
		DEBUG("ShareWriteFileMgr accessPath ret %d", ret);
		if(ret < 0)
		{
			WARN("ShareWriteFileMgr rootpath no exist net work will lost");
			return -102;
		}
	}
	if(pFile_ == nullptr)
	{
		WARN("ShareWriteFileMgr writeToFile switchToNewFile nullptr");
		return -103;
	}

	do{
		size_t tmpSize = 0;
		const char* tmpChar = content + wriSize;
		tmpSize = fwrite(tmpChar, sizeof(char), size, pFile_);
		wriSize += tmpSize;
		size -= tmpSize;
		tryTime++;
		if(tryTime >= 3)
		{
			WARN("ShareWriteFileMgr writeToFile tryTime %d wriSize %ld", tryTime, wriSize);
			break;
		}
		DEBUG("ShareWriteFileMgr writeToFile error no write %ld", size);
	}while(size > 0);

	if(size > 0)
	{
		ERROR("ShareWriteFileMgr writeToFile error no write %ld", size);
		return -104;
	}else{
		if (EOF == fflush(pFile_))
		{
			ret = switchToNewFile();
			INFO("localWriteFileMgr switchToNewFile ret %d", ret);
			if(ret < 0)
			{
				return ret;
			}
			pFile_ = nullptr;
			ret = -1036;
			return ret;
		}
	}

	DEBUG("ShareWriteFileMgr writeToFile wriSize %ld size %ld", wriSize, size);
	curCount_++;
	ret = 0;
	return ret;
}

void ShareWriteFileMgr::everySecondCheck(uint64_t second)
{
    if(isExit_)
    {
        return;
    }
	std::lock_guard<std::mutex> lock(mutex_);
	
	uint64_t diffSecond = second - creatSecond_;
	int tmpSecond = static_cast<int>(diffSecond);
	//DEBUG("ShareWriteFileMgr everySecondCheck second %ld creatSecond_ %ld tmpSecond %d outSecond_ %d", second, creatSecond_, tmpSecond, outSecond_);
	if(pFile_ && (creatSecond_ > 0) && (tmpSecond > outSecond_))
	{
		CloseCurFile();
	}

	return;
}

void ShareWriteFileMgr::updateGenFreq(int second)
{
    second < 1 ? second = 60 : 1;
    second > 1800 ? second = 60 : 1;
    if(outSecond_ != second)
    {
        outSecond_ = second;
    }
}

void ShareWriteFileMgr::updateMaxFileSize(int size)
{
    size < 1 ? size = 20 : 1;
    size > 1000 ? size = 20 : 1;

    if(maxCount_ != size)
    {
        maxCount_ = size;
    }
}

int ShareWriteFileMgr::switchToNewFile()
{
	int ret = 0;
	char filePath[1024] = {0};

	ret = CloseCurFile();
	DEBUG("ShareWriteFileMgr switchToNewFile CloseCurFile %d", ret);
	if(ret < 0)
	{
		ERROR("ShareWriteFileMgr switchToNewFile CloseCurFile ret %d", ret);
		return ret;
	}
	
	size_t len = snprintf(filePath, sizeof(filePath), "%s", sharePath_.c_str());
	if(len >= 1024)
	{
		ERROR("ShareWriteFileMgr switchToNewFile too long");
		return -105;
	}
	if (mkdirs(filePath) == -1)
	{
		ERROR("ShareWriteFileMgr switchToNewFile mkdirs error");
		return -106;
	}

	seqId_++;
	if(seqId_ < 1 || seqId_ > 99999)
	{
		seqId_ = 1;
	}
	
	struct timeval tv;
 	gettimeofday(&tv, NULL);
	struct tm tm_time;
	creatSecond_ = tv.tv_sec;
	localtime_r(&(tv.tv_sec), &tm_time);

	millSecond_ = static_cast<int>(tv.tv_usec / 1000);

	memset(filePath, 0 ,1024);
	//WGNO_+条数_MMDDHHMMSSmmm+自增ID
	len = snprintf(filePath, sizeof(filePath), "%s/%d_%d_%02d%02d%02d%02d%02d%03d%05d.tmp", sharePath_.c_str(), gateNo_, 0, (tm_time.tm_mon + 1),
		tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millSecond_, seqId_);
	DEBUG("ShareWriteFileMgr switchToNewFile %s %ld", filePath, creatSecond_);
	if(len >= 1024)
	{
		ERROR("ShareWriteFileMgr path too long");
		return -107;
	}

	pFile_ = fopen(filePath, "ab+");
	if(pFile_)
	{
		curCount_ = 0;	
		return 0;
	}
	return -108;
}

int ShareWriteFileMgr::CloseCurFile()
{
	int ret = 0;
	char szCurName[1024] = {0}, szOkName[1024] = {0};

	DEBUG("ShareWriteFileMgr CloseCurFile");
	if (pFile_ == nullptr)
	{
		INFO("CloseCurFile file already close");
		return 0 ;
	}

	ret = accessPath(rootPath_.c_str());
	if(ret < 0)
	{
		ERROR("ShareWriteFileMgr rootpath no exist");
		return -102;
	}
	
	time_t tv_sec = static_cast<time_t>(creatSecond_);
	struct tm tm_time;
	localtime_r(&(tv_sec), &tm_time);

	snprintf(szCurName, sizeof(szCurName), "%s/%d_%d_%02d%02d%02d%02d%02d%03d%05d.tmp", sharePath_.c_str(), gateNo_, 0, (tm_time.tm_mon + 1),
		tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millSecond_, seqId_);

	snprintf(szOkName, sizeof(szOkName), "%s/%d_%d_%02d%02d%02d%02d%02d%03d%05d.ok", sharePath_.c_str(), gateNo_, curCount_, (tm_time.tm_mon + 1),
		tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millSecond_, seqId_);
	DEBUG("ShareWriteFileMgr CloseCurFile szCurName %s szOkName %s", szCurName, szOkName);
	
	ret = fflush(pFile_);
	if(ret < 0)
	{
		ERROR("ShareWriteFileMgr CloseCurFile fflush %d", ret);
		ret = -109;
		return ret;
	}
	
	ret = fclose(pFile_);
	if(ret < 0)
	{
		ERROR("ShareWriteFileMgr CloseCurFile fclose %d", ret);
		ret = -1010;
		return ret;
	}
	pFile_ = NULL;

	ret = renameFile(szCurName, szOkName);
	if(ret)
	{
		ERROR("ShareWriteFileMgr CloseCurFile  renameFile %d %d", ret, errno);
		ret = -1011;
	}
	
	creatSecond_ = 0;
	millSecond_ = 0;
	curCount_ = 0;

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//fragFolder根目录下分文件夹的文件夹名称，传统是使用用户名分隔
localWriteFileMgr::localWriteFileMgr(int gateNo, int maxCout, int outSecond, int flag, const std::string& rootPath, const std::string& fragFolder)
    :isExit_(0)
    ,gateNo_(gateNo)
    ,seqId_(0)
    ,millSecond_(0)
    ,outSecond_(outSecond)
    ,maxCount_(maxCout)
    ,curCount_(0)
    ,creatSecond_(0)
    ,pFile_(nullptr)
    ,rootPath_(rootPath)
    ,localPath_()
    ,fragFolder_(fragFolder)
    ,mutex_()
{
    char filePath[1024] = {0};
    switch(flag)
    {
        case RE_REMAIN_FLAG:
            {
                snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), reportDir.c_str(), fragFolder.c_str());
                localPath_.assign(filePath);
                break;
            }
        case RE_REMAIN_BAK_FLAG:
            {
                snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), reportBakDir.c_str(), fragFolder.c_str());
                localPath_.assign(filePath);
                break;
            }
        case MO_REMAIN_FLAG:
            {
                snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), moDir.c_str(), fragFolder.c_str());
                localPath_.assign(filePath);
                break;
            }
        case MO_REMAIN_BAK_FLAG:
            {
                snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), moBakDir.c_str(), fragFolder.c_str());
                localPath_.assign(filePath);
                break;
            }
        case MT_REMAIN_FLAG:
            {
                snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), mtDir.c_str(), fragFolder.c_str());
                localPath_.assign(filePath);
                break;
            }
        case MT_REMAIN_BAK_FLAG:
            {
                snprintf(filePath, sizeof(filePath), "%s/%s/%s", rootPath_.c_str(), mtBak.c_str(), fragFolder.c_str());
                localPath_.assign(filePath);
                break;
            }
        default:
            {
                ERROR("localWriteFileMgr init flag %d", flag);
                break;
            }
    }
    DEBUG("localWriteFileMgr init rootPath_ %s localPath_ %s", rootPath_.c_str(), localPath_.c_str());
}

localWriteFileMgr::~localWriteFileMgr()
{
    ERROR("~localWriteFileMgr exit");
    exitClose();
}

void localWriteFileMgr::exitClose()
{
    isExit_ = 1;
    DEBUG("localWriteFileMgr exitClose");
    std::lock_guard<std::mutex> lock(mutex_);
    if(pFile_)
    {
        CloseCurFile();
        pFile_ = nullptr;
    }
}

int localWriteFileMgr::writeToFile(const char* content, size_t size)
{
	size_t wriSize = 0;
	int ret = 0, tryTime = 0;

	DEBUG("localWriteFileMgr writeToFile %ld", size);
	std::lock_guard<std::mutex> lock(mutex_);
	if(isExit_)
	{
		WARN("localWriteFileMgr writeToFile exit flag had been set");
		return -1012;
	}

	if(pFile_ == nullptr || (curCount_ >= maxCount_))
	{
		ret = switchToNewFile();
		DEBUG("localWriteFileMgr switchToNewFile ret %d", ret);
		if(ret < 0)
		{
			return ret;
		}
	}
	if(pFile_ == nullptr)
	{
		WARN("localWriteFileMgr writeToFile switchToNewFile nullptr");
		return -1014;
	}

	do{
		size_t tmpSize = 0;
		const char* tmpChar = content + wriSize;
		tmpSize = fwrite(tmpChar, sizeof(char), size, pFile_);
		wriSize += tmpSize;
		size -= tmpSize;
		tryTime++;
		if(tryTime >= 3)
		{
			WARN("localWriteFileMgr writeToFile tryTime %d wriSize %ld", tryTime, wriSize);
			break;
		}
		DEBUG("localWriteFileMgr writeToFile error no write %ld", size);
	}while(size > 0);

	if(size > 0)
	{
		ERROR("localWriteFileMgr writeToFile error no write %ld", size);
		return -1015;
	}else{
		if (EOF == fflush(pFile_))
		{
			ret = switchToNewFile();
			INFO("localWriteFileMgr switchToNewFile ret %d", ret);
			if(ret < 0)
			{
				return ret;
			}
			pFile_ = nullptr;
			ret = -1036;
			return ret;
		}
	}

	DEBUG("localWriteFileMgr writeToFile wriSize %ld size %ld", wriSize, size);
	curCount_++;
	ret = 0;
	return ret;
}

void localWriteFileMgr::everySecondCheck(uint64_t second)
{
    if(isExit_)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t diffSecond = second - creatSecond_;
    int tmpSecond = static_cast<int>(diffSecond);
    //DEBUG("localWriteFileMgr everySecondCheck second %ld creatSecond_ %ld tmpSecond %d outSecond_ %d", second, creatSecond_, tmpSecond, outSecond_);
    if(pFile_ && (creatSecond_ > 0) && (tmpSecond > outSecond_))
    {
        CloseCurFile();
    }

    return;
}

void localWriteFileMgr::updateGenFreq(int second)
{
    second < 1 ? second = 60 : 1;
    second > 1800 ? second = 60 : 1;
    if(outSecond_ != second)
    {
        outSecond_ = second;
    }
}

void localWriteFileMgr::updateMaxFileSize(int size)
{
    size < 1 ? size = 20 : 1;
    size > 1000 ? size = 20 : 1;

    if(maxCount_ != size)
    {
        maxCount_ = size;
    }
}

int localWriteFileMgr::switchToNewFile()
{
	int ret = 0;
	char filePath[1024] = {0};

	ret = CloseCurFile();
	DEBUG("localWriteFileMgr switchToNewFile CloseCurFile %d", ret);
	if(ret < 0)
	{
		ERROR("localWriteFileMgr switchToNewFile CloseCurFile ret %d", ret);
		return ret;
	}
	
	size_t len = snprintf(filePath, sizeof(filePath), "%s", localPath_.c_str());
	if(len >= 1024)
	{
		ERROR("localWriteFileMgr switchToNewFile too long");
		return -1016;
	}
	if (mkdirs(filePath) == -1)
	{
		ERROR("localWriteFileMgr switchToNewFile mkdirs error");
		return -1017;
	}

	seqId_++;
	if(seqId_ < 1 || seqId_ > 99999)
	{
		seqId_ = 1;
	}
	
	struct timeval tv;
 	gettimeofday(&tv, NULL);
	struct tm tm_time;
	creatSecond_ = tv.tv_sec;
	localtime_r(&(tv.tv_sec), &tm_time);

	millSecond_ = static_cast<int>(tv.tv_usec / 1000);
	
	memset(filePath, 0 ,1024);
	//WGNO_+条数_MMDDHHMMSSmmm+自增ID
	len = snprintf(filePath, sizeof(filePath), "%s/%d_%d_%02d%02d%02d%02d%02d%03d%05d.tmp", localPath_.c_str(), gateNo_, 0, (tm_time.tm_mon + 1),
		tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millSecond_, seqId_);
	DEBUG("localWriteFileMgr switchToNewFile %s %ld", filePath, creatSecond_);
	if(len >= 1024)
	{
		ERROR("localWriteFileMgr path too len");
		return -1018;
	}

	pFile_ = fopen(filePath, "ab+");
	if(pFile_)
	{
		curCount_ = 0;
		return 0;
	}
	
	return -1019;
}

int localWriteFileMgr::CloseCurFile()
{
	int ret = 0;
	char szCurName[1024] = {0}, szOkName[1024] = {0};

	DEBUG("localWriteFileMgr CloseCurFile");
	if (pFile_ == nullptr)
	{
		INFO("localWriteFileMgr CloseCurFile file already close");
		return 0;
	}
	
	time_t tv_sec = static_cast<time_t>(creatSecond_);
	struct tm tm_time;
	localtime_r(&(tv_sec), &tm_time);

	snprintf(szCurName, sizeof(szCurName), "%s/%d_%d_%02d%02d%02d%02d%02d%03d%05d.tmp", localPath_.c_str(), gateNo_, 0, (tm_time.tm_mon + 1),
		tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millSecond_, seqId_);

	snprintf(szOkName, sizeof(szOkName), "%s/%d_%d_%02d%02d%02d%02d%02d%03d%05d.ok", localPath_.c_str(), gateNo_, curCount_, (tm_time.tm_mon + 1),
		tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millSecond_, seqId_);

	DEBUG("localWriteFileMgr CloseCurFile szCurName %s szOkName %s", szCurName, szOkName);
	ret = fflush(pFile_);
	DEBUG("localWriteFileMgr CloseCurFile fflush %d", ret);
	if(ret < 0)
	{
		ERROR("localWriteFileMgr fflush %d", ret);
		ret = -1020;
	}
	ret = fclose(pFile_);
	DEBUG("localWriteFileMgr CloseCurFile fclose %d", ret);
	if(ret < 0)
	{
		ERROR("localWriteFileMgr fclose %d", ret);
		ret = -1021;
	}
	pFile_ = NULL;

	ret = renameFile(szCurName, szOkName);
	DEBUG("localWriteFileMgr CloseCurFile renameFile %d %d", ret, errno);
	if(ret)
	{
		ERROR("localWriteFileMgr renameFile %d", ret);
		ret = -1022;
	}
	
	creatSecond_ = 0;
	millSecond_ = 0;
	curCount_ = 0;
	
	return ret;
}

//fragFolder根目录下分文件夹的文件夹名称，传统是使用用户名分隔
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WriteFileMgr::WriteFileMgr(int gateNo, int maxCout, int outSecond, const std::string& sharePath, const std::string localPath, const std::string& fragFolder)
    :isExit_(0)
    ,gateNo_(gateNo)
    ,maxCount_(maxCout)
    ,outSecond_(outSecond)
    ,localPath_(localPath)
    ,sharePath_(sharePath)
    ,fragFolder_(fragFolder)
    ,shareReFile(nullptr)
    ,shareReBakFile(nullptr)
    ,shareMoFile(nullptr)
    ,shareMoBakFile(nullptr)
    ,shareMtFile(nullptr)
    ,shareMtBakFile(nullptr)
    ,localReFile(nullptr)
    ,localReBakFile(nullptr)
    ,localMoFile(nullptr)
    ,localMoBakFile(nullptr)
    ,localMtFile(nullptr)
    ,localMtBakFile(nullptr)
{
}

WriteFileMgr::~WriteFileMgr()
{
    exitClose();
}

int WriteFileMgr::writeToFile(int flag, const char* content, size_t size)
{
	int ret = 0;
	switch(flag)
	{
		case RE_REMAIN_FLAG:
			{
				if(shareReFile == nullptr)
				{
					shareReFile.reset(new ShareWriteFileMgr(gateNo_, maxCount_, outSecond_, RE_REMAIN_FLAG, sharePath_, fragFolder_));
				}
				
				if(shareReFile == nullptr)
				{
					WARN("WriteFileMgr writeToFile shareReFile nullptr");
					ret = -1023;
				}else{
					ret = shareReFile->writeToFile(content, size);
					DEBUG("WriteFileMgr writeToFile shareReFile->writeToFile ret %d", ret);
					if(ret < 0)
					{
						if(localReFile == nullptr)
						{
							localReFile.reset(new localWriteFileMgr(gateNo_, maxCount_, outSecond_, RE_REMAIN_FLAG, localPath_, fragFolder_));
						}

						if(localReFile == nullptr)
						{
							WARN("WriteFileMgr writeToFile localReFile nullptr");
							ret = -1024;
						}else{
							ret = localReFile->writeToFile(content, size);
							DEBUG("WriteFileMgr writeToFile localReFile->writeToFile ret %d", ret);
						}
					}
				}
				break;
			}
		case RE_REMAIN_BAK_FLAG:
			{
				if(shareReBakFile == nullptr)
				{
					shareReBakFile.reset(new ShareWriteFileMgr(gateNo_, maxCount_, outSecond_, RE_REMAIN_BAK_FLAG, sharePath_, fragFolder_));
				}
				
				if(shareReBakFile == nullptr)
				{
					WARN("WriteFileMgr writeToFile shareReBakFile nullptr");
					ret = -1025;
				}else{
					ret = shareReBakFile->writeToFile(content, size);
					DEBUG("WriteFileMgr writeToFile shareReBakFile->writeToFile ret %d", ret);
					if(ret < 0)
					{
						if(localReBakFile == nullptr)
						{
							localReBakFile.reset(new localWriteFileMgr(gateNo_, maxCount_, outSecond_, RE_REMAIN_BAK_FLAG, localPath_, fragFolder_));
						}

						if(localReBakFile == nullptr)
						{
							WARN("WriteFileMgr writeToFile localReBakFile nullptr");
							ret = -1026;
						}else{
							ret = localReBakFile->writeToFile(content, size);
							DEBUG("WriteFileMgr writeToFile localReBakFile->writeToFile ret %d", ret);
						}
					}
				}
				break;
			}
		case MO_REMAIN_FLAG:
			{
				if(shareMoFile == nullptr)
				{
					shareMoFile.reset(new ShareWriteFileMgr(gateNo_, maxCount_, outSecond_, MO_REMAIN_FLAG, sharePath_, fragFolder_));
				}
				
				if(shareMoFile == nullptr)
				{
					WARN("WriteFileMgr writeToFile shareMoFile nullptr");
					ret = -1027;
				}else{
					ret = shareMoFile->writeToFile(content, size);
					DEBUG("WriteFileMgr writeToFile shareMoFile->writeToFile ret %d", ret);
					if(ret < 0)
					{
						if(localMoFile == nullptr)
						{
							localMoFile.reset(new localWriteFileMgr(gateNo_, maxCount_, outSecond_, MO_REMAIN_FLAG, localPath_, fragFolder_));
						}

						if(localMoFile == nullptr)
						{
							WARN("WriteFileMgr writeToFile localMoFile nullptr");
							ret = -1028;
						}else{
							ret = localMoFile->writeToFile(content, size);
							DEBUG("WriteFileMgr writeToFile localMoFile->writeToFile ret %d", ret);
						}
					}
				}
				break;
			}
		case MO_REMAIN_BAK_FLAG:
			{
				if(shareMoBakFile == nullptr)
				{
					shareMoBakFile.reset(new ShareWriteFileMgr(gateNo_, maxCount_, outSecond_, MO_REMAIN_BAK_FLAG, sharePath_, fragFolder_));
				}
				
				if(shareMoBakFile == nullptr)
				{
					WARN("WriteFileMgr writeToFile shareMoBakFile nullptr");
					ret = -1029;
				}else{
					ret = shareMoBakFile->writeToFile(content, size);
					DEBUG("WriteFileMgr writeToFile shareMoBakFile->writeToFile ret %d", ret);
					if(ret < 0)
					{
						if(localMoBakFile == nullptr)
						{
							localMoBakFile.reset(new localWriteFileMgr(gateNo_, maxCount_, outSecond_, MO_REMAIN_BAK_FLAG, localPath_, fragFolder_));
						}

						if(localMoBakFile == nullptr)
						{
							WARN("WriteFileMgr writeToFile localMoBakFile nullptr");
							ret = -1030;
						}else{
							ret = localMoBakFile->writeToFile(content, size);
							DEBUG("WriteFileMgr writeToFile localMoBakFile->writeToFile ret %d", ret);
						}
					}
				}
				break;
			}
		case MT_REMAIN_FLAG:
			{
				if(shareMtFile == nullptr)
				{
					shareMtFile.reset(new ShareWriteFileMgr(gateNo_, maxCount_, outSecond_, MT_REMAIN_FLAG, sharePath_, fragFolder_));
				}
				
				if(shareMtFile == nullptr)
				{
					WARN("WriteFileMgr writeToFile shareMtFile nullptr");
					ret = -1031;
				}else{
					ret = shareMtFile->writeToFile(content, size);
					DEBUG("WriteFileMgr writeToFile shareMtFile->writeToFile ret %d", ret);
					if(ret < 0)
					{
						if(localMtFile == nullptr)
						{
							localMtFile.reset(new localWriteFileMgr(gateNo_, maxCount_, outSecond_, MT_REMAIN_FLAG, localPath_, fragFolder_));
						}

						if(localMtFile == nullptr)
						{
							WARN("WriteFileMgr writeToFile localMtFile nullptr");
							ret = -1032;
						}else{
							ret = localMtFile->writeToFile(content, size);
							DEBUG("WriteFileMgr writeToFile localMtFile->writeToFile ret %d", ret);
						}
					}
				}
				break;
			}
		case MT_REMAIN_BAK_FLAG:
			{
				if(shareMtBakFile == nullptr)
				{
					shareMtBakFile.reset(new ShareWriteFileMgr(gateNo_, maxCount_, outSecond_, MT_REMAIN_BAK_FLAG, sharePath_, fragFolder_));
				}
				
				if(shareMtBakFile == nullptr)
				{
					WARN("WriteFileMgr writeToFile shareMtBakFile nullptr");
					ret = -1033;
				}else{
					ret = shareMtBakFile->writeToFile(content, size);
					DEBUG("WriteFileMgr writeToFile shareMtBakFile->writeToFile ret %d", ret);
					if(ret < 0)
					{
						if(localMtBakFile == nullptr)
						{
							localMtBakFile.reset(new localWriteFileMgr(gateNo_, maxCount_, outSecond_, MT_REMAIN_BAK_FLAG, localPath_, fragFolder_));
						}

						if(localMtBakFile == nullptr)
						{
							WARN("WriteFileMgr writeToFile localMtBakFile nullptr");
							ret = -1034;
						}else{
							ret = localMtBakFile->writeToFile(content, size);
							DEBUG("WriteFileMgr writeToFile localMtBakFile->writeToFile ret %d", ret);
						}
					}
				}
				break;
			}
		default:
			{
				WARN("WriteFileMgr writeToFile flag %d", flag);
				ret = -1035;
				break;
			}
	}
	return ret;
}

void WriteFileMgr::exitClose()
{
	INFO("WriteFileMgr exitClose");
	if(shareReFile)
	{
		DEBUG("WriteFileMgr exitClose shareReFile");
		shareReFile.reset();
	}
	if(shareReBakFile)
	{
		DEBUG("WriteFileMgr exitClose shareReBakFile");
		shareReBakFile.reset();
	}
	if(shareMoFile)
	{
		DEBUG("WriteFileMgr exitClose shareMoFile");
		shareMoFile.reset();
	}
	if(shareMoBakFile)
	{
		DEBUG("WriteFileMgr exitClose shareMoBakFile");
		shareMoBakFile.reset();
	}
	if(shareMtFile)
	{
		DEBUG("WriteFileMgr exitClose shareMtFile");
		shareMtFile.reset();
	}
	if(shareMtBakFile)
	{
		DEBUG("WriteFileMgr exitClose shareMtBakFile");
		shareMtBakFile.reset();
	}
	if(localReFile)
	{
		DEBUG("WriteFileMgr exitClose localReFile");
		localReFile.reset();
	}
	if(localReBakFile)
	{
		DEBUG("WriteFileMgr exitClose localReBakFile");
		localReBakFile.reset();
	}
	if(localMoFile)
	{
		DEBUG("WriteFileMgr exitClose localMoFile");
		localMoFile.reset();
	}
	if(localMoBakFile)
	{
		DEBUG("WriteFileMgr exitClose localMoBakFile");
		localMoBakFile.reset();
	}
	if(localMtFile)
	{
		DEBUG("WriteFileMgr exitClose localMtFile");
		localMtFile.reset();
	}
	if(localMtBakFile)
	{
		DEBUG("WriteFileMgr exitClose localMtBakFile");
		localMtBakFile.reset();
	}
}

void WriteFileMgr::everySecondCheck(uint64_t second)
{
    if(shareReFile)
    {
        shareReFile->everySecondCheck(second);
    }
    if(shareReBakFile)
    {
        shareReBakFile->everySecondCheck(second);
    }
    if(shareMoFile)
    {
        shareMoFile->everySecondCheck(second);
    }
    if(shareMoBakFile)
    {
        shareMoBakFile->everySecondCheck(second);
    }
    if(shareMtFile)
    {
        shareMtFile->everySecondCheck(second);
    }
    if(shareMtBakFile)
    {
        shareMtBakFile->everySecondCheck(second);
    }
    if(localReFile)
    {
        localReFile->everySecondCheck(second);
    }
    if(localReBakFile)
    {
        localReBakFile->everySecondCheck(second);
    }
    if(localMoFile)
    {
        localMoFile->everySecondCheck(second);
    }
    if(localMoBakFile)
    {
        localMoBakFile->everySecondCheck(second);
    }
    if(localMtFile)
    {
        localMtFile->everySecondCheck(second);
    }
    if(localMtBakFile)
    {
        localMtBakFile->everySecondCheck(second);
    }
}

void WriteFileMgr::updateGenFreq(int second)
{
    if(shareReFile)
    {
        shareReFile->updateGenFreq(second);
    }
    if(shareReBakFile)
    {
        shareReBakFile->updateGenFreq(second);
    }
    if(shareMoFile)
    {
        shareMoFile->updateGenFreq(second);
    }
    if(shareMoBakFile)
    {
        shareMoBakFile->updateGenFreq(second);
    }
    if(shareMtFile)
    {
        shareMtFile->updateGenFreq(second);
    }
    if(shareMtBakFile)
    {
        shareMtBakFile->updateGenFreq(second);
    }
    if(localReFile)
    {
        localReFile->updateGenFreq(second);
    }
    if(localReBakFile)
    {
        localReBakFile->updateGenFreq(second);
    }
    if(localMoFile)
    {
        localMoFile->updateGenFreq(second);
    }
    if(localMoBakFile)
    {
        localMoBakFile->updateGenFreq(second);
    }
    if(localMtFile)
    {
        localMtFile->updateGenFreq(second);
    }
    if(localMtBakFile)
    {
        localMtBakFile->updateGenFreq(second);
    }
}

void WriteFileMgr::updateMaxFileSize(int size)
{
    if(shareReFile)
    {
        shareReFile->updateMaxFileSize(size);
    }
    if(shareReBakFile)
    {
        shareReBakFile->updateMaxFileSize(size);
    }
    if(shareMoFile)
    {
        shareMoFile->updateMaxFileSize(size);
    }
    if(shareMoBakFile)
    {
        shareMoBakFile->updateMaxFileSize(size);
    }
    if(shareMtFile)
    {
        shareMtFile->updateMaxFileSize(size);
    }
    if(shareMtBakFile)
    {
        shareMtBakFile->updateMaxFileSize(size);
    }
    if(localReFile)
    {
        localReFile->updateMaxFileSize(size);
    }
    if(localReBakFile)
    {
        localReBakFile->updateMaxFileSize(size);
    }
    if(localMoFile)
    {
        localMoFile->updateMaxFileSize(size);
    }
    if(localMoBakFile)
    {
        localMoBakFile->updateMaxFileSize(size);
    }
    if(localMtFile)
    {
        localMtFile->updateMaxFileSize(size);
    }
    if(localMtBakFile)
    {
        localMtBakFile->updateMaxFileSize(size);
    }
}

}


