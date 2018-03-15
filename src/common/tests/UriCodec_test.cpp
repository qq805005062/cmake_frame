#include <common/UriCodec.h>
#include <iostream>
#include <libgen.h>

using namespace std;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s encode|decode\n", basename(argv[0]));
		return -1;
	}

	std::string codeType = argv[1];
	if ("encode" == codeType)
	{
		uint64_t msgId = 0;
		cout << "输入要转码的UINT64数据:" << endl;
		cin >> msgId;
		std::string msgIdStr = common::encode(msgId);
		cout << "encode msgId: " << msgId << ", msgIdStr = " << msgIdStr << endl;
	}
	else if ("decode" == codeType)
	{
		std::string msgIdStr;
		cout << "输入要转码的串:" << endl;
		cin >> msgIdStr;
		uint64_t msgId = common::decode(msgIdStr);
		cout << "decode msgIdStr: " << msgIdStr << ", msgId = " << msgId << endl;
	}
	else
	{
		fprintf(stderr, "编码方式必须为：encode|decode\n");
		return -1;
	}
	return 0;
}