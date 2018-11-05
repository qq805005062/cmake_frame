#ifndef __LIBEVENT_TCPCLI_NONCOPYABLE_H__
#define __LIBEVENT_TCPCLI_NONCOPYABLE_H__

namespace LIBEVENT_TCP_CLI
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

} // end namespace LIBEVENT_TCP_CLI

#endif // __LIBEVENT_TCPCLI_NONCOPYABLE_H__

