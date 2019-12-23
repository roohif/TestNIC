#include "TestNIC.h"

namespace TestNIC
{
	void TestNIC::EnableTimestamping(int sock)
	{
		int flags =
			SOF_TIMESTAMPING_RX_HARDWARE
			| SOF_TIMESTAMPING_TX_HARDWARE
			| SOF_TIMESTAMPING_RAW_HARDWARE;
 			// | SOF_TIMESTAMPING_SYS_HARDWARE; // Apparently this option is deprecated?

		std::cout << "Selecting hardware timestamping mode 0x";
		std::cout << std::hex << flags << std::dec << " [fd = " << sock << "]" << std::endl;

		int rc = setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof(flags));
		std::cout << "Return code from setsockopt() is [rc = " << rc << "]" << std::endl;

		if (true)
		{
			int enable = 1;
			rc = setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &enable, sizeof(enable));
			std::cout << "Return code of SO_TIMESTAMP [rc = " << rc << "]" << std::endl;
		}

		if (true)
		{
			int enable = 1;
			rc = setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS, &enable, sizeof(enable));
			std::cout << "Return code of SO_TIMESTAMPNS [rc = " << rc << "]" << std::endl;
		}
	}

	void TestNIC::ReadPacket(int sock)
	{
		struct msghdr msg;
		struct sockaddr_in host_address;
		struct iovec iov;
		int flags = 0;

		char control[1024]; // Buffer for CMSGs
		char buffer[4000]; // Data Buffer

		host_address.sin_family = AF_INET;
		host_address.sin_port = htons(15516);
		host_address.sin_addr.s_addr = INADDR_ANY;

		msg.msg_namelen = sizeof(struct sockaddr_in); // Needs to be set each call
		msg.msg_name = &host_address;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1; // One buffer
		iov.iov_base = buffer;
		iov.iov_len = 4000;

		msg.msg_control = control;
		msg.msg_controllen = 1024; // Needs to be set each call

		// Do a recvmsg()
		ssize_t bytes_rcvd = recvmsg(sock, &msg, flags);

		std::cout << "Bytes received = " << bytes_rcvd << std::endl;
		std::cout << "Length of CMSGs = " << msg.msg_controllen << std::endl;
		std::cout << "Message flags: " << msg.msg_flags << std::endl;

		std::cout << "Attempting to extract timestamps ..." << std::endl;

		struct cmsghdr* cmsg;
		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
		{
			std::cout << "CMSG level = " << cmsg->cmsg_level << std::endl;
			std::cout << "CMSG type = " << cmsg->cmsg_type << std::endl;

			if (cmsg->cmsg_type == 35) // SO_TIMESTAMPNS
			{
				// SO_TIMESTAMPNS
				struct timespec *ts = (struct timespec *)CMSG_DATA(cmsg);
				std::cout << "SO_TIMESTAMPNS: [sec = " << ts->tv_sec << "]";
				std::cout << "[nsec = " << ts->tv_nsec << "]" << std::endl;
			}

			if (cmsg->cmsg_type == 37) // SO_TIMESTAMPING
			{
				struct timespec *ts = (struct timespec *)CMSG_DATA(cmsg);
				std::cout << "SO_TIMESTAMPING:" << std::endl;

				std::cout << "SW  Timespec [sec = " << ts[0].tv_sec << "]";
				std::cout << "[nsec = " << ts[0].tv_nsec << "]" << std::endl; // 0 = Software

				std::cout << "HW  Timespec [sec = " << ts[1].tv_sec << "]";
				std::cout << "[nsec = " << ts[1].tv_nsec << "]" << std::endl; // 1 = HW transformed

				std::cout << "RAW Timespec [sec = " << ts[2].tv_sec << "]";
				std::cout << "[nsec = " << ts[2].tv_nsec << "]" << std::endl; // 2 = HW Raw
			}
		}


	}

	void TestNIC::ListenMulticast(std::string local_ip)
	{
		// Create the buffer
		read_buffer = new char[1000];
		const std::string group_ip = "233.37.54.162";
		const int port = 15516;

		// Create a UDP socket and listen for packets
		m_udp_feed = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_udp_feed < 0)
		{
			std::cout << "socket creation failed" << std::endl;
			throw new std::runtime_error("KRX socket creation failed");
		}

		std::cout << "Created socket [fd = " << m_udp_feed << "]" << std::endl;

		// Enable SO_REUSEADDR to allow multiple instances of this
		// application to receive copies of the multicast datagrams.
		if (true)
		{
			int reuse = 1;
			setsockopt(m_udp_feed, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
			std::cout << "Reuse address" << std::endl;
		}

		// http://stackoverflow.com/questions/9243292/subscribing-to-multiple-multicast-groups-on-one-socket-linux-c

		// Bind to the multicast port
		struct sockaddr_in host_address;
		memset(&host_address, 0, sizeof(host_address));
		host_address.sin_family = AF_INET;

		// Bind to the *GROUP* address in the case of multicast UDP.
		host_address.sin_addr.s_addr = inet_addr(group_ip.c_str()); // Linux
		host_address.sin_port = htons(port);

		// Using bind from the global namespace, not std::bind
		int rc = ::bind(m_udp_feed, (struct sockaddr*)&host_address, sizeof(host_address));
		if (rc < 0)
		{
			std::cout << "Socket binding failed" << std::endl;
			throw new std::runtime_error("Socket binding failed");
		}

		std::cout << "Bound" << std::endl;

		// Join the Multicast Group
		// For the machine to receive multicast packets, I needed to allow IP forwarding
		// and disable rp_filter, both in /etc/sysctl.conf
		ip_mreq request;
		request.imr_multiaddr.s_addr = inet_addr(group_ip.c_str());
		request.imr_interface.s_addr = inet_addr(local_ip.c_str());

		std::cout << "About to join Multicast group [group = " << group_ip << "]" << std::endl;

		int am = setsockopt(m_udp_feed, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&request, sizeof(request));
		if (am < 0)
		{
			std::cout << "Failed to join Multicast group :: "
				<< group_ip << " on " << local_ip;
			throw new std::runtime_error("Failed to join Multicast group");
		}
		std::cout << "Return code to join multicastgroup [rc = " << rc << "]" << std::endl;

		// Success!
		std::cout << "Socket created, listening on " << group_ip << ":" << port;
		std::cout << " [fd = " << m_udp_feed << "]" << std::endl;
		std::cout << "Trying to enable timestamps ..." << std::endl;
		EnableTimestamping(m_udp_feed);
	}

	void TestNIC::DoEventLoop()
	{
		struct epoll_event evt;
		struct epoll_event *evts;

		int epfd = epoll_create(1);
		if (epfd == -1)
			std::cout << "epoll_create() failed";

		std::cout << "epoll created [epfd = " << epfd << "]" << std::endl;

		evt.data.fd = m_udp_feed;
		evt.events = EPOLLIN | EPOLLET;
		int rc = epoll_ctl(epfd, EPOLL_CTL_ADD, m_udp_feed, &evt);

		std::cout << "Added socket to epoll" << std::endl;

		while (true)
		{
			int rc = epoll_wait(epfd, &evt, 1, 5000 /* milliseconds */);

			if (rc > 0)
			{
				std::cout << "Packet received" << std::endl;
				ReadPacket(m_udp_feed);
			}

			if (rc == 0)
				std::cout << "epoll_wait() timed out" << std::endl;

			if (rc == -1)
			{
				std::cout << "epoll_wait() failed [error = " << errno << "]";
				return;
			}

		} // while (true)
	}

}
