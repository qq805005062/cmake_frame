#ifndef __CLUSTER_REDIS_ASYNC_H__
#define __CLUSTER_REDIS_ASYNC_H__

#include <vector>
#include <functional>
#include <mutex>
#include <memory>

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
#define INIT_NO_INIT_ERROR                      (-5)

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

#define REDIS_SVR_INIT_STATE                    (0)
#define REDIS_SVR_RUNING_STATE                  (1)
#define REDIS_SVR_ERROR_STATE                   (2)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * [function]
 * @author
 * @param
 * @param
 * @param
 *
 * @return
 */


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
 * @param asyFd 句柄，因为内部支持多个redis服务。每个服务唯一的句柄
 * @param ret 初始化返回值，等于0是成功，其他是错误
 * @param errMsg 当初始化失败的时候，错误信息,可能是redis集群返回的，也可能是内部自己定义
 * @param
 *
 * @return 无
 */
typedef std::function<void(int asyFd, int ret, const std::string& errMsg)> InitCallback;

/*
 * [ExceptionCallBack] 模块运行中异常回调，当初始化完成之后;初始化完成也分初始化失败和初始化成功，
                        初始化失败之后模块资源会释放。不会有其他操作，但是在初始中或者初始成功之后
                        内部有任何异常，比如断线会调用这个回调
                        初始化成功之后，所有异常，内部会主动尝试恢复，但是会回调上层，让上层感知
 * @author xiaoxiao 2019-04-18
 * @param asyFd 句柄，因为内部支持多个redis服务。每个服务唯一的句柄
 * @param exceCode 异常编号
 * @param exceMsg 异常信息
 * @param
 *
 * @return 无
 */
typedef std::function<void(int asyFd, int exceCode, const std::string& exceMsg)> ExceptionCallBack;

/*
 * [function] 执行命令结果回调函数。
 * @author xiaoxiao 2019-04-14
 * @param ret 命令执行结果,等于0是成功，其他是错误
 * @param priv 执行命令携带的私有指针
 * @param resultMsg 查询结果，结果可能是成对出现的
                    这个参数很重要，是表示结果集，正常情况下单一结果，比较清晰
                    但是hgetall zset是都是成对出现的。
                    hget指定域的话，就一定要注意了，一定是按域的顺序出现结果集的。但是结果集中不带域名称
 *
 * @return 无
 */
typedef std::function<void(int64_t ret, void* priv, const StdVectorStringPtr& resultMsg)> CmdResultCallback;

/*
 * [ClusterRedisAsync]异步redis集群或者单点管理类
 * @author xiaoxiao 2019-04-15
 * @param 这个类正常情况下是单例。但是特殊情况下也要支持多实例。所以内部还是要把指针传递下去使用。内部不要用单实例。保证外部可以选择单实例或者多实例模式
 * @param
 * @param
 *
 * 模块中有部分撰写代码的方式有些奇怪。主要是为了尽可能少的暴露模块内部定义及头文件出去。
 * 这样外面使用者不用关心内部的定义，只用关心此头文件里面的一些基础定义就可以完成使用
 * 
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
     * [redisAsyncInit] 初始化方法,单线程初始化，只需要初始化一次, 设置内部线程数及回调线程数。设置回调函数，主要设置初始化回调函数，设置异常回调函数
     * @author xiaoxiao 2019-04-15
     * @param threadNum 内部线程数，内部会开多少个线程来处理与redis的io。建议参考集群数量，但是也不是要特别多，最少也要是一个
     * @param callbackNum 回调线程池数。内部会开启多少个线程回调回调函数，因为不能用IO线程处理回调。不能拥堵IO
     * @param connOutSecond 连接超时时间，一般3秒
     * @param keepSecond 内部redis保持激活的间隔秒钟，保证这段时间，与redis至少心跳一次。以保持连接活跃
     * @param initcb 初始化完成回调函数，无论成功与失败，都会回调，初始化回调只会回调一次，回调则说明初始化完成
     * @param excecb 异常回调函数，可能会回调多次，包括在初始化过程中有异常会回调。或者运行中有异常也会回调
     *
     * @return 大于等于0是成功，其他是错误
     */
    int redisAsyncInit(int threadNum, int callbackNum, int connOutSecond, int keepSecond, const InitCallback& initcb, const ExceptionCallBack& excecb);

    /*
     * [addSigleRedisInfo] 初始化单节点redis服务
     * @author xiaoxiao 2019-05-06
     * @param ipPortInfo 单节点服务的服务信息，ip:port
     * @param
     * @param
     *
     * @return 大于等于0都是成功，其他是错误。返回值为此redis服务的句柄。后续在此服务操作命令使用。内部此redis服务的唯一标识
     */
    int addSigleRedisInfo(const std::string& ipPortInfo);

    
    /*
     * [addMasterSlaveInfo] 初始化redis主从服务信息
     * @author xiaoxiao 2019-05-06
     * @param ipPortInfo 主从服务地址信息信息，ip:port，多个以逗号分隔
     * @param
     * @param
     *
     * @return 大于等于0都是成功，其他是错误。返回值为此redis服务的句柄。后续在此服务操作命令使用。内部此redis服务的唯一标识
     */
    int addMasterSlaveInfo(const std::string& ipPortInfo);

    
    /*
     * [addClusterInfo] 初始化redis集群服务信息
     * @author xiaoxiao 2019-05-06
     * @param ipPortInfo 集群服务地址信息信息，ip:port，多个以逗号分隔
     * @param
     * @param
     *
     * @return 大于等于0都是成功，其他是错误。返回值为此redis服务的句柄。后续在此服务操作命令使用。内部此redis服务的唯一标识
     */
    int addClusterInfo(const std::string& ipPortInfo);
    
    /*
     * [function]
     * @author
     * @param
     * @param
     * @param
     *
     * @return
     */
    void redisAsyncExit();

    
    /*
     * [function]
     * @author
     * @param
     * @param
     * @param
     *
     * @return
     */
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
///下面这个虽然是public，但是外面使用者千万不要调用，都是内部调用接口
    void libeventIoThread(int index);

    void cmdReplyCallPool();

    //void initConnectException(int exceCode, std::string& exceMsg);

    void redisSvrOnConnect(int state, const std::string& ipaddr, int port);

private:
    int redisInitConnect();

    void asyncInitCallBack(int asyFd, int initCode, const std::string& initMsg);

    void asyncExceCallBack(int asyFd, int exceCode, const std::string& exceMsg);
    
    int callBackNum_;
    int ioThreadNum_;
    int keepSecond_;
    int connOutSecond_;
    int isExit_;

    std::mutex initMutex_;
    InitCallback initCb_;
    ExceptionCallBack exceCb_;
};

}
#endif
