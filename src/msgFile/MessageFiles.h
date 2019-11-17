#ifndef __CCBMSP_MESSAGEFILES_H__
#define __CCBMSP_MESSAGEFILES_H__

#include <string>
#include <common/noncopyable.h>

#define MSGFILES_FOLDER_NAME            "MsgFiles"
//#define CCBMSP_MSG_DETAIL               "CCBMSP_MT_RVOK"
//#define CCBMSP_MSGOK_DETAIL             "CCBMSP_MT_SDOK"
//#define CCBMSP_RE_DETAIL                "CCBMSP_RE_RVOK"
//#define CCBMSP_REOK_DETAIL              "CCBMSP_RE_SDOK"
//#define CCBMSP_MO_DETAIL                "CCBMSP_MO_RVOK"
//#define CCBMSP_MOOK_DETAIL              "CCBMSP_MO_SDOK"


#define FILE_EXNAME                 ".txt"

class MessageFiles
{
public:
    MessageFiles() = default;
    virtual ~MessageFiles() = default;

public:
    virtual bool InitParams(const std::string& szPtCode, int gateNo) = 0;

    virtual void updateGenFreq(int freq = 60) = 0;

    virtual void updateMaxFileSize(int size = 20) = 0;
    
    virtual int  InitMtDetailsFile(const std::string& szPtCode = "UNKNOWN",
                                        int gateNo = 0,
                                        const std::string& fileType = "MT_RVOK",
                                        int maxFileSize = 5,
                                        int genFreq = 60) = 0;

    virtual int  InitMtOkDetailsFile(const std::string& szPtCode = "UNKNOWN",
                                        int gateNo = 0,
                                        const std::string& fileType = "MT_SDOK",
                                        int maxFileSize = 5,
                                        int genFreq = 60) = 0;

    virtual int  InitReDetailsFile(const std::string& szPtCode = "UNKNOWN",
                                        int gateNo = 0,
                                        const std::string& fileType = "RE_RVOK",
                                        int maxFileSize = 5,
                                        int genFreq = 60) = 0;

    virtual int  InitReOkDetailsFile(const std::string& szPtCode = "UNKNOWN",
                                        int gateNo = 0,
                                        const std::string& fileType = "RE_SDOK",
                                        int maxFileSize = 5,
                                        int genFreq = 60) = 0;

    virtual int  InitMoDetailsFile(const std::string& szPtCode = "UNKNOWN",
                                        int gateNo = 0,
                                        const std::string& fileType = "MO_RVOK",
                                        int maxFileSize = 5,
                                        int genFreq = 60) = 0;

    virtual int  InitMoOkDetailsFile(const std::string& szPtCode = "UNKNOWN",
                                        int gateNo = 0,
                                        const std::string& fileType = "MO_SDOK",
                                        int maxFileSize = 5,
                                        int genFreq = 60) = 0;

    virtual bool bExistFile(const std::string& fileType) = 0;
    virtual int  WriteFile(const std::string& fileType, const std::string& data) = 0;

    virtual void SwitchMessageFiles() = 0;
    virtual int  GetLongTimeNoUploadFilesCnt(int min = 10) = 0;
    virtual void CloseAll() = 0;
};

class MessageFilesFactory : common::noncopyable
{
public:
    static MessageFiles* New();
    static void Destroy(MessageFiles* files);
};


#endif // __RDN_MESSAGEFILES_H__
