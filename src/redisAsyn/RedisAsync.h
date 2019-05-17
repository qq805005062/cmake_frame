#ifndef __CLUSTER_REDIS_ASYNC_H__
#define __CLUSTER_REDIS_ASYNC_H__

#include <vector>
#include <functional>
#include <mutex>
#include <memory>

//#include "RedisAsync.h"
#include "src/Singleton.h"
#include "src/noncopyable.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//exception code define
//异常回调函数中状态编码宏定义
#define EXCE_RECONNECT_NEWORDER_NULL            (-1)

#define EXCE_INIT_CONN_FAILED                   (-1)
#define EXCE_RUNING_CONN_FAILED                 (-2)
#define EXCE_RUNING_DISCONN                     (-3)
#define EXCE_SYSTEM_ERROR                       (-4)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//return code define
//接口调用返回值状态编号宏定义
#define INIT_SUCCESS_CODE                       (0)
#define INIT_PARAMETER_ERROR                    (-1)
#define INIT_SYSTEM_ERROR                       (-2)
#define INIT_CONNECT_FAILED                     (-3)
#define INIT_SVRINFO_ERROR                      (-4)
#define INIT_NO_INIT_ERROR                      (-5)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//cmd code define
//执行命令回调状态编号宏定义
#define CMD_SUCCESS_CODE                        (0)
#define CMD_PARAMETER_ERROR_CODE                (-1)
#define CMD_SVR_NO_INIT_CODE                    (-2)
#define CMD_SYSTEM_MALLOC_CODE                  (-3)

//callback

#define CMD_OUTTIME_CODE                        (-4)
#define CMD_REPLY_EMPTY_CODE                    (-5)
#define CMD_EMPTY_RESULT_CODE                   (-6)
#define CMD_REDIS_ERROR_CODE                    (-7)
#define CMD_REDIS_UNKNOWN_CODE                  (-8)

#define CMD_SVR_DISCONNECT_CODE                 (-9)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//inside connect define 
//客户端连接服务端状态标志
#define CONNECT_REDISVR_SUCCESS                 (0)
#define CONNECT_REDISVR_RESET                   (1)
#define REDISVR_CONNECT_DISCONN                 (2)

//svr type define
//内部redis节点连接服务端类型宏定义
#define REDIS_ASYNC_INIT_STATE                  (0)
#define REDIS_ASYNC_SINGLE_RUNING_STATE         (1)
#define REDIS_ASYNC_MASTER_SLAVE_STATE          (2)
#define REDIS_ASYNC_CLUSTER_RUNING_STATE        (3)

//state callback code define
//状态回调函数中状态编号宏定义
#define REDIS_SVR_INIT_STATE                    (0)//初始化状态
#define REDIS_SVR_RUNING_STATE                  (1)//正常运行状态
#define REDIS_EXCEPTION_STATE                   (2)//部分异常状态
#define REDIS_SVR_ERROR_STATE                   (3)//错误异常，此状态下彻底无法执行命令
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
 * [StateChangeCb] 状态变化回调函数，异步函数中，当服务端redis状态发生变化时，回调此函数
                    此回调函数表示对应服务端连接或者服务使用状态发生变化。内部会做恢复机制。但是比较担心的是内存不足情况下，new不出来对象的情况
                    当初始化单个节点服务时，连接成功会调用这个回调，表示服务可用了，连接失败会调用异常回调。但是内部会重连
                    当初始化集群或者主从服务时。第一个节点连接成功并不会回调此函数，因为状态并没有初始化好。
                    当主从或者集群所有连接都初始化正常，并且服务正常时会回调此函数。内部有异常，都会尝试恢复、重连。
                    如果内部想实现主从读写分离，需要自己修改代码。因为定制化需求太多。在基础版上略作修改即可。
                    比如：读写分离可能需要。但是某些情况下考虑临界情况则不可以做读写分离。多个主从做集群。都可在基础版上做修改
 * @author xiaoxiao 2019-04-14
 * @param asyFd 句柄，因为内部支持多个redis服务。每个服务唯一的句柄
 * @param ret 状态变化值对应 REDIS_SVR_INIT_STATE REDIS_SVR_RUNING_STATE REDIS_EXCEPTION_STATE REDIS_SVR_ERROR_STATE
 * @param errMsg 状态变化对应部分信息
 * @param
 *
 * @return 无
 */
typedef std::function<void(int asyFd, int ret, const std::string& errMsg)> StateChangeCb;

/*
 * [ExceptionCallBack] redis服务运行过程中异常回调函数
                        初始化中或者运行过程中，任何异常都会回调此异常回调，
                        如果异常导致服务状态发送变化同时也会回调状态变化回调函数
                        其中有一个异常比较难处理。当自动重连是new失败的情况。这个回调回来应该由上层去处理了。但是就违背了内部自动恢复的原则了。需要考虑（可以让上层延迟重连）
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
 * [CmdResultCallback] 执行命令结果回调函数。
                        任何异步命令结果回调函数，也有可能是超时了。超时也会回调此函数。
                        此回调通知上层命令执行结果。如果一个连接上堆积的异步命令因为连接断掉。都会瞬间全部调用此回调
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
typedef std::function<void(int ret, void* priv, const StdVectorStringPtr& resultMsg)> CmdResultCallback;

/*
 * [RedisAsync]异步redis集群或者单点管理类
 * @author xiaoxiao 2019-04-15
 * @param 这个类是单例模式，可以保证上层调用此模块管理多个、多种类型的redis服务，包括单点、
            主从、集群情况。内部会做各种异常恢复。
            但是当内存不足的情况下new不出来空间就会比较麻烦了。所以此模块运行的环境最好保证内存充足。
            如果内存不足情况下。并且连接又发生异常就很糟糕
            此类实现最基本的功能。由于可定制化需求太多。可以在此基本功能上在增加
 * @param
 * @param
 *
 * 模块中有部分撰写代码的方式有些奇怪。主要是为了尽可能少的暴露模块内部定义及头文件出去。
 * 这样外面使用者不用关心内部的定义，只用关心此头文件里面的一些基础定义就可以完成使用
 * 
 *
 * @return
 */
class RedisAsync : public common::noncopyable
{
public:
    RedisAsync();

    ~RedisAsync();

    static RedisAsync& instance() { return common::Singleton<RedisAsync>::instance(); }

    /*
     * [redisAsyncInit] 初始化方法,单线程初始化，只需要初始化一次, 设置内部线程数及回调线程数。设置回调函数，主要设置初始化回调函数，设置异常回调函数
     * @author xiaoxiao 2019-04-15
     * @param threadNum 内部线程数，内部会开多少个线程来处理与redis的io。建议参考集群数量，但是也不是要特别多，最少也要是一个
     * @param callbackNum 回调线程池数。内部会开启多少个线程回调回调函数，因为不能用IO线程处理回调。不能拥堵IO
     * @param connOutSecond 连接超时时间，一般3秒
     * @param keepSecond 内部redis保持激活的间隔秒钟，保证这段时间，与redis至少心跳一次。以保持连接活跃
     * @param statecb 初始化完成回调函数，无论成功与失败，都会回调，初始化回调只会回调一次，回调则说明初始化完成
     * @param excecb 异常回调函数，可能会回调多次，包括在初始化过程中有异常会回调。或者运行中有异常也会回调
     *
     * @return 大于等于0是成功，其他是错误
     */
    int redisAsyncInit(int threadNum, int callbackNum, int connOutSecond, int keepSecond, const StateChangeCb& statecb, const ExceptionCallBack& excecb);

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
     * [redisAsyncExit] 模块退出方法。会释放所有连接及内部资源。此模块暂时未考虑支持运行中exit之后再init
                            最好是程序启动时 init。退出前exit就好。
     * @author xiaoxiao 2019-05-10
     * @param
     * @param
     * @param
     *
     * @return 无
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
    int set(int asyFd, const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv);

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
    int redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...);

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
    int redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const std::string& cmdStr);


public:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///下面这些接口虽然是public，但是外面使用者千万不要调用，都是内部调用接口

    void timerThreadRun();

    void libeventIoThread(int index);

    //异步结果回调
    void cmdReplyCallPool();

    //void initConnectException(int exceCode, std::string& exceMsg);

    void redisSvrOnConnect(size_t asyFd, int state, const std::string& ipaddr, int port);

    void asyncStateCallBack(int asyFd, int stateCode, const std::string& stateMsg);

    void asyncExceCallBack(int asyFd, int exceCode, const std::string& exceMsg);

    //异步结果回调
    void asyncCmdResultCallBack();

    void clusterInitCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg);

    void masterSalveInitCb(int ret, void* priv, const StdVectorStringPtr& resultMsg);

private:
    int redisInitConnect();
    
    int callBackNum_;
    int ioThreadNum_;
    int keepSecond_;
    int connOutSecond_;
    int isExit_;

    volatile uint64_t nowSecond_;
    std::mutex initMutex_;
    StateChangeCb stateCb_;
    ExceptionCallBack exceCb_;
};

}
#endif
