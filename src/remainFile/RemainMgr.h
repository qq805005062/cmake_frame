#ifndef __REMAIN_MGR_FILE_H__
#define __REMAIN_MGR_FILE_H__

#include "src/Singleton.h"
#include "src/noncopyable.h"

#define REMAIN_MGR_RE_FLAG          1
#define REMAIN_MGR_MO_FLAG          2
#define REMAIN_MGR_MT_FLAG          3

namespace REMAIN_MGR
{

/*
*标志位第一位是上行报告，第二位是上行短信，第三位是下行短信
*
*读文件优先读本地路径下的信息，
*写文件优先写共享路径下的信息
*
*/
class RemainMgr : public noncopyable
{
public:
    RemainMgr();

    ~RemainMgr();

    static RemainMgr& instance() { return Singleton<RemainMgr>::instance();}

    /*
     *模块退出接口，清除所有资源
     *
     *
     */
    void remainMgrExit();

    /*
     *第一个参数是网关编号
     *第二个参数是每个文件最多保存多少条
     *第三个参数为超时切换一个文件，单位秒钟
     *第四个参数是共享目录路径，如果为空，则不处理共享目录，路径都是根目录，根目录下的路径就是固定的了
     *第五个参数是本地目录路径，如果为空，默认是相对路径下的对应文件夹，路径都是根目录，根目录下的路径就是固定的了
     *
     *返回值，==0的时候表示正常，小于0表示错误
     */
    int remainMgrInit(uint32_t gateNo, int maxCount = 50, int outSeconds = 60, const std::string& sharePath = "", const std::string& localPath = "");

    /*
     *写滞留文件，根据bitFlag区分写入文件的类型
     *userId为用户帐户名称，
     *
     *
     *返回值，==0的时候表示正常，小于0表示错误
     */
    int writeRemainFile(int bitFlag, const std::string& userId, const char *data, size_t dataSize);

    /*
     *写滞留文件，根据bitFlag区分写入文件的类型,此接口写入滞留就再也不会读出来了，除非有人工干预，下行接口慎重
     *userId为用户帐户名称，
     *
     *
     *
     *返回值，==0的时候表示正常，小于0表示错误
     */
    int writeRemainFileBak(int bitFlag, const std::string& userId, const char *data, size_t dataSize);

    /*
     *读文件滞留，根据bitFlag区分需要读出文件类型，每次读一个完整的文件
     *userId为用户帐户名称，
     *
     *data为上层分配的空间的地址，dataSize为空间大小，data必须为new出来的数据，内部有可能会delete。
     *返回值，当返回0的时候，正常，其他都是一场错误，上层要注意判断string为空的情况
     */
    int readRemainFile(int bitFlag, const std::string& userId, std::string& remainData);

    /*
     *统计滞留数量，分别统计本地滞留数量，以及共享目录统计数量
     *
     *
     *返回值，==0的时候表示正常，小于0表示错误
     */
    int statisticsRemainCount(int bitFlag, const std::string& userId, int& localCount, int& shareCount);

    /*
     *检索文件是否超时，超时则关闭文件
     *
     *
     */
    void everySecondCheck(uint64_t second = 0);

    void updateGenFreq(int freq = 60);

    void updateMaxFileSize(int size = 20);

private:
    
    int isExit_;
    int isInit_;
    int gateNo_;
    int maxCount_;
    int outSecond_;

    std::string sharePath_;
    std::string localPath_;
};

}

#endif