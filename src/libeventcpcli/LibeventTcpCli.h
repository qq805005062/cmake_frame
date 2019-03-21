#ifndef __LIBEVENT_TCP_CLIENT_H__
#define __LIBEVENT_TCP_CLIENT_H__

#include <mutex>

#include <stdint.h>
#include <functional>

#include "src/Singleton.h"
#include "src/noncopyable.h"

//#include "LibeventTcpCli.h"

/*
 *TCP 客户端封装，支持连接多个服务端，库本身不能保证消息必达。消息不丢不重需要通信协议来保证
 *每一个连接都会有一个唯一的编号，如果要保持多个程序之间唯一的话，在初始化时保证第一个参数唯一即可
 *连接回调和消息到达回调会带回每个连接的唯一编号，并且每一个连接都可以携带一个私有数据结构，私有数据结构内部只负责保持指针，不维护内存
 *连接回调和消息到达回调同时会将服务端的ip地址和端口带回
 *
 */

namespace LIBEVENT_TCP_CLI
{
//state = 0 是未连接上，连接失败， 1 是连接成功。 2 是连接成功之后断开
//连接回调，因为内部会通知断开或者连接上，其中断开包括没有连接成功和从连接成功之后端口两种情况。上层要注意区分
typedef std::function<void(uint64_t uniqueid, void* priv, int state, const std::string& ipaddr, int port)> TcpConnectCallback;
//IO线程不要做阻塞操作，尽快返回

//返回已经使用缓冲区多少数据
typedef std::function<size_t(uint64_t uniqueid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port)> TcpOnMessageCallback;
//IO线程不要做阻塞式操作，尽快返回

class LibeventTcpCli : public noncopyable
{
public:
	//构造函数，无参数
	LibeventTcpCli();
	//析构函数
	~LibeventTcpCli();

	//单实例模式，任何项目全局唯一就可，可以支持连接多个服务端，上层控制重连和连接数，测试发送，并不是连接数越多，发送量可以越大，带宽会影响发送量，2个连接比较合适了
	static LibeventTcpCli& instance() { return Singleton<LibeventTcpCli>::instance();}

	//模块退出时使用，阻塞式操作，如果正在收发数据时，可能会丢数据，并不能保证底层发送缓冲区全部发送完成或者接受缓冲区全部回调
	void libeventTcpCliExit();

	//模块初始化操作，
	//第一参数给一个随机值，无所谓什么值，给一个就好，生成后续唯一编号用的，如果要保持全局唯一的话，每个程序要保持不同即可
	//第二个参数，内部IO线程池数目，
	//连接回调函数注册
	//收到消息回调函数
	int libeventTcpCliInit(unsigned int uniquId, unsigned int threadNum, const TcpConnectCallback& connCb_, const TcpOnMessageCallback& msgCb_);

	//增加一个服务端连接接口
	//服务端ip地址，回调函数会带回来那个ip地址回来的数据
	//服务端端口，回调函数会带回来那个端口回来的数据
	//每个连接上携带的私有数据，内部不维护内存结构，外部要自己析构、new
	//连接超时时间，如果连接在大于这个时间无数据来往的话，及认为不可用，单位秒钟
	int libeventAddConnect(const std::string& ipaddr, int port, void* priv = NULL, int dataOutSecond = 30, int connOutSecond = 3);

	//发送消息，异步发送，返回值仅仅表示这个连接是否可以发送，并不是真正的发送成功
	//第一个参数是每一个连接唯一的标识数子
	//发送消息的指针，
	//发送消息的长度，
	int libeventTcpCliSendMsg(uint64_t unid, const char* msg, size_t msglen);

	//重置TCP连接的私有数据。这个操作要注意，
	//首先私有数据都是在io线程回调的，所以这个方法最好在io回调中调用。保证线程安全
	//同时要注意上层其他地方保留的数据对应关系
	int libeventTcpCliResetPrivate(uint64_t unid, void* priv = NULL);

	//会触发回调，但是不在IO线程回调，也可以做成不触发回调//可能会有一点危险性，不知道libevent内部如何处理，如果我析构了bufevent，但是有后续信号还会不会被调用，如果会被调用，可能会core。
	//但是如果内部处理好的话，应该就没问题，慎用
	int libeventTcpCliDisconnect(uint64_t unid);
	
///内部使用，外部不需要关心
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void libeventIoExpireThread();

	void libeventIoThread(size_t index);

	void libeventIoThreadReady();

	void tcpServerConnect(uint64_t uniqueid, void* priv, int state, const std::string& ipaddr, int port);

	size_t tcpServerOnMessage(uint64_t uniqueid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port);
		
private:
	int isExit;

	unsigned int uniquId_;
	unsigned int ioThreadNum;
	unsigned int lastIndex;
	unsigned int readyIothread;
	unsigned int exitIothread;

	TcpConnectCallback connCb;
	TcpOnMessageCallback msgCb;

	std::mutex mutex_;
};

}

#endif
