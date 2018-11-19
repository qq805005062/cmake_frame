#ifndef __XIAO_CONNINFO_H__
#define __XIAO_CONNINFO_H__

#include <curl/curl.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#include <memory.h>

#include <string>

#include "../HttpReqSession.h"
#include "../CurlHttpCli.h"

//#include "ConnInfo.h"

#define CURL_ERROR_SIZE 256

namespace CURL_HTTP_CLI
{

/* Global information, common to all connections */
typedef struct _GlobalInfo
{
	struct event_base *evbase;
	struct event wake_event;
	struct event timer_event;
	CURLM *multi;
	int still_running;
	int stopped;
} GlobalInfo;

/* Information associated with a specific socket */
typedef struct _SockInfo
{
	curl_socket_t sockfd;
	CURL *easy;
	int action;
	long timeout;
	struct event ev;
	GlobalInfo *global;
} SockInfo;

/* Information associated with a specific easy handle */
class ConnInfo
{
public:
	ConnInfo(size_t subIndex)
		:isNew_(true)
		,easy(nullptr)
		,global(nullptr)
		,headers(nullptr)
		,reqInfo(nullptr)
		,error(nullptr)
		,reqUrl(nullptr)
		,subIndex_(subIndex)
		,urlSize(0)
		,outSecond(0)
		,rspdata()
	{
	}

	~ConnInfo()
	{
		if(global)
		{
			global = NULL;
		}

		if(headers)
		{
			curl_slist_free_all(headers);
			headers = NULL;
		}
		
		if(easy)
		{
			curl_easy_cleanup(easy);
			easy = NULL;
		}

		if(reqInfo)
		{
			WARN("may lost request");
		}

		if(error)
		{
			delete[] error;
			error = NULL;
		}

		if(reqUrl)
		{
			urlSize = 0;
			delete[] reqUrl;
			reqUrl = NULL;
		}
	}

	int connInfoInit()
	{
		int ret = 0;
		if(error == NULL)
		{
			error = static_cast<char *>(malloc(CURL_ERROR_SIZE));
			if(error)
			{
				memset(error, 0, CURL_ERROR_SIZE);
			}else{
				return -1;
			}
		}

		easy = curl_easy_init();
		if(easy == NULL)
		{
			return -1;
		}

		return ret;
	}

	int connInfoReinit()
	{
		outSecond = 0;
		reqInfo = NULL;
		
		memset(error, 0, CURL_ERROR_SIZE);
		global = NULL;
		rspdata.assign("");
		
		return 0;
	}

	void connMultiRemoveHandle()
	{
		if(global)
		{
			curl_multi_remove_handle(global->multi, easy);
		}
	}

	void connInfoSetEasy(CURL *e)
	{
		easy = e;
	}

	CURL* connInfoEasy()
	{
		return easy;
	}

	void connInfoSetGlobal(GlobalInfo* g)
	{
		global = g;
	}

	GlobalInfo* connInfoGlobal()
	{
		return global;
	}

	void connInfoSetHeader(struct curl_slist *h)
	{
		headers = h;
	}

	void connInfoSetReqinfo(HttpReqSession* req)
	{
		reqInfo = req;
	}

	HttpReqSession* connInfoReqinfo()
	{
		return reqInfo;
	}

	char* connInfoErrorMsg()
	{
		return error;
	}

	void connInfoSetOutSecond(int64_t second)
	{
		outSecond = second;
	}

	int64_t connInfoOutSecond()
	{
		return outSecond;
	}

	void connInfoSetRspbody(const char *pChar, size_t bodyLen)
	{
		rspdata.assign(pChar, bodyLen);
	}

	void connInfoAppendRspbody(const char *pChar, size_t bodyLen)
	{
		rspdata.append(pChar, bodyLen);
	}

	std::string connInfoRspBody()
	{
		return rspdata;
	}

	int connInfoSetReqUrl(const std::string& url)
	{
		int ret = 0;
		if(urlSize == 0)
		{
			if(url.length() >= 1024)
			{
				urlSize = url.length() + 1;
			}else{
				urlSize = 1024;
			}
			reqUrl = static_cast<char *>(malloc(urlSize));
			if(reqUrl)
			{
				memset(reqUrl, 0, urlSize);
				memcpy(reqUrl, url.c_str(), url.size());
			}else{
				ret = -1;
			}
		}else{
			if(url.length() >= urlSize)
			{
				delete[] reqUrl;
				urlSize = url.length() + 1;
				reqUrl = static_cast<char *>(malloc(urlSize));
				if(reqUrl == NULL)
				{
					ret = -1;
				}
			}
			if(ret == 0)
			{
				memset(reqUrl, 0, urlSize);
				memcpy(reqUrl, url.c_str(), url.size());
			}
		}
		return ret;
	}

	char* connInfoReqUrl()
	{
		return reqUrl;
	}

	size_t ioThreadSubindex()
	{
		return subIndex_;
	}

	void setIoThreadSubindex(size_t subIndex)
	{
		subIndex_ = subIndex;
	}

	bool connInfoIsNew()
	{
		bool ret = isNew_;
		isNew_ = false;
		return ret;
	
}
private:
	bool isNew_;
	CURL *easy;
	GlobalInfo *global;
	struct curl_slist *headers;
	HttpReqSession* reqInfo;
	char *error;
	char *reqUrl;
	
	size_t subIndex_;
	size_t urlSize;
	
	int64_t outSecond;
	std::string rspdata;
};
typedef std::shared_ptr<ConnInfo> ConnInfoPtr;

}

#endif

