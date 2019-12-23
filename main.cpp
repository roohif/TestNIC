#include<locale.h>
#include <iostream>

#include "TestNIC.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <local_ip>" << std::endl;
		return 1;
	}

	std::string local_ip = argv[1];

	try
	{
		TestNIC::TestNIC nic;
		nic.ListenMulticast(local_ip);
		nic.DoEventLoop();
	}
	catch (std::exception& e)
	{
		std::cout << "main() :: " << e.what();
	}

}
