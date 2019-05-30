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
#define EXCE_MALLOC_NULL                        (-1)//这是一个非常严重的异常，内存空间不足，导致malloc或者new失败。通常这种情况会导致服务不可以使用，进入REDIS_SVR_INVALID_STATE状态
#define SVR_CONNECT_RESET                       (-2)//redis svr连接失败，内部会延迟之后再次重连
#define SVR_CONECT_DISCONNECT                   (-3)//redis svr断连。内部会延迟重连
#define CLUSTER_NODES_CHANGE                    (-4)//集群中节点信息变化
#define UNKOWN_REDIS_SVR_TYPE                   (-5)//位置服务端类型
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//return code define
//初始化接口调用返回值状态编号宏定义
//接口是异步的，但是同步也会做一些必要检查，校验。这些错误调用异步初始化接口可能的返回值
#define INIT_SUCCESS_CODE                       (0)
#define INIT_PARAMETER_ERROR                    (-1)//接口传入参数有误
#define INIT_SYSTEM_ERROR                       (-2)//通常是malloc 空指针
#define INIT_NO_INIT_ERROR                      (-3)//应该先调用redisAsyncInit方法

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//cmd code define
//执行命令返回值、以及回调函数中状态编号宏定义
//调用命令接口是异步的，但是同步会做一些检查，校验，这部分错误值分同步返回和异步返回的
#define CMD_SUCCESS_CODE                        (0)
#define CMD_PARAMETER_ERROR_CODE                (-1)
#define CMD_SVR_STATE_ERROR_CODE                (-2)//redis服务端初始化未完成或者出现异常，不可以执行任何命令操作
#define CMD_SYSTEM_MALLOC_CODE                  (-3)//mallc nullptr
#define CMD_SVR_INVALID_CODE                    (-4)//服务端已经失效，内部已经释放相关资源，一般由于内存不足会产生这个
#define CMD_SLOTS_CALCUL_ERROR_CODE             (-6)//槽计算错误，正常情况下不应该有这个错误
//当redis某一个点主从连接都挂掉了，并且正好落在这个点上就会返回这个错误。这个时候redis服务要设置槽覆盖不全也可以使用
//或者是单点的连接断掉了
#define CMD_SVR_NODES_DOWN                      (-7)

//callback
#define CMD_OUTTIME_CODE                        (-8)
#define CMD_REPLY_EMPTY_CODE                    (-9)
#define CMD_EMPTY_RESULT_CODE                   (-10)
#define CMD_REDIS_ERROR_CODE                    (-11)
#define CMD_REDIS_UNKNOWN_CODE                  (-12)
#define CMD_SVR_CLOSER_CODE                     (-13)//服务断连接
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//state callback code define
//状态回调函数中状态编号宏定义
#define REDIS_SVR_INIT_STATE                    (0)//初始化状态
#define REDIS_SVR_RUNING_STATE                  (1)//正常运行状态
#define REDIS_EXCEPTION_STATE                   (2)//部分异常状态
#define REDIS_SVR_ERROR_STATE                   (3)//错误异常，此状态下彻底无法执行命令
#define REDIS_SVR_INVALID_STATE                 (4)//redis服务彻底失效状态，一般在重连、恢复时内部内存不足。无法自我修复时才会传回这个值，此状态非常危险
                                                   //此状态值下会释放对应句柄所有一切内存资源。但是占用的句柄不会重复使用。
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

                    注意如果没有设置回调的方法，内部是不关心是否已经执行的。如果正处理退出临界区。可能会丢失
 *
 * @return 无
 */
typedef std::function<void(int ret, void* priv, const CLUSTER_REDIS_ASYNC::StdVectorStringPtr& resultMsg)> CmdResultCallback;

/*
 * [RedisAsync]异步redis集群或者单点管理类
 * @author xiaoxiao 2019-04-15
 * @param 这个类是单例模式，可以保证上层调用此模块管理多个、多种类型的redis服务，包括单点、
            主从、集群情况。内部会做各种异常恢复。
            但是当内存不足的情况下new不出来空间就会比较麻烦了。所以此模块运行的环境最好保证内存充足。
            如果内存不足情况下。并且连接又发生异常就很糟糕
            此类实现最基本的功能。由于可定制化需求太多。可以在此基本功能上在增加
            大多数方法实现都是异步的。减少IO线程操作，因为这个redis异步操作中，io回调回去......
            内部超时时间，并不是非常的精确，最大可能会有一秒的误差
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
     * @param threadNum 内部线程数，内部会开多少个线程来处理与redis的io。建议参考集群数量，但是也不是要特别多，最少也要是一个,内部会多开一个定时器线程
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
     * [addSigleRedisInfo] 初始化单节点redis服务，一个服务只需要初始化一次，多次初始化，会有多个连接。因为是异步的。所以一个连接基本就可以了
     * @author xiaoxiao 2019-05-06
     * @param ipPortInfo 单节点服务的服务信息，ip:port，不可以是多个
     * @param
     * @param
     *
     * @return 大于等于0都是成功，其他是错误。返回值为此redis服务的句柄。后续在此服务操作命令使用。内部此redis服务的唯一标识
     */
    int addSigleRedisInfo(const std::string& ipPortInfo);

    
    /*
     * [addMasterSlaveInfo] 初始化redis主从服务信息，一个服务只需要初始化一次，多次初始化，会有多个连接。因为是异步的。所以一个连接基本就可以了
     * @author xiaoxiao 2019-05-06
     * @param ipPortInfo 主从服务地址信息信息，ip:port，多个以逗号分隔
     * @param
     * @param
     *
     * @return 大于等于0都是成功，其他是错误。返回值为此redis服务的句柄。后续在此服务操作命令使用。内部此redis服务的唯一标识
     */
    int addMasterSlaveInfo(const std::string& ipPortInfo);

    
    /*
     * [addClusterInfo] 初始化redis集群服务信息，一个服务只需要初始化一次，多次初始化，会有多个连接。因为是异步的。所以一个连接基本就可以了
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
     * [set] set方法，在此位置也可以封装、增加一些其他命令的方法。
     * @author xiaoxiao 2019-05-30
     * @param asyFd 初始化服务信息返回的句柄
     * @param key 键
     * @param value 值
     * @param outSecond 命令执行超时时间
     * @param cb 回调函数
     * @param priv 私有数据指针，回调函数会调用回来
     *
     * @return 大于等于0都是成功，其他都是错误
     */
    int set(int asyFd, const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv);

    /*
     * [redisAppendCommand]异步执行命令
     * @author xiaoxiao 2019-04-19
     * @param asyFd 初始化服务信息返回的句柄
     * @param cb 命令结果回调函数
     * @param outSecond 命令超时时间如果超过这个时间未有结果则回调错误
     * @param priv 私有指针，保存命令的上下文
     * @param key 命令中的key
     * @param format 格式化的命令格式
     *
     * @return 大于等于0都是成功，其他都是错误
     */
    int redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...);

    /*
     * [redisAppendCommand]异步执行命令
     * @author xiaoxiao 2019-04-19
     * @param asyFd 初始化服务信息返回的句柄
     * @param cb 命令结果回调函数
     * @param outSecond 命令超时时间如果超过这个时间未有结果则回调错误
     * @param priv 私有指针，保存命令的上下文
     * @param key 命令中的key
     * @param cmdStr 命令的string
     *
     * @return 大于等于0都是成功，其他都是错误
     */
    int redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const std::string& cmdStr);


public:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///下面这些接口虽然是public，但是外面使用者千万不要调用，都是内部调用接口

    //内部会统计异步请求数字。在退出的时候。必须所有的异步回调都回调结束之后才能退出。当有回调方法的时候，才会计数。
    //所以通常没有回调的时候我们并不关心有没有执行完
    void asyncRequestCmdAdd();

    //内部连接成功或者失败调用此回调，连接事件的回调函数
    void redisSvrOnConnect(size_t asyFd, int state, const std::string& ipaddr, int port, const std::string errMsg = "");

    //异步结果回调，当有命令执行结束会通知回调线程池处理回调
    void asyncCmdResultCallBack();

private:

    //定时器线程运行方法
    void timerThreadRun();

    //每一个io线程运行方法
    void libeventIoThread(int index);

    //初始化redis初始化参数连接，同步方法
    int redisInitConnect();

    //当redis服务失效时异步回调这个方法，不可以同步调用，会core，慎用
    void redisSvrMgrInvalid(size_t asyFd);

    //异步结果回调
    void cmdReplyCallPool();

    //集群初始化命令回调函数
    void clusterInitCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg);
    //集群异常之后执行cluster info之后的回调函数
    void clusterInfoCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg);
    //集群异常之后执行cluster nodes之后的回调函数
    void clusterNodesCallBack(int ret, void* priv, const StdVectorStringPtr& resultMsg);
    
    //主从redis服务命令回调函数
    void masterSalveInitCb(int ret, void* priv, const StdVectorStringPtr& resultMsg);

    //redis服务状态变化回调函数
    void asyncStateCallBack(int asyFd, int stateCode, const std::string& stateMsg);

    //redis服务异常状态回调函数
    void asyncExceCallBack(int asyFd, int exceCode, const std::string& exceMsg);

    //立刻重连函数
    void reconnectAtOnece(size_t asyFd, const std::string& ipaddr, int port);
    //延迟重连函数
    void delayReconnect(size_t asyFd, const std::string& ipaddr, int port);
    
    int callBackNum_;//回调线程池线程数
    int ioThreadNum_;//io线程池，线程数
    int keepSecond_;//保持活跃心跳间隔秒钟
    int connOutSecond_;//连接超时秒钟数
    int isExit_;//是否退出标志
    int timerExit_;//是否退出标志

    volatile uint64_t nowSecond_;//当前绝对时间。这个时间很重要，每隔一秒会更新一次。后面所有的时间都是以他为基准
    std::mutex initMutex_;//内部初始化锁，
    StateChangeCb stateCb_;//状态变化回调函数
    ExceptionCallBack exceCb_;//异常状态回调函数
};

}
#endif
