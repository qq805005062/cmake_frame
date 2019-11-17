#ifndef __CCBMSP_MESSAGE_FILES_IMPL_H__
#define __CCBMSP_MESSAGE_FILES_IMPL_H__

#include <string>
#include <vector>
#include <map>

#include "FileMgr.h"
#include "MessageFiles.h"

#include <mutex>

typedef std::map<std::string, FileMgr*> FileMgrMap;

class MessageFilesImpl : public MessageFiles
{
public:
    MessageFilesImpl();
    virtual ~MessageFilesImpl() = default;

public:
    virtual bool InitParams(const std::string& szPtCode, int gateNo);

    virtual void updateGenFreq(int freq = 60);

    virtual void updateMaxFileSize(int size = 20);
    
    virtual int  InitMtDetailsFile(const std::string& szPtCode,
                                        int gateNo,
                                        const std::string& fileType,
                                        int maxFileSize,
                                        int genFreq);

    virtual int  InitMtOkDetailsFile(const std::string& szPtCode,
                                        int gateNo,
                                        const std::string& fileType,
                                        int maxFileSize,
                                        int genFreq);

    virtual int  InitReDetailsFile(const std::string& szPtCode,
                                        int gateNo,
                                        const std::string& fileType,
                                        int maxFileSize,
                                        int genFreq);

    virtual int  InitReOkDetailsFile(const std::string& szPtCode,
                                        int gateNo,
                                        const std::string& fileType,
                                        int maxFileSize,
                                        int genFreq);

    virtual int  InitMoDetailsFile(const std::string& szPtCode,
                                        int gateNo,
                                        const std::string& fileType,
                                        int maxFileSize,
                                        int genFreq);

    virtual int  InitMoOkDetailsFile(const std::string& szPtCode,
                                        int gateNo,
                                        const std::string& fileType,
                                        int maxFileSize,
                                        int genFreq);

    virtual bool bExistFile(const std::string& fileType);
    virtual int  WriteFile(const std::string& fileType, const std::string& data);
    virtual void SwitchMessageFiles();
    virtual int  GetLongTimeNoUploadFilesCnt(int min = 10);
    virtual void CloseAll();

private:
    // 扫描没有被重命名的文件，将其重命名为_ok.txt
    void ScanfAndRenameTmpFile();

    // 扫描tmp文件
    void EnumTmpFile(const char* szFolderName, std::vector<std::string>& vFileName);

    // 扫描没有被及时上传的文件
    void EnumNoUploadFiles(const char* szFolderName, int nMin, int& nCount);

    // 扫描距当前若干天前的未上传的文件
    int  EnumNoUploadFiles(int nDaySpan, int nMin);

private:
    bool bDldFileInit_;
    std::string szMainPath_;
    int gateNo_;
    std::string szPtCode_;

    std::mutex mutex_;
    FileMgrMap details_;
};


#endif // __RDN_MESSAGE_FILES_IMPL_H__
