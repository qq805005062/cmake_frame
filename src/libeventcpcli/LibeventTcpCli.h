#ifndef __LIBEVENT_TCP_CLIENT_H__
#define __LIBEVENT_TCP_CLIENT_H__

#include <mutex>

#include <stdint.h>
#include <functional>

#include "src/Singleton.h"
#include "src/noncopyable.h"

//#include "LibeventTcpCli.h"

/*
 *TCP �ͻ��˷�װ��֧�����Ӷ������ˣ��Ȿ���ܱ�֤��Ϣ�ش��Ϣ����������Ҫͨ��Э������֤
 *ÿһ�����Ӷ�����һ��Ψһ�ı�ţ����Ҫ���ֶ������֮��Ψһ�Ļ����ڳ�ʼ��ʱ��֤��һ������Ψһ����
 *���ӻص�����Ϣ����ص������ÿ�����ӵ�Ψһ��ţ�����ÿһ�����Ӷ�����Я��һ��˽�����ݽṹ��˽�����ݽṹ�ڲ�ֻ���𱣳�ָ�룬��ά���ڴ�
 *���ӻص�����Ϣ����ص�ͬʱ�Ὣ����˵�ip��ַ�Ͷ˿ڴ���
 *
 */
namespace LIBEVENT_TCP_CLI
{

//���ӻص�����Ϊ�ڲ���֪ͨ�Ͽ����������ϣ����жϿ�����û�����ӳɹ��ʹ����ӳɹ�֮��˿�����������ϲ�Ҫע������
typedef std::function<void(uint64_t uniqueid, void* priv, bool isConn, const std::string& ipaddr, int port)> TcpConnectCallback;
//IO�̲߳�Ҫ���������������췵��

//�����Ѿ�ʹ�û�������������
typedef std::function<size_t(uint64_t uniqueid, void* priv, const char* msg, size_t msglen, const std::string& ipaddr, int port)> TcpOnMessageCallback;
//IO�̲߳�Ҫ������ʽ���������췵��

class LibeventTcpCli : public noncopyable
{
public:
	//���캯�����޲���
	LibeventTcpCli();
	//��������
	~LibeventTcpCli();

	//��ʵ��ģʽ���κ���Ŀȫ��Ψһ�Ϳɣ�����֧�����Ӷ������ˣ��ϲ���������������������Է��ͣ�������������Խ�࣬����������Խ�󣬴����Ӱ�췢������2�����ӱȽϺ�����
	static LibeventTcpCli& instance() { return Singleton<LibeventTcpCli>::instance();}

	//ģ���˳�ʱʹ�ã�����ʽ��������������շ�����ʱ�����ܻᶪ���ݣ������ܱ�֤�ײ㷢�ͻ�����ȫ��������ɻ��߽��ܻ�����ȫ���ص�
	void libeventTcpCliExit();

	//ģ���ʼ��������
	//��һ������һ�����ֵ������νʲôֵ����һ���ͺã����ɺ���Ψһ����õģ����Ҫ����ȫ��Ψһ�Ļ���ÿ������Ҫ���ֲ�ͬ����
	//�ڶ����������ڲ�IO�̳߳���Ŀ��
	//���ӻص�����ע��
	//�յ���Ϣ�ص�����
	int libeventTcpCliInit(unsigned int uniquId, unsigned int threadNum, const TcpConnectCallback& connCb_, const TcpOnMessageCallback& msgCb_);

	//����һ����������ӽӿ�
	//�����ip��ַ���ص�������������Ǹ�ip��ַ����������
	//����˶˿ڣ��ص�������������Ǹ��˿ڻ���������
	//ÿ��������Я����˽�����ݣ��ڲ���ά���ڴ�ṹ���ⲿҪ�Լ�������new
	//���ӳ�ʱʱ�䣬��������ڴ������ʱ�������������Ļ�������Ϊ�����ã���λ����
	int libeventAddConnect(const std::string& ipaddr, int port, void* priv = NULL, int outSecond = 30);

	//������Ϣ���첽���ͣ�����ֵ������ʾ��������Ƿ���Է��ͣ������������ķ��ͳɹ�
	//��һ��������ÿһ������Ψһ�ı�ʶ����
	//������Ϣ��ָ�룬
	//������Ϣ�ĳ��ȣ�
	int libeventTcpCliSendMsg(uint64_t unid, const char* msg, size_t msglen);

	//�ᴥ���ص������ǲ���IO�̻߳ص���Ҳ�������ɲ������ص�
	int libeventTcpCliDisconnect(uint64_t unid);
	
///�ڲ�ʹ�ã��ⲿ����Ҫ����
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void libeventIoThread(size_t index);

	void libeventIoThreadReady();

	void tcpServerConnect(uint64_t uniqueid, void* priv, bool isConn, const std::string& ipaddr, int port);

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
