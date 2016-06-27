#pragma once

#define FD_SETSIZE 1024 // http://blog.naver.com/znfgkro1/220175848048

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <deque>
#include <unordered_map>

#include "Define.h"
#include "ServerErrorCode.h"
#include "ILog.h"

namespace ServerNetworkLib {

	class TCPServer
	{
	public:
		TCPServer() = default;
		~TCPServer() = default;

		NET_ERROR_CODE Init(const ServerConfig* config, ILog* logger);
		void Run();

		NET_ERROR_CODE AcceptNewSession();

	private:
		void RunSelectedSocket(fd_set readfd, fd_set writefd, fd_set execfd);
		void CreateSessionPool(const int maxClientCount);
		int AllocNewSessionIndex();

		void ConnectSession(int sessionIndex, int clientSocket, const char* clientIP);
		void CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex);
		void AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos);

		// º¯¼ö
		ServerConfig	serverConfig;
		SOCKET			serverSocket;

		ILog*			serverLogger;
		fd_set			read_set;
		
		std::vector<ClientSession>	clientSessionPool;
		std::deque<int>			clientSessionPoolIndex;
		std::deque<RecvPacketInfo>	packetQueue;
	};

}

