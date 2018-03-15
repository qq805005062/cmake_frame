
#ifndef __RMS_USER_INPUT_PASS_H__
#define __RMS_USER_INPUT_PASS_H__

#include <iostream>
#include <string>

namespace common
{

inline std::string userInputPassword()
{
	std::string password = "";
	std::cout << "please input mysql password:";

	for (std::string line; std::getline(std::cin, line); line.clear())
	{
		if(password.length() > 0)
		{
			if(line == "y")
				break;
			else
			{
				password.clear();
				std::cout << "please input mysql password:";
				continue;
			}
		}
		if(line.length() > 0)
		{
			password = line;
			std::cout << "you put input mysql password is :: \""<< password << "\".Are you sure?(y/n)";
		}
	}
	return password;
}

}
#endif