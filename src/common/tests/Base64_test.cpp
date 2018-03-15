#include <common/Base64.h>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "Usage: " << argv[0] << " " << "encode|decode" << endl;
		return -1;
	}

	string codeType = argv[1];
	if (codeType == "encode")
	{
		cout << "输入你要转Base64的原串: " << endl;
		string buffer, result;
		cin >> buffer;
		common::Base64::Encode(buffer, &result);
		cout << "Base64编码后的串: " << result << endl; 
	}
	else if (codeType == "decode")
	{
		cout << "输入你要解码的Base64串: " << endl;
		string buffer, result;
		cin >> buffer;
		common::Base64::Decode(buffer, &result);
		cout << "Base64解码后的值为: " << result << endl; 
	}
	else
	{
		cout << "error code type" << endl;
		return -1; 
	}
	return 0;
}
