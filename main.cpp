#include<locale.h>
#include <iostream>

// #include <Core/Utility.h>
#include <Core/Platform.h>

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
		#ifdef _WIN64
		WSADATA data;
		int rc = WSAStartup(MAKEWORD(2, 2), &data);
		std::cout << "WSAStartup() = " << rc;
		#endif

		TestNIC::TestNIC nic;
		nic.ListenMulticast(local_ip);
		nic.DoEventLoop();
	}
	catch (std::exception& e)
	{
		std::cout << "main() :: " << e.what();
	}

}
