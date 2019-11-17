#ifndef __REMAIN_MGR_WRITEFILE_H__
#define __REMAIN_MGR_WRITEFILE_H__

#include <map>
#include <memory>
#include <vector>
#include <mutex>

//返回错误码以10开头
namespace REMAIN_MGR
{

//共享目录下文件操作函数，会首先判断共享目录是否存在，共享根目录不存在的话则表示共享目录有问题
//因为是操作共享文件夹，所以会优先判断根目录是否存在，如果跟目录不存在的话，则证明共享文件已经离线
//挂载的根目录应该比配置的根目录少一层目录
//根目录配置空的话，则说明未配置共享路径
class ShareWriteFileMgr
{
public:

    //构造方法，内部不做复杂操作，只做赋值
    ShareWriteFileMgr(int gateNo, int maxCout, int outSecond, int flag, const std::string& rootPath, const std::string& userId);

    ~ShareWriteFileMgr();

    //写文件类容，内部自动维护文件切换，更新
    int writeToFile(const char* content, size_t size);

    //类退出，析构相关资源的类
    void exitClose();

    //检查是否要切换新的文件
    void everySecondCheck(uint64_t second);

    void updateGenFreq(int second);

    void updateMaxFileSize(int size);

private:

	//切换新的文件句柄
	int switchToNewFile();

	//关闭当前的文件句柄
	int CloseCurFile();
	
	int isExit_;
	int gateNo_;
	int seqId_;
	int millSecond_;

	int outSecond_;
	
	int maxCount_;
	volatile int curCount_;

	volatile uint64_t creatSecond_;

	FILE* pFile_;

	std::string userName_;
	std::string rootPath_;
	std::string sharePath_;
	
	std::mutex mutex_;
};

typedef std::shared_ptr<ShareWriteFileMgr> ShareWriteFileMgrPtr;

//本地磁盘文件操作封装类，此操作失败的概率会小很多但是也是有失败的可能
class localWriteFileMgr
{
public:

	localWriteFileMgr(int gateNo, int maxCout, int outSecond, int flag, const std::string& rootPath, const std::string& userId);

	~localWriteFileMgr();

	int writeToFile(const char* content, size_t size);
	
	void exitClose();

	void everySecondCheck(uint64_t second);

    void updateGenFreq(int second);

    void updateMaxFileSize(int size);

private:

	int switchToNewFile();

	int CloseCurFile();

	int isExit_;
	int gateNo_;
	int seqId_;
	int millSecond_;

	int outSecond_;
	
	int maxCount_;
	volatile int curCount_;

	volatile uint64_t creatSecond_;

	FILE* pFile_;

	std::string userName_;
	std::string rootPath_;
	std::string localPath_;
	
	std::mutex mutex_;
};

typedef std::shared_ptr<localWriteFileMgr> localWriteFileMgrPtr;

/*
 *由于linux共享路径的特性。共享路径mount到本地应该多一层路径，程序会首先判断多的一层路径在不在，不在的话，则说明共享路径出了问题
 *
 *并且由于共享磁盘的问题，如果出错的话，则需要人工干预
 *
 *
 *
 */
class WriteFileMgr
{
public:
	WriteFileMgr(int gateNo, int maxCout, int outSecond, const std::string& sharePath, const std::string localPath, const std::string& userId);

	~WriteFileMgr();

	int writeToFile(int flag, const char* content, size_t size);
	
	void exitClose();

	void everySecondCheck(uint64_t second);

    void updateGenFreq(int second);

    void updateMaxFileSize(int size);

	std::string fileMgrUserName()
	{
		return userName_;
	}
private:

	int isExit_;
	int gateNo_;
	int maxCount_;
	int outSecond_;

	std::string userName_;
	std::string localPath_;
	std::string sharePath_;

	ShareWriteFileMgrPtr shareReFile;
	ShareWriteFileMgrPtr shareReBakFile;
	ShareWriteFileMgrPtr shareMoFile;
	ShareWriteFileMgrPtr shareMoBakFile;
	ShareWriteFileMgrPtr shareMtFile;
	ShareWriteFileMgrPtr shareMtBakFile;

	localWriteFileMgrPtr localReFile;
	localWriteFileMgrPtr localReBakFile;
	localWriteFileMgrPtr localMoFile;
	localWriteFileMgrPtr localMoBakFile;
	localWriteFileMgrPtr localMtFile;
	localWriteFileMgrPtr localMtBakFile;
};


typedef std::shared_ptr<WriteFileMgr> WriteFileMgrPtr;

typedef std::vector<WriteFileMgrPtr> WriteFileMgrPtrVect;

typedef std::map<std::string, WriteFileMgrPtr> UserIdFileMgrMap;
typedef UserIdFileMgrMap::iterator UserIdFileMgrMapIter;

}

#endif

