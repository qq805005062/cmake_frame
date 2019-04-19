#ifndef __CLUSTER_REDIS_ASYNC_H__
#define __CLUSTER_REDIS_ASYNC_H__

#include <vector>
#include <functional>

#include "src/Singleton.h"
#include "src/noncopyable.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//exception code define
#define EXCE_INIT_CONN_FAILED                   (-1)
#define EXCE_RUNING_CONN_FAILED                 (-2)
#define EXCE_RUNING_DISCONN                     (-3)
#define EXCE_RUNING_SYSTEM_ERROR                (-4)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init code define

#define INIT_SUCCESS_CODE                       (0)
#define INIT_PARAMETER_ERROR                    (-1)
#define INIT_SYSTEM_ERROR                       (-2)
#define INIT_CONNECT_FAILED                     (-3)
#define INIT_SVRINFO_ERROR                      (-4)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//cmd code define
#define CMD_OUTTIME_CODE                        (-1)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//inside define 

#define REDIS_ASYNC_INIT_STATE                  (0)
#define REDIS_ASYNC_RUNING_STATE                (1)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace CLUSTER_REDIS_ASYNC
{

typedef std::vector<std::string> StdVectorString;
typedef StdVectorString::iterator StdVectorStringIter;


/*
 * [InitCallback] 初始化回调函数，异步初始化，当异步初始化完成之后，回调会被调用
                    当初始化单个节点时，因为区分不了是集群还是单点，所以连接失败之后或者成功之后会回调这个回调函数
                    当初始化多个节点时。默认是集群。从第一个节点信息尝试连接，如果全部连接失败，会回调这个函数，初始化失败
                    并且其中依次连接失败都会回调异常回调函数ExceptionCallBack。有一个点连接成功，并且正确获得集群信息，则认为初始化完成
                    反正有一个点连接成功，但是未获得集群信息，则认为初始化失败
                    初始化失败内部会释放所有资源
 * @author xiaoxiao 2019-04-14
 * @param ret 初始化返回值，等于0是成功，其他是错误
 * @param errMsg 当初始化失败的时候，错误信息,可能是redis集群返回的，也可能是内部自己定义
 * @param
 *
 * @return 无
 */
typedef std::function<void(int ret, const std::string& errMsg)> InitCallback;

/*
 * [ExceptionCallBack] 模块运行中异常回调，当初始化完成之后;初始化完成也分初始化失败和初始化成功，
                        初始化失败之后模块资源会释放。不会有其他操作，但是在初始中或者初始成功之后
                        内部有任何异常，比如断线会调用这个回调
                        初始化成功之后，所有异常，内部会主动尝试恢复，但是会回调上层，让上层感知
 * @author xiaoxiao 2019-04-18
 * @param exceCode 异常编号
 * @param exceMsg 异常信息
 * @param
 *
 * @return 无
 */
typedef std::function<void(int exceCode, const std::string& exceMsg)> ExceptionCallBack;

/*
 * [function] 执行命令结果回调函数。
 * @author xiaoxiao 2019-04-14
 * @param ret 命令执行结果,等于0是成功，其他是错误
 * @param priv 执行命令携带的私有指针
 * @param resultMsg 查询结果，结果可能是成对出现的
 *
 * @return 无
 */
typedef std::function<void(int64_t ret, void* priv, StdVectorString& resultMsg)> CmdResultCallback;

/*
 * [ClusterRedisAsync]异步redis集群或者单点管理类
 * @author xiaoxiao 2019-04-15
 * @param
 * @param
 * @param
 *
 * @return
 */
class ClusterRedisAsync : public common::noncopyable
{
public:
    ClusterRedisAsync();

    ~ClusterRedisAsync();

    static ClusterRedisAsync& instance() { return common::Singleton<ClusterRedisAsync>::instance(); }

    /*
     * [redisAsyncInit] 初始化方法,单线程初始化，只需要初始化一次
     * @author xiaoxiao 2019-04-15
     * @param threadNum 内部线程数，内部会开多少个线程来处理与redis的io。建议参考集群数量，但是也不是要特别多，最少也要是一个
     * @param ipPortInfo redis集群信息，可以配置单点，也可以配置多个点。初始化如果连不上会失败，内部支持主从切换、支持自动重连
     * @param initcb 初始化完成回调函数，无论成功与失败，都会回调
     * @param connNum 连接数量，因为是异步，所以基本上1个也就够用了，当然也可以初始化多个
     *
     * @return 大于等于0是成功，其他是错误，内部异步初始化
     */
    int redisAsyncInit(int threadNum, const std::string& ipPortInfo, InitCallback& initcb, int connNum = 1);

    int set(const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv);

    /*
     * [redisAppendCommand]异步执行命令
     * @author xiaoxiao 2019-04-19
     * @param cb 命令结果回调函数
     * @param outSecond 命令超时时间如果超过这个时间未有结果则回调错误
     * @param priv 私有指针，保存命令的上下文
     * @param key 命令中的key
     * @param format 格式化的命令格式
     *
     * @return
     */
    int redisAsyncCommand(const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...);

    /*
     * [redisAppendCommand]异步执行命令
     * @author xiaoxiao 2019-04-19
     * @param cb 命令结果回调函数
     * @param outSecond 命令超时时间如果超过这个时间未有结果则回调错误
     * @param priv 私有指针，保存命令的上下文
     * @param key 命令中的key
     * @param cmdStr 命令的string
     *
     * @return
     */
    int redisAsyncCommand(const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const std::string& cmdStr);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void redisAsyncConnect();

    void libeventIoThread(int index);
private:

    void asyncInitCallBack(int ret);

    int connNum_;
    int ioThreadNum_;
    int state_;
    
    InitCallback initCb_;
    ExceptionCallBack exceCb_;
};

}
#endif
