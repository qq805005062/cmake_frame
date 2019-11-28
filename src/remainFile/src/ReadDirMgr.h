#ifndef __REMAIN_MGR_READDIR_H__
#define __REMAIN_MGR_READDIR_H__

#include <memory>

namespace REMAIN_MGR
{

class ReadDirMgr
{
public:
    //读取目录类，
    //第一参数是决定读取那个类的滞留文件夹，优先读本地，后读共享文件夹(使用上层头文件中的宏定义区分
    //第二个参数是对应的网关编号
    //第三个参数是读取那个用户名下面的滞留
    //第四个参数是共享文件夹路径
    //第五个参数是本地文件夹路径
    ReadDirMgr(int flag, int gateNo, const std::string& fragFolder, const std::string& sharePath, const std::string& localPath);

    //析构类
    ~ReadDirMgr();

    //读取相关数据，内部使用文件读取类读取具体内容
    int readReaminData(std::string& data);

private:

    int readFileFromDir(const std::string& path, std::string& data);

    int bitFlag_;
    int gateNo_;

    std::string shareRootPath_;
    std::string sharePath_;
    std::string localPath_;
    std::string fragFolder_;
};

typedef std::shared_ptr<ReadDirMgr> ReadDirMgrPtr;
}
#endif