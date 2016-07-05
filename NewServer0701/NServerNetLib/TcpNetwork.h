#pragma once

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <deque>
#include "ITcpNetwork.h"

namespace NServerNetLib
{
	class TcpNetwork : public ITcpNetwork
	{
	public:
		TcpNetwork() = default;
		~TcpNetwork() = default;

		NET_ERROR_CODE Init(const ServerConfig* pConfig, ILog* pLogger);
		NET_ERROR_CODE SendData(const int sessionIndex, const short packetId, const short dataSize, const char* pMsg);

		void Run();
		RecvPacketInfo GetPacketInfo();

	private:
		void CreateSessionPool(const int maxClientCount, const int extraClientCount);
		void ConnectSession(const int index, const int clientFD, const char* clientIP);
		void SetSockOption(const int clientFD);

		void AcceptNewSession();
		void RunSelectedSession(fd_set& readSet, fd_set& writeSet, fd_set& exceptSet);
		void CloseSession(const SOCKET_CLOSE_CASE closeCase, const int sessionIndex);

		void AddPacketQueue(const int sessionIndex, const short packetId, const int dataSize, char* pMsg);
		void RecvFromClient(const int sessionIndex);
		NET_ERROR_CODE RecvBufferToPacketQueue(const int sessionIndex);

		void SendFromSession(const int sessionIndex);
		NetError FlushSendBuff(const int sessionIndex);

		ServerConfig m_Config;
		ILog* m_pLogger;
		SOCKET m_serverSocket;
		fd_set m_fdset;

		std::vector<ClientSession*> sessionPool;
		std::deque<int> sessionIndexPool;
		std::deque<RecvPacketInfo> packetQueue;
	};
}