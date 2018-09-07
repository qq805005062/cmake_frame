#ifndef __CURL_NONCOPYABLE_H__
#define __CURL_NONCOPYABLE_H__

namespace CURLHTTPCLI
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

} // end namespace CURLHTTPCLI

#endif // __CURL_NONCOPYABLE_H__