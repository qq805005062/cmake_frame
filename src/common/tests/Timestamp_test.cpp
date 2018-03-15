#include <iostream>
#include <common/Timestamp.h>

using namespace std;

int main(int argc, char* argv[])
{
	common::Timestamp now = common::Timestamp::now();
	cout << "now.toString(): " << now.toString() << endl;
	cout << "now.toFormattedString(true): " << now.toFormattedString(true) << endl;
	cout << "now.toFormattedString(false): " << now.toFormattedString(false) << endl;
	cout << "now.secondsSinceEpoch(): " << now.secondsSinceEpoch() << endl;
	
	common::Timestamp now2 = common::Timestamp::now();
	if (now2 > now)
	{
		cout << "now2 > now is OK" << endl;
	}
	double diff = common::timeDifference(now2, now);
	cout << "diff = " << diff << endl;
	return 0;
}
