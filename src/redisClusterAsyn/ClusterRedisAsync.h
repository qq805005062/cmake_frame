#ifndef __CLUSTER_REDIS_ASYNC_H__
#define __CLUSTER_REDIS_ASYNC_H__

#include <vector>
#include <functional>

#include "src/Singleton.h"
#include "src/noncopyable.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//exception code define
//异常状态编码宏定义
#define EXCE_INIT_CONN_FAILED                   (-1)
#define EXCE_RUNING_CONN_FAILED                 (-2)
#define EXCE_RUNING_DISCONN                     (-3)
#define EXCE_SYSTEM_ERROR                       (-4)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init code define
//初始化状态编号宏定义
#define INIT_SUCCESS_CODE                       (0)
#define INIT_PARAMETER_ERROR                    (-1)
#define INIT_SYSTEM_ERROR                       (-2)
#define INIT_CONNECT_FAILED                     (-3)
#define INIT_SVRINFO_ERROR                      (-4)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//cmd code define
//执行命令状态编号宏定义
#define CMD_SUCCESS_CODE                        (0)
#define CMD_OUTTIME_CODE                        (-1)
#define CMD_NOREDIS_SVR_NODE_CODE               (-2)
#define CMD_REDIS_NODE_DISCONNECT               (-3)
#define CMD_REPLY_EMPTY_CODE                    (-4)
#define CMD_MALLOC_NULL_CODE                    (-5)
#define CMD_EMPTY_RESULT_CODE                   (-6)
#define CMD_REDIS_ERROR_CODE                    (-7)
#define CMD_REDIS_UNKNOWN_CODE                  (-8)


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//inside define 
//连接redis服务端状态标志
#define CONNECT_REDISVR_SUCCESS                 (0)
#define CONNECT_REDISVR_RESET                   (1)
#define REDISVR_CONNECT_DISCONN                 (2)

//内部redis各个状态标志位宏定义
#define REDIS_ASYNC_INIT_STATE                  (0)
#define REDIS_ASYNC_SINGLE_RUNING_STATE         (1)
#define REDIS_ASYNC_MASTER_SLAVE_STATE          (2)
#define REDIS_ASYNC_CLUSTER_RUNING_STATE        (3)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace CLUSTER_REDIS_ASYNC
{
typedef std::shared_ptr<std::string> StdStringSharedPtr;
typedef std::vector<StdStringSharedPtr> StdVectorStringPtr;
typedef StdVectorStringPtr::iterator StdVectorStringPtrIter;

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
typedef std::function<void(int64_t ret, void* priv, const StdVectorStringPtr& resultMsg)> CmdResultCallback;

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
     * [redisAsyncSetCallback] 设置回调函数，主要设置初始化回调函数，设置异常回调函数
     * @author xiaoxiao 2019-04-15
     * @param initcb 初始化完成回调函数，无论成功与失败，都会回调，初始化回调只会回调一次，回调则说明初始化完成
     * @param excecb 异常回调函数，可能会回调多次，包括在初始化过程中有异常会回调。或者运行中有异常也会回调
     *
     * @return 无
     */
    void redisAsyncSetCallback(const InitCallback& initcb, const ExceptionCallBack& excecb);
    /*
     * [redisAsyncInit] 初始化方法,单线程初始化，只需要初始化一次
     * @author xiaoxiao 2019-04-15
     * @param threadNum 内部线程数，内部会开多少个线程来处理与redis的io。建议参考集群数量，但是也不是要特别多，最少也要是一个
     * @param callbackNum 回调线程池数。内部会开启多少个线程回调回调函数，因为不能用IO线程处理回调。不能拥堵IO
     * @param ipPortInfo redis集群信息，可以配置单点，也可以配置多个点。初始化如果连不上会失败，内部支持主从切换、支持自动重连
     * @param connOutSecond 连接超时时间，一般3秒
     * @param keepSecond 内部redis保持激活的间隔秒钟，保证这段时间，与redis至少心跳一次。以保持连接活跃
     * @param connNum 连接数量，因为是异步，所以基本上1个也就够用了，当然也可以初始化多个
     *
     * @return 大于等于0是成功，其他是错误，内部异步初始化
     */
    int redisAsyncInit(int threadNum, int callbackNum, const std::string& ipPortInfo, int connOutSecond, int keepSecond, int connNum = 1);

    void redisAsyncExit();

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

    //void initConnectException(int exceCode, std::string& exceMsg);

    void redisSvrOnConnect(int state, const std::string& ipaddr, int port);

    void cmdReplyCallPool();
private:

    void asyncInitCallBack(int initCode, const std::string& initMsg);

    void asyncExceCallBack(int exceCode, const std::string& exceMsg);

    int connNum_;
    int callBackNum_;
    int ioThreadNum_;
    int state_;//内部模块状态及运行状态的标志位
    int keepSecond_;
    int connOutSecond_;
    int isExit_;
    
    InitCallback initCb_;
    ExceptionCallBack exceCb_;
};

}
#endif
