#ifndef __XIAO_HTTP_REQ_SESSION_H__
#define __XIAO_HTTP_REQ_SESSION_H__

#include <vector>
#include <memory>
#include <string>
#include <functional>

typedef enum{
	CURLHTTP10 = 0,
	CURLHTTP11,
	CURLHTTP20,
	CURLHTTP2TLS,
	CURLHTTPNONE
}CURL_HTTP_VERSION;

typedef enum{
	CURLHTTP_GET = 0,
	CURLHTTP_POST,
	CURLHTTP_PUT,
	CURLHTTP_DELETE,
	CURLHTTP_UPDATE,
	CURLHTTP_UNKONOW
}CURL_HTTP_REQUEST_TYPE;

typedef std::vector<std::string> HttpHeadPrivate;
typedef HttpHeadPrivate::iterator HttpHeadPrivateIter;

namespace CURL_HTTP_CLI
{

class HttpReqSession;
typedef std::shared_ptr<HttpReqSession> HttpReqSessionPtr;
//typedef std::function<void(CURL_HTTP_CLI::HttpReqSession* curlReq)> CurlRespondCallBack;
typedef std::function<void(CURL_HTTP_CLI::HttpReqSessionPtr& curlReq)> CurlRespondCallBack;

class HttpReqSession
{
public:
	HttpReqSession()
		:httpVer(CURLHTTP11)
		,reqType(CURLHTTP_POST)
		,private_(nullptr)
		,connInfo_(nullptr)
		,maxconns_(0)
		,connOutSecond(30)
		,dataOutSecond(60)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,isKeepAlive_(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl()
		,reqData()
		,errorMsg()
		,cacertFile()
		,headVec()
		,cb_(nullptr)
	{
	}

	HttpReqSession(CURL_HTTP_VERSION ver, CURL_HTTP_REQUEST_TYPE type, const std::string& url, const std::string& body)
		:httpVer(ver)
		,reqType(type)
		,private_(nullptr)
		,connInfo_(nullptr)
		,maxconns_(0)
		,connOutSecond(30)
		,dataOutSecond(60)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,isKeepAlive_(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl(url)
		,reqData(body)
		,errorMsg()
		,cacertFile()
		,headVec()
		,cb_(nullptr)
	{
	}

	HttpReqSession(const HttpReqSession& that)
		:httpVer(CURLHTTP11)
		,reqType(CURLHTTP_POST)
		,private_(nullptr)
		,connInfo_(nullptr)
		,maxconns_(0)
		,connOutSecond(30)
		,dataOutSecond(60)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,isKeepAlive_(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl()
		,reqData()
		,errorMsg()
		,cacertFile()
		,headVec()
		,cb_(nullptr)
	{
		*this = that;
	}

	HttpReqSession& operator=(const HttpReqSession& that)
	{
		if (this == &that) return *this;

		httpVer = that.httpVer;
		reqType = that.reqType;
		private_ = that.private_;
		connInfo_ = that.connInfo_;
		maxconns_ = that.maxconns_;
		connOutSecond = that.connOutSecond;
		dataOutSecond = that.dataOutSecond;
		insertMicroSecond = that.insertMicroSecond;
		reqMicroSecond = that.reqMicroSecond;
		rspMicroSecond = that.rspMicroSecond;
		isKeepAlive_ = that.isKeepAlive_;
		rspCode = that.rspCode;
		sslVerifyPeer = that.sslVerifyPeer;
		sslVeriftHost = that.sslVeriftHost;
		rspBody = that.rspBody;
		reqUrl = that.reqUrl;
		reqData = that.reqData;
		errorMsg = that.errorMsg;
		cacertFile = that.cacertFile;
		headVec = that.headVec;
		cb_ = that.cb_;
		
		return *this;
	}

	~HttpReqSession()
	{
	}

	void setHttpReqVer(CURL_HTTP_VERSION ver)
	{
		httpVer = ver; 
	}

	void setHttpReqType(CURL_HTTP_REQUEST_TYPE type)
	{
		reqType = type;
	}

	void setHttReqPrivate(void *p)
	{
		private_ = p;
	}

	void setHttpReqConnoutSecond(int second)
	{
		connOutSecond = second;
	}

	void setHttpReqdataoutSecond(int second)
	{
		dataOutSecond = second;
	}

	void setHttpReqInsertMicroSecond(int64_t micro)
	{
		insertMicroSecond = micro;
	}

	void setHttpReqMicroSecond(int64_t micro)
	{
		reqMicroSecond = micro;
	}

	void setHttpRspMicroSecond(int64_t micro)
	{
		rspMicroSecond = micro;
	}
	
	void setHttpResponseCode(int code)
	{
		rspCode = code;
	}

	void setHttpsSslVerifyPeer(int isNeed)
	{
		sslVerifyPeer = isNeed;
	}

	void setHttpsSslVerifyHost(int isNeed)
	{
		sslVeriftHost = isNeed;
	}

	void setHttpResponstBody(const std::string& body)
	{
		rspBody.assign(body);
	}

	void setHttpreqUrl(const std::string& url)
	{
		reqUrl.assign(url);
	}

	void setHttpReqBody(const std::string& body)
	{
		reqData.assign(body);
	}

	void setHttpReqErrorMsg(const std::string& msg)
	{
		errorMsg.assign(msg);
	}

	void addHttpReqPrivateHead(const std::string& head)
	{
		headVec.push_back(head);
	}

	void setHttpReqPrivateHead(const HttpHeadPrivate& head)
	{
		headVec.assign(head.begin(), head.end());
	}

	void setHttpReqCallback(const CurlRespondCallBack& cb)
	{
		cb_ = cb;
	}

	CURL_HTTP_VERSION httpRequestVer()
	{
		return httpVer;
	}

	CURL_HTTP_REQUEST_TYPE httpRequestType()
	{
		return reqType;
	}

	void* httpRequestPrivate()
	{
		return private_;
	}

	long httpReqConnoutSecond()
	{
		return connOutSecond;
	
	}

	long httpReqDataoutSecond()
	{
		return dataOutSecond;
	}

	int64_t httpReqInsertMicroSecond()
	{
		return insertMicroSecond;
	}

	int64_t httpReqMicroSecond()
	{
		return reqMicroSecond;
	}

	int64_t httpRspMicroSecond()
	{
		return rspMicroSecond;
	}

	int httpResponstCode()
	{
		return rspCode;
	}

	int httpsSslVerifyPeer()
	{
		return sslVerifyPeer;
	}

	int httpsSslVerifyHost()
	{
		return sslVeriftHost;
	}

	std::string httpResponseData()
	{
		return rspBody;
	}

	std::string httpRequestUrl()
	{
		return reqUrl;
	}

	std::string httpRequestData()
	{
		return reqData;
	}

	std::string httpReqErrorMsg()
	{
		return errorMsg;
	}

	HttpHeadPrivate httpReqPrivateHead()
	{
		return headVec;
	}
	
	void httpRespondCallBack()
	{
		HttpReqSessionPtr myself(this);//这个地方必须要严格注意，这个对象必须是new出来的，否则会core的
		if(cb_)
		{
			cb_(myself);
		}
		return;
	}

	void setHttpsCacertFile(const std::string& file)
	{
		cacertFile.assign(file);
	}

	std::string httpsCacerFile()
	{
		return cacertFile;
	}

	void setHttpConnInfo(void *conn)
	{
		connInfo_ = conn;
	}

	void* httpConnInfo()
	{
		return connInfo_;
	}

	void setHttpUrlMaxConns(size_t max)
	{
		maxconns_ = max;
	}

	size_t httpUrlmaxConns()
	{
		return maxconns_;
	}

	void setHttpUrlKeepAlive(int isKeep)
	{
		isKeepAlive_ = isKeep;
	}

	int httpUrlIskeepAlive()
	{
		return isKeepAlive_;
	
}
	
private:
	CURL_HTTP_VERSION httpVer;
	CURL_HTTP_REQUEST_TYPE reqType;

	void *private_;//外部请求携带的私有数据
	void *connInfo_;//内部使用，外部不要关心，更不要设值

	size_t maxconns_;//当请求的URL为内部前几次请求的时候，这个值一定要设置
	long connOutSecond;//连接超时时间，秒钟
	long dataOutSecond;//数据传输超时时间，秒钟
	int64_t insertMicroSecond;//调用接口插入队列时间，微秒，所有的时间都是有内部插入，方便外部统计耗时使用
	int64_t reqMicroSecond;//实际请求时间，微秒
	int64_t rspMicroSecond;//实际响应时间，微秒

	int isKeepAlive_;//是否保持长连接，这个参数随URL保持一致。中间不可修改
	int rspCode;//响应编码
	int sslVerifyPeer;///是否校验对端证书
	int sslVeriftHost;//是否校验对端主机ip地址

	std::string rspBody;//响应报文
	std::string reqUrl;//https://192.169.0.61:8888/hello?dadsadada//http://192.169.0.61:8888/hello?dadsadada
	std::string reqData;//请求报文
	std::string errorMsg;//错误信息
	std::string cacertFile;//HTTPS证书全路径
	HttpHeadPrivate headVec;//私有头信息，完整一行，直接按行加在头中
	CurlRespondCallBack cb_;//每个请求的回调函数
};

}

#endif
