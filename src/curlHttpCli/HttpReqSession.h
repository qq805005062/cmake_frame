#ifndef __XIAO_HTTP_REQ_SESSION_H__
#define __XIAO_HTTP_REQ_SESSION_H__

#include <vector>
#include <memory>
#include <string>
#include <functional>

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
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,rspCode(0)
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
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,rspCode(0)
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
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,rspCode(0)
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
		,reqMicroSecond(0)
		,rspMicroSecond(0)
		,rspCode(0)
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
		reqMicroSecond = that.reqMicroSecond;
		rspMicroSecond = that.rspMicroSecond;
		rspCode = that.rspCode;
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

	void httpRespondCallBack()
	{
		if(cb_)
		{
			cb_(this);
		}
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

private:
	HTTP_VERSION httpVer;
	HTTP_REQUEST_TYPE reqType;
	
	long connOutSecond;//连接超时时间，秒钟
	long dataOutSecond;//数据传输超时时间，秒钟
	int64_t reqMicroSecond;
	int64_t rspMicroSecond;

	int rspCode;//响应编码
	std::string rspBody;//响应报文
	std::string reqUrl;//https://192.169.0.61:8888/hello?dadsadada//http://192.169.0.61:8888/hello?dadsadada
	std::string reqData;//请求报文
	std::string errorMsg;//错误信息
	HttpHeadPrivate headVec;//私有头信息，完整一行，直接按行加在头中
	CurlRespondCallBack cb_;//每个请求的回调函数
};

}

#endif
