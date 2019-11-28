
#include <dirent.h>
#include <sys/stat.h>

#include "Incommon.h"
#include "ReadDirMgr.h"
#include "ReadFileMgr.h"

namespace REMAIN_MGR
{

ReadDirMgr::ReadDirMgr(int flag, int gateNo, const std::string& fragFolder, const std::string& sharePath, const std::string& localPath)
    :bitFlag_(flag)
    ,gateNo_(gateNo)
    ,shareRootPath_()
    ,sharePath_()
    ,localPath_()
    ,fragFolder_(fragFolder)
{
    char shareTmpPath[1024] = {0}, localTmpPath[1024] = {0};
    if(flag == RE_REMAIN_FLAG)
    {
        if(!sharePath.empty())
        {
            shareRootPath_.assign(sharePath);
            sprintf(shareTmpPath, "%s/%s/%s", sharePath.c_str(), reportDir.c_str(), fragFolder_.c_str());
        }
        sprintf(localTmpPath, "%s/%s/%s", localPath.c_str(), reportDir.c_str(), fragFolder_.c_str());

        sharePath_.assign(shareTmpPath);
        localPath_.assign(localTmpPath);
    }else if(flag == MO_REMAIN_FLAG)
    {
        if(!sharePath.empty())
        {
            shareRootPath_.assign(sharePath);
            sprintf(shareTmpPath, "%s/%s/%s", sharePath.c_str(), moDir.c_str(), fragFolder_.c_str());
        }
        sprintf(localTmpPath, "%s/%s/%s", localPath.c_str(), moDir.c_str(), fragFolder_.c_str());
        sharePath_.assign(shareTmpPath);
        localPath_.assign(localTmpPath);
    }else if(flag == MT_REMAIN_FLAG)
    {
        if(!sharePath.empty())
        {
            shareRootPath_.assign(sharePath);
            sprintf(shareTmpPath, "%s/%s/%s", sharePath.c_str(), mtDir.c_str(), fragFolder_.c_str());
        }
        sprintf(localTmpPath, "%s/%s/%s", localPath.c_str(), mtDir.c_str(), fragFolder_.c_str());
        sharePath_.assign(shareTmpPath);
        localPath_.assign(localTmpPath);
    }else{
        ERROR("ReadDirMgr flag %d", flag);
    }
    DEBUG("ReadDirMgr init bitFlag_ %d gateNo_ %d fragFolder_ %s sharePath_ %s localPath_ %s", bitFlag_, gateNo_, fragFolder_.c_str(), sharePath_.c_str(), localPath_.c_str());
}

ReadDirMgr::~ReadDirMgr()
{
    ERROR("~ReadDirMgr exit");
}

int ReadDirMgr::readReaminData(std::string& data)
{
    int ret = 0;

    ret = readFileFromDir(localPath_, data);
    DEBUG("ReadDirMgr readReaminData %d::%s", ret, data.c_str());
    if(ret == 0)
    {
        return ret;
    }

    ret = accessPath(shareRootPath_.c_str());
    DEBUG("ReadDirMgr readReaminData ret %d", ret);
    if(ret < 0)
    {
        WARN("ReadDirMgr readReaminData share rootpath no exist net work will lost");
        return -123;
    }
    ret = readFileFromDir(sharePath_, data);
    DEBUG("ReadDirMgr readReaminData %d::%s", ret, data.c_str());
    return ret;
}

int ReadDirMgr::readFileFromDir(const std::string& path, std::string& data)
{
    char dirPath[1024] = {0};
    struct dirent* filename = NULL;
    DIR* dir = NULL;

    DEBUG("ReadDirMgr readFileFromDir %s", path.c_str());
    memcpy(dirPath, path.c_str(), path.length());
    dir = opendir(dirPath);
    if (NULL == dir)
    {
        WARN("ReadDirMgr readFileFromDir %s nullptr", dirPath);
        return - 121;
    }

    while ((filename = readdir(dir)) != NULL)
    {
        DEBUG("ReadDirMgr readFileFromDir %s", filename->d_name);
        std::string strFilename(path);
        strFilename += "/";
        strFilename += filename->d_name;

        struct stat st;
        if (0 == stat(strFilename.c_str(), &st))
        {
            //S_ISLNK(st_mode);是否是一个连接.
            //S_ISREG(st_mode);是否是一个常规文件.
            //S_ISDIR(st_mode);是否是一个目录
            //S_ISCHR(st_mode);是否是一个字符设备.
            //S_ISBLK(st_mode);是否是一个块设备
            //S_ISFIFO(st_mode);是否 是一个FIFO文件.
            //S_ISSOCK(st_mode);是否是一个SOCKET文件 
            if (!S_ISDIR(st.st_mode))
            {
                char* pTmp = endCharStr(filename->d_name);
                pTmp -= TAIL_OKFILE_STRLEN;
                INFO("readFileFromDir name %s ", strFilename.c_str());
                int ret = strcmp(pTmp, tailOkFile);
                if(ret == 0)
                {
                    ReadFileMgrPtr readFile(new ReadFileMgr(gateNo_, strFilename));
                    if(readFile)
                    {
                        ret = readFile->readFromFile(data);
                        DEBUG("readFileFromDir readFile->readFromFile %d ", ret);
                        if(ret == 0)
                        {
                            INFO("readFileFromDir readFile name %s::%s", strFilename.c_str(), data.c_str());
                            closedir(dir);
                            return ret;
                        }
                    }else{
                        WARN("ReadDirMgr readFileFromDir ReadFileMgrPtr new nullptr");
                    }
                }else{
                    WARN("ReadDirMgr readFileFromDir %s is no ok file", strFilename.c_str());
                }
            }else{
                WARN("ReadDirMgr readFileFromDir %s is dir", strFilename.c_str());
            }
        }else{
            WARN("ReadDirMgr readFileFromDir %s stat error", strFilename.c_str());
        }
    }

    WARN("ReadDirMgr readFileFromDir %s no read any data", path.c_str());
    closedir(dir);
    return -122;
}

}

