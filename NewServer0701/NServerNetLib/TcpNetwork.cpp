#include "TcpNetwork.h"

namespace NServerNetLib
{
	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* pConfig, ILog* pLogger)
	{
		memcpy(&m_Config, pConfig, sizeof(ServerConfig));
		m_pLogger = pLogger;

		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		// socket()
		// ���� ������ �� ������ fd�� �޽��ϴ�.
		m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_serverSocket == INVALID_SOCKET)
		{
			m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | socket function error", __FUNCTION__);
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		// ���� �ɼ� ����
		auto n = 1;
		if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | setsockopt function error", __FUNCTION__);
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		// sockaddr_in struct init
		// �ش� ����ü�� ������ �ּ� ������ �����մϴ�.
		// bind�� ���Ǹ�, Windows ȯ�濡���� SOCKADDR�� ���� ũ���� ����ü�� SOCKADDR_IN�� ����մϴ�.
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(m_Config.port);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// bind()
		// ���� ���Ͽ� ���� �ּ� ����ü ������ bind ���ݴϴ�.
		if (bind(m_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		{
			m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | bind function error", __FUNCTION__);
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}

		// listen()
		// ���� ������ Ŭ���̾�Ʈ�κ��� connect�� ������ �� �ֵ���
		// listen �Լ��� �������ݴϴ�.
		if (listen(m_serverSocket, m_Config.backLogCount) == SOCKET_ERROR)
		{
			m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | listen function error", __FUNCTION__);
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		// TCPServer�� ��������� ���� �ִ� fd_set�� �ʱ�ȭ���ݴϴ�
		FD_ZERO(&m_fdset);
		FD_SET(m_serverSocket, &m_fdset);

		// sessionPool ����
		// client�κ��� �޾ƿ� ���� ������ ������ Ǯ�� ����
		// ó���� client ���� �ִ밪 + ���а��� �Ҵ��Ѵ�
		// �߿��� ���ε�, �ϴ� ���б��� accept�Ͽ� ������ ���� �Ҵ��ϵ�, ������ �����ϴ� ���� �׺��� �۰� �����ϰڴٴ� ��
		// ������ �濡 �� client���� pool�� �� á���� ���ư���� �Ҹ��� ��� �ȴ�.
		CreateSessionPool(m_Config.maxClientCount, m_Config.extraClientCount);

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetId, const short dataSize, const char* pMsg)
	{
		auto& client = sessionPool[sessionIndex];
		
		int writePos = client->sendDataSize;

		// send buffer�� ���� ���� ������ ���մϴ�
		if (m_Config.maxClientSendBufferSize < writePos + dataSize + PACKET_HEADER_SIZE)
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;

		PacketHeader pktHeader{ packetId, dataSize };
		memcpy(&client->pSendBuffer[writePos], (char*)&pktHeader, PACKET_HEADER_SIZE);
		//std::copy(&(char*)(pktHeader)[0], &pktHeader[PACKET_HEADER_SIZE], &pktHeader);
		std::copy(&pMsg[0], &pMsg[dataSize], &client->pSendBuffer[writePos+ PACKET_HEADER_SIZE]);
		client->sendDataSize += (PACKET_HEADER_SIZE + dataSize);
		
		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::Run()
	{
		fd_set readSet = m_fdset;
		fd_set writeSet = m_fdset;
		fd_set exceptSet = m_fdset;

		timeval timeout{ 0, 1000 };

		auto result = select(0, &readSet, &writeSet, &exceptSet, &timeout);
		if (result < 0)
		{
			// log ���
		}

		if (result == 0)
		{
			return;
		}

		if(FD_ISSET(m_serverSocket, &readSet))
		{
			AcceptNewSession();
		}
		else
		{
			RunSelectedSession(readSet, writeSet, exceptSet);
		}
	}

	void TcpNetwork::CreateSessionPool(const int maxClientCount, const int extraClientCount)
	{
		for (int i = 0; i < maxClientCount + extraClientCount; ++i)
		{
			ClientSession* session = new ClientSession();
			session->pRecvBuffer = new char[MAX_BUFFER_LENGTH];
			session->pSendBuffer = new char[MAX_BUFFER_LENGTH];
			sessionPool.push_back(session);
		}

		for (int i = 0; i < maxClientCount; ++i)
		{
			sessionIndexPool.push_back(i);
		}
	}

	void TcpNetwork::AcceptNewSession()
	{
		if (sessionIndexPool.empty())
		{
			// m_Logger->
			return;
		}

		SOCKADDR_IN clientAddr;
		int clientAddrLength = sizeof(clientAddr);
		auto clientFD = accept(m_serverSocket, (SOCKADDR*)&clientAddr, &clientAddrLength);

		if (clientFD <= 0)
		{
			// m_pLogger->
			return;
		}

		char clientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, MAX_IP_LEN -1);

		SetSockOption(clientFD);

		FD_SET(clientFD, &m_fdset);
		int indexToAlloc = sessionIndexPool.front();
		sessionIndexPool.pop_front();

		ConnectSession(indexToAlloc, (int)clientFD, clientIP);
		
		return;	
	}

	void TcpNetwork::RunSelectedSession(fd_set& readSet, fd_set& writeSet, fd_set& exceptSet)
	{
		for (auto& iter : sessionPool)
		{
			if (FD_ISSET(iter->socketFD, &exceptSet))
			{
				// m_pLogger->
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, iter->index);
			}

			if (FD_ISSET(iter->socketFD, &readSet))
			{
				RecvFromClient(iter->index);
				auto result = RecvBufferToPacketQueue(iter->index);
				if (result != NET_ERROR_CODE::NONE)
				{
					CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, iter->index);
				}
			}

			if (FD_ISSET(iter->socketFD, &writeSet))
			{
				SendFromSession(iter->index);
			}
		}
	}

	void TcpNetwork::SetSockOption(const int clientFD)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(clientFD, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = m_Config.maxClientSockOptRecvBufferSize;
		int size2 = m_Config.maxClientSockOptSendBufferSize;
		setsockopt(clientFD, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(clientFD, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}
	
	void TcpNetwork::ConnectSession(const int index, const int clientFD, const char * clientIP)
	{
		sessionPool[index]->index = index;
		sessionPool[index]->socketFD = clientFD;
		std::copy(&clientIP[0], &clientIP[MAX_IP_LEN], &sessionPool[index]->ip[0]);
		
		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Connect Success index : %d, fd : %d, IP : %s", __FUNCTION__, index, clientFD, clientIP);
	}

	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE closeCase, const int sessionIndex)
	{
		if (sessionPool[sessionIndex]->IsConnected() == false)
			return;

		int clientFD = sessionPool[sessionIndex]->socketFD;

		if (closeCase != SOCKET_CLOSE_CASE::SESSION_POOL_IS_FULL)
		{
			sessionPool[sessionIndex]->Clear();
			sessionIndexPool.push_back(sessionIndex);
		}

		closesocket(clientFD);
		FD_CLR(clientFD, &m_fdset);

		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Session Closed. FD(%d), IP(%s)", __FUNCTION__, clientFD, sessionPool[sessionIndex]->ip);
		return;
	}

	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short packetId, const int dataSize, char * pMsg)
	{
		RecvPacketInfo recvPacket;
		recvPacket.sessionIndex = sessionIndex;
		recvPacket.packetId = packetId;
		recvPacket.dataSize = dataSize;
		recvPacket.pData = pMsg;

		packetQueue.push_back(recvPacket);
	}
	
	void TcpNetwork::RecvFromClient(const int sessionIndex)
	{
		auto& client = sessionPool[sessionIndex];
		int recvPos = client->remainedDataSize;
		auto recvSize = recv(client->socketFD, &(client->pRecvBuffer[recvPos]), MAX_PACKET_SIZE * 2, 0);

		if (recvSize <= 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				return;
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, client->index);
		}

		client->remainedDataSize += recvSize;
		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Recv data from client. FD(%d), IP(%s)", __FUNCTION__, client->socketFD, client->ip);
	}

	NET_ERROR_CODE TcpNetwork::RecvBufferToPacketQueue(const int sessionIndex)
	{
		auto& client = sessionPool[sessionIndex];
		int startPos = 0;
		int dataSize = client->remainedDataSize;
		
		while (dataSize - startPos >= PACKET_HEADER_SIZE)
		{
			PacketHeader* pktHeader = (PacketHeader*)&client->pRecvBuffer[startPos];
			startPos += PACKET_HEADER_SIZE;

			if (pktHeader->bodySize > dataSize - startPos)
			{
				startPos -= PACKET_HEADER_SIZE;
				break;
			}

			if (pktHeader->bodySize > MAX_PACKET_SIZE)
			{
				startPos -= PACKET_HEADER_SIZE;
				return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
			}

			AddPacketQueue(sessionIndex, pktHeader->id, pktHeader->bodySize, &(client->pRecvBuffer[startPos]));
			startPos += pktHeader->bodySize;
		}

		client->remainedDataSize -= startPos;

		if (client->remainedDataSize >= 0)
		{
			std::copy(&client->pRecvBuffer[startPos], &client->pRecvBuffer[startPos + client->remainedDataSize], &client->pRecvBuffer[0]);
		}

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::SendFromSession(const int sessionIndex)
	{
		auto& client = sessionPool[sessionIndex];
		auto sentSize = send(client->socketFD, client->pSendBuffer, client->sendDataSize, 0);

		if (sentSize < 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				return;
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, client->index);
			return;
		}

		if (sentSize == 0)
			return;

		if (sentSize < client->sendDataSize)
		{
			std::copy(&client->pSendBuffer[sentSize], &client->pSendBuffer[client->sendDataSize], &client->pSendBuffer[0]);
			client->sendDataSize -= sentSize;
		}
		else
		{
			client->sendDataSize = 0;
		}
		
		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Send data from session. FD(%d), IP(%s)", __FUNCTION__, client->socketFD, client->ip);
	}

	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		if (packetQueue.empty() == false)
		{
			packetInfo = packetQueue.front();
			packetQueue.pop_front();
		}

		return packetInfo;
	}
}