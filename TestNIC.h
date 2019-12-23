#pragma once

#include <cstring>
#include <iostream>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

// Timestamping enums
#include <linux/net_tstamp.h>

namespace TestNIC
{
	class TestNIC
	{
	public:
		void EnableTimestamping(int sock);
		void ListenMulticast(std::string local_ip);
		void DoEventLoop();
		void ReadPacket(int sock);

	private:
		int m_udp_feed;
		int m_tcp_socket;

		char *read_buffer;
	};
}
