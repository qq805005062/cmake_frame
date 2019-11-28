#ifndef __REMAIN_MGR_READFILE_H__
#define __REMAIN_MGR_READFILE_H__

#include <sys/types.h>    
#include <sys/stat.h>

namespace REMAIN_MGR
{
/*
 *捞取一个文件的类，由于可能是多进程捞取文件，可能会捞取失败
 *
 *写文件优先写共享文件，捞取文件优先捞本地文件
 *
 *
 *
 */
class ReadFileMgr
{
public:
    ReadFileMgr(int gateNo, const std::string& fileName);

    ~ReadFileMgr();

    //== 0 success ; < 0 error
    //读取一个文件的的全部内容，string返回读取到的类容
    int readFromFile(std::string& readData);

private:

    //获取一个要读文件的长度，此类每次只读一个文件，但是如果重命名失败之后就不会读取，返回失败
    size_t readFileLen();

    //删除一个读取之后的文件，文件读取之后就会被删除
    void deleteReadFile();

    void renameErrorFile();

    int gateNo_;

    std::string fileName_;
    std::string bakFileName_;
    std::string errFileName;
};

typedef std::shared_ptr<ReadFileMgr> ReadFileMgrPtr;

}

#endif
