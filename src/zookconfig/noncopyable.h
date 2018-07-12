#ifndef __ZOO_KFK_TOPICS_NONCOPYABLE_H__
#define __ZOO_KFK_TOPICS_NONCOPYABLE_H__

namespace ZOOKEEPERKAFKA
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

} // end namespace ZOOKEEPERKAFKA

#endif // __ZOO_KFK_TOPICS_NONCOPYABLE_H__