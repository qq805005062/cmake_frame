#ifndef __XIAO_NONCOPYABLE_H__
#define __XIAO_NONCOPYABLE_H__

namespace CURL_HTTP_CLI
{

class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;

private:
	noncopyable(const noncopyable& ) = delete;
	const noncopyable& operator=(const noncopyable& ) = delete;
};

} // end namespace CURL_HTTP_CLI

#endif // __XIAO_NONCOPYABLE_H__

