#ifndef __CURL_HTTP_SESSION_H__
#define __CURL_HTTP_SESSION_H__

#include <memory>
#include <vector>

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


class CurlHttpRequest;
typedef std::shared_ptr<CurlHttpRequest> CurlHttpRequestPtr;
typedef std::function<void(const CurlHttpRequestPtr& curlReq)> CurlRespondCallBack;//回调里面千万不能阻塞，如果有长时间操作最好可以丢队列里面

typedef std::vector<std::string> HttpHeadPrivate;
typedef HttpHeadPrivate::iterator HttpHeadPrivateIter;

class CurlHttpRequest
{

public:
	CurlHttpRequest()
		:httpVer(HTTP11)
		,reqType(HTTP_UNKONOW)
		,rspCode(0)
		,rspBody()
		,reqUrl()
		,reqData()
		,headVec()
		,cb_(nullptr)
	{
	}
	
	~CurlHttpRequest()
	{
	}

	void setCurlReqVersion(HTTP_VERSION v)
	{
		httpVer = v;
	}

	void setCurlReqType(HTTP_REQUEST_TYPE t)
	{
		reqType = t;
	}

	void setCurlReqUrl(const std::string& u)
	{
		reqUrl = u;
	}

	void setCurlReqBody(const std::string& body)
	{
		reqData = body;
	}

	void setCurlRspCode(int code)
	{
		rspCode = code;
	}

	void setCurlRspBody(const std::string& body)
	{
		rspBody = body;
	}
	
	std::string curlHttpUrl() const
	{
		return reqUrl;
	}

	HTTP_VERSION curlHttpVersion() const
	{
		return httpVer;
	}

	HTTP_REQUEST_TYPE curlHttpReqType() const
	{
		return reqType;
	}
	
	std::string curlHttpData() const
	{
		return reqData;
	}

	int curlRspCode() const
	{
		return rspCode;
	}

	std::string curlRspBody() const
	{
		return rspBody;
	}

	void addCurlHttpHead(const std::string& head)
	{
		headVec.push_back(head);
	}
	
	void setCurlHttpHead(const HttpHeadPrivate& head)
	{
		headVec.assign(head.begin(), head.end());
	}

	HttpHeadPrivate curlHttpHead()
	{
		return headVec;
	}

	void setCurlHttpPrivate(void *p)
	{
		private = p;
	}

	void* curlHttpPrivate()
	{
		return private;
	}

	void setRespondCallback(CurlRespondCallBack cb)
	{
		cb_ = cb;
	}

	void curlRespondCallBack(const CurlHttpRequestPtr& rsp)
	{
		if(cb_)
			cb_(rsp);
	}

private:
	
	HTTP_VERSION httpVer;
	HTTP_REQUEST_TYPE reqType;
	int rspCode;
	std::string rspBody;
	std::string reqUrl;//https://192.169.0.61:8888/hello?dadsadada//http://192.169.0.61:8888/hello?dadsadada
	std::string reqData;
	HttpHeadPrivate headVec;
	CurlRespondCallBack cb_;

	void* private;
};

#endif