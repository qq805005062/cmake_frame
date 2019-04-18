#ifndef MW_TIMESTAMP_H
#define MW_TIMESTAMP_H

#include <boost/any.hpp>
#include <string>
#include <sys/time.h>
#include <stdio.h>

namespace MWTIMESTAMP
{
class Timestamp
{
public:
	static const int kMicroSecondsPerSecond = 1000 * 1000;
public:
	Timestamp();
	Timestamp(int64_t microSecondsSinceEpochArg);
	~Timestamp();

	// ��������ʱ�佻��
	void Swap(Timestamp& that)
	{
		std::swap(m_any, that.m_any);
	}

	// default copy/assignment/dtor are Okay
	std::string ToString() const;

	// ��ʽ��ʱ���ַ�����1970-01-01 08:00:00.000
	// showMicroseconds:�Ƿ���ʾ΢��
	std::string ToFormattedString(bool showMicroseconds = true) const;

	bool Valid() const;

	// for internal usage.
	int64_t MicroSecondsSinceEpoch() const;
	time_t SecondsSinceEpoch() const;

	// �Ե�ǰʱ������һ��Timestamp����
	static Timestamp Now();

	//����һ��Ĭ�ϵ�ʱ������� 1970-01-01 08:00:00
	static Timestamp Invalid();

	// ��һ��time_t������Timestamp����
	static Timestamp FromUnixTime(time_t t);
	static Timestamp FromUnixTime(time_t t, int microseconds);

private:
	boost::any m_any;
	
};

inline bool operator<(const Timestamp& lhs, const Timestamp& rhs)
{
	return lhs.MicroSecondsSinceEpoch() < rhs.MicroSecondsSinceEpoch();
}

inline bool operator>(const Timestamp& lhs, const Timestamp& rhs)
{
	return lhs.MicroSecondsSinceEpoch() > rhs.MicroSecondsSinceEpoch();
}

inline bool operator==(const Timestamp& lhs, const Timestamp& rhs)
{
	return lhs.MicroSecondsSinceEpoch() == rhs.MicroSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline double TimeDifference(Timestamp high, Timestamp low)
{
	int64_t diff = high.MicroSecondsSinceEpoch() - low.MicroSecondsSinceEpoch();
	return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp AddTime(Timestamp timestamp, double seconds)
{
	int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
	return Timestamp(timestamp.MicroSecondsSinceEpoch() + delta);
}
}
#endif // !MW_TIMESTAMP_H

// �÷�����
/*
#include "../MWTimestamp.h"
#include <iostream>

using namespace MWTIMESTAMP;

int main()
{
	Timestamp t1 = Timestamp::Now();
	Timestamp t2;
	t2.Swap(t1);
	std::cout << "now-->" << t1.ToFormattedString() << std::endl;
	std::cout << "t1 cmp t2-->" << (t2 == t1) << std::endl;

	return 0;
}
*/