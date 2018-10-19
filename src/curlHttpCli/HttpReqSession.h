#ifndef __XIAO_HTTP_REQ_SESSION_H__
#define __XIAO_HTTP_REQ_SESSION_H__

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "src/Atomic.h"

typedef enum{
	HTTP10 = 0,
	HTTP11,
	HTTPS
}HTTP_VERSION;

typedef enum{
	HTTP_GET = 0,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_UPDATE,
	HTTP_UNKONOW
}HTTP_REQUEST_TYPE;

typedef std::vector<std::string> HttpHeadPrivate;
typedef HttpHeadPrivate::iterator HttpHeadPrivateIter;

namespace CURL_HTTP_CLI
{

class HttpReqSession;
typedef std::function<void(CURL_HTTP_CLI::HttpReqSession* curlReq)> CurlRespondCallBack;


class HttpReqSession
{
public:
	HttpReqSession()
		:httpVer(HTTP11)
		,reqType(HTTP_POST)
		,connOutSecond(3)
		,dataOutSecond(6)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,outSecond(0)
		,isCallBack()
		,callbackFlag(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl()
		,reqData()
		,errorMsg()
		,headVec()
		,cb_(nullptr)
	{
	}

	HttpReqSession(HTTP_VERSION ver, HTTP_REQUEST_TYPE type, const std::string& url)
		:httpVer(ver)
		,reqType(type)
		,connOutSecond(3)
		,dataOutSecond(6)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,outSecond(0)
		,isCallBack()
		,callbackFlag(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl(url)
		,reqData()
		,errorMsg()
		,headVec()
		,cb_(nullptr)
	{
	}

	HttpReqSession(HTTP_VERSION ver, HTTP_REQUEST_TYPE type, const std::string& url, const std::string& body)
		:httpVer(ver)
		,reqType(type)
		,connOutSecond(3)
		,dataOutSecond(6)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,outSecond(0)
		,isCallBack()
		,callbackFlag(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl(url)
		,reqData(body)
		,errorMsg()
		,headVec()
		,cb_(nullptr)
	{
	}

	HttpReqSession(const HttpReqSession& that)
		:httpVer(HTTP11)
		,reqType(HTTP_POST)
		,connOutSecond(3)
		,dataOutSecond(6)
		,insertMicroSecond(0)
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,outSecond(0)
		,isCallBack()
		,callbackFlag(0)
		,rspCode(0)
		,sslVerifyPeer(0)
		,sslVeriftHost(0)
		,rspBody()
		,reqUrl()
		,reqData()
		,errorMsg()
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
		connOutSecond = that.connOutSecond;
		dataOutSecond = that.dataOutSecond;
		insertMicroSecond = that.insertMicroSecond;
		reqMicroSecond = that.reqMicroSecond;
		rspMicroSecond = that.rspMicroSecond;
		outSecond = that.outSecond;
		isCallBack.getAndSet(that.isCallBack.get());
		callbackFlag = that.callbackFlag;
		rspCode = that.rspCode;
		sslVerifyPeer = that.sslVerifyPeer;
		sslVeriftHost = that.sslVeriftHost;
		rspBody = that.rspBody;
		reqUrl = that.reqUrl;
		reqData = that.reqData;
		errorMsg = that.errorMsg;
		headVec = that.headVec;
		cb_ = that.cb_;
		
		return *this;
	}

	~HttpReqSession()
	{
	}

	void setHttpReqVer(HTTP_VERSION ver)
	{
		httpVer = ver; 
	}

	void setHttpReqType(HTTP_REQUEST_TYPE type)
	{
		reqType = type;
	}

	void setHttpReqConnoutSecond(int second)
	{
		connOutSecond = second;
	}

	void setHttpReqdataoutSecond(int second)
	{
		dataOutSecond = second;
	}

	void setHttpResponseCode(int code)
	{
		rspCode = code;
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

	uint32_t httpRespondCallBack()
	{
		uint32_t call = isCallBack.incrementAndGet();
		if(cb_ && call == 1)
		{
			cb_(this);
		}
		callbackFlag = 1;
		return call;
	}

	HttpHeadPrivate httpReqPrivateHead()
	{
		return headVec;
	}

	std::string httpReqErrorMsg()
	{
		return errorMsg;
	}

	std::string httpRequestData()
	{
		return reqData;
	}

	std::string httpRequestUrl()
	{
		return reqUrl;
	}

	std::string httpResponseData()
	{
		return rspBody;
	}

	int httpResponstCode()
	{
		return rspCode;
	}

	long httpReqConnoutSecond()
	{
		return connOutSecond;
	}

	long httpReqDataoutSecond()
	{
		return dataOutSecond;
	}

	HTTP_VERSION httpRequestVer()
	{
		return httpVer;
	}

	HTTP_REQUEST_TYPE httpRequestType()
	{
		return reqType;
	}

	void setHttpReqMicroSecond(int64_t micro)
	{
		reqMicroSecond = micro;
	}

	void setHttpRspMicroSecond(int64_t micro)
	{
		rspMicroSecond = micro;
	}

	int64_t httpReqMicroSecond()
	{
		return reqMicroSecond;
	}

	int64_t httpRspMicroSecond()
	{
		return rspMicroSecond;
	}

	void setHttpReqInsertMicroSecond(int64_t micro)
	{
		insertMicroSecond = micro;
	}

	int64_t httpReqInsertMicroSecond()
	{
		return insertMicroSecond;
	}

	void setHttpsSslVerifyPeer(int isNeed)
	{
		sslVerifyPeer = isNeed;
	}

	void setHttpsSslVerifyHost(int isNeed)
	{
		sslVeriftHost = isNeed;
	}

	int httpsSslVerifyPeer()
	{
		return sslVerifyPeer;
	}

	int httpsSslVerifyHost()
	{
		return sslVeriftHost;
	}

	void setHttpOutSecond(int64_t second)
	{
		outSecond = second;
	}

	int64_t httpRequestOutSecond()
	{
		return outSecond;
	}

	int httpResponseCallFlag()
	{
		return callbackFlag;
	}
	
private:
	HTTP_VERSION httpVer;
	HTTP_REQUEST_TYPE reqType;
	
	long connOutSecond;//连接超时时间，秒钟
	long dataOutSecond;//数据传输超时时间，秒钟
	int64_t insertMicroSecond;//调用接口插入队列时间，微秒
	int64_t reqMicroSecond;//实际请求时间，微秒
	int64_t rspMicroSecond;//实际响应时间，微秒
	int64_t outSecond;

	mutable AtomicUInt32 isCallBack;

	int callbackFlag;
	int rspCode;//响应编码
	int sslVerifyPeer;///是否校验对端证书
	int sslVeriftHost;//是否校验对端主机ip地址
	std::string rspBody;//响应报文
	std::string reqUrl;//https://192.169.0.61:8888/hello?dadsadada//http://192.169.0.61:8888/hello?dadsadada
	std::string reqData;//请求报文
	std::string errorMsg;//错误信息
	HttpHeadPrivate headVec;//私有头信息，完整一行，直接按行加在头中
	CurlRespondCallBack cb_;//每个请求的回调函数
};

}

#endif
