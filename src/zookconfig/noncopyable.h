#ifndef __ZOO_CONFIG_TOPICS_NONCOPYABLE_H__
#define __ZOO_CONFIG_TOPICS_NONCOPYABLE_H__

namespace ZOOKCONFIG
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

} // end namespace ZOOKCONFIG

#endif // __ZOO_CONFIG_TOPICS_NONCOPYABLE_H__