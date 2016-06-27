#include "stdafx.h"
#include "TCPServer.h"

namespace ServerNetworkLib
{
	NET_ERROR_CODE ServerNetworkLib::TCPServer::Init(const ServerConfig * config, ILog * logger)
	{
		memcpy(&serverConfig, &config, sizeof(config));
		serverLogger = logger;

		// socket()
		// ���� ������ �� ������ fd�� �޽��ϴ�.
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket == INVALID_SOCKET)
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;

		// ���� �ɼ� ����
		auto n = 1;
		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;

		// sockaddr_in struct init
		// �ش� ����ü�� ������ �ּ� ������ �����մϴ�.
		// bind�� ���Ǹ�, Windows ȯ�濡���� SOCKADDR�� ���� ũ���� ����ü�� SOCKADDR_IN�� ����մϴ�.
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(serverConfig.Port);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// bind()
		// ���� ���Ͽ� ���� �ּ� ����ü ������ bind ���ݴϴ�.
		if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;

		// listen()
		// ���� ������ Ŭ���̾�Ʈ�κ��� connect�� ������ �� �ֵ���
		// listen �Լ��� �������ݴϴ�.
		if (listen(serverSocket, serverConfig.BackLogCount) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;

		// TCPServer�� ��������� ���� �ִ� fd_set�� �ʱ�ȭ���ݴϴ�
		// 
		FD_ZERO(&read_set);
		FD_SET(serverSocket, &read_set);

		// sessionPool ����
		// client�κ��� �޾ƿ� ���� ������ ������ Ǯ�� ����
		// ó���� client ���� �ִ밪 + ���а��� �Ҵ��Ѵ�
		// �߿��� ���ε�, �ϴ� ���б��� accept�Ͽ� ������ ���� �Ҵ��ϵ�, ������ �����ϴ� ���� �׺��� �۰� �����ϰڴٴ� ��
		// ������ �濡 �� client���� pool�� �� á���� ���ư���� �Ҹ��� ��� �ȴ�.
		CreateSessionPool(serverConfig.MaxClientCount + serverConfig.ExtraClientCount);

		return NET_ERROR_CODE::NONE;
	}

	void TCPServer::Run()
	{
		fd_set readfd = read_set;
		fd_set writefd = read_set;
		fd_set execfd = read_set;
		timeval timeout{ 0, 1000 };

		// ����� �����ϰ� �ִ� fdset�� üũ�Ѵ�.
		// ���� �߻� ����, client�κ��� read/write�� "����"���� ���θ� �˷��ش�.
		int setCount = select(clientSessionPool.size() + 1, &readfd, &writefd, &execfd, &timeout);
		if (setCount < 0)
			return;

		// readfd���� serverSocket�� set�� ���, ���ο� ������ �õ��Ǵ� ���̹Ƿ�
		// AcceptNewSession�� �����Ѵ�.
		if (FD_ISSET(serverSocket, &readfd))
			AcceptNewSession();
		
		// serverSocket�ܿ� set�� ���� �ִ� ��� client�� ���ؼ� write/read ����
		if(setCount - 1 > 0)
			RunSelectedSocket(readfd, writefd, execfd);
	}

	NET_ERROR_CODE TCPServer::AcceptNewSession()
	{
		// client�� ���ؼ� accept�Ͽ� client�� socket�� �����մϴ�.
		SOCKADDR_IN client_addr;
		int addrLength = sizeof(client_addr);

		auto clientSocket = accept(serverSocket, (sockaddr*)&client_addr, &addrLength);
		if (clientSocket == SOCKET_ERROR)
			return NET_ERROR_CODE::ACCEPT_API_ERROR;

		// clientSocket�� ������ �Է��� �� session�� ��ȣ�� �˷��ݴϴ�.
		int clientIndex = AllocNewSessionIndex();

		// -1�� ��� �� session�� �������� �ʴ´ٴ� ��.
		// accept�� ������ socket�� �ݾ��ݴϴ�.
		if (clientIndex < 0)
		{
			//m_pRefLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, client_sockfd);

			// �� �̻� ������ �� �����Ƿ� �ٷ� ¥����.
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, clientSocket, clientIndex);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}
		
		char clientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, (SOCKADDR*)&client_addr, clientIP, MAX_IP_LEN);

		FD_SET(clientSocket, &read_set);
		
		// �ش� �Լ����� Ŭ���̾�Ʈ�� fd�� ip�� session�̶�� �濡 �����մϴ�
		ConnectSession(clientIndex, clientSocket, clientIP);

		return NET_ERROR_CODE::NONE;
	}
	
	void TCPServer::RunSelectedSocket(fd_set readfd, fd_set writefd, fd_set execfd)
	{
		// ������ session�� �Ҵ�� fd�� ���ؼ� ��ȸ
		for (auto& iter : clientSessionPool)
		{
			// ���� ���� �߻� ���θ� üũ�Ͽ�, ������ �߻��� ��� ���� ���̽��� �Բ�
			// �ش� fd�� session�� index�� �����Ͽ� �ش� session�� ������
			if (FD_ISSET(iter.SocketFD, &execfd))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, iter.SocketFD, iter.Index);
				continue;
			}

			// read�� ������ client�� �����ϴ��� check
			if (FD_ISSET(iter.SocketFD, &readfd))
			{
				auto& session = clientSessionPool[iter.Index];
				SOCKET clientSocket = session.SocketFD;

				if (session.IsConnected == false)
					continue;

				// ���� ���� ��ġ�� session�� ���� ��Ŷ�� ���� ���ϰ� �����ִ� ������ �ٷ� ���Դϴ�.
				// ���� ���� ��ġ�� session�� remainDatasize�� �˴ϴ�.
				int recvPos = session.RemainingDataSize;
				int recvSize = recv(iter.SocketFD, &iter.pRecvBuffer[recvPos], MAX_PACKET_SIZE, 0);

				// recvSize���� �޾ƿ� �������� ����� ����˴ϴ�.
				// �ش� ����� ������� ����, ������ �߻������� �ǹ��մϴ�.
				if (recvSize < 0)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, iter.SocketFD, iter.Index);
					}
					continue;
				}

				// ���� �����͸� �޾ƿ�����, ���� ������ ����� recvSize�� �߰�
				// ���� ������ ����� ����
				session.RemainingDataSize += recvSize;

				/*
					FindThis
				*/
				// �� session�� recvBuffer�� ����� ������ packet���� �����Ѵ�.
				int readPos = 0;
				const int dataSize = session.RemainingDataSize;
				PacketHeader* pktHeader;

				while ((dataSize - readPos) > PACKET_HEADER_SIZE)
				{
					pktHeader = (PacketHeader*)&session.pRecvBuffer[readPos];
					readPos += PACKET_HEADER_SIZE;

					if (pktHeader->BodySize > 0)
					{
						if (pktHeader->BodySize > (dataSize - readPos))
						{
							break;
						}
						if (pktHeader->BodySize > MAX_PACKET_SIZE)
						{
							// ���� �߻�, �������ݴϴ�.
							return;
						}
					}
					AddPacketQueue(iter.Index, pktHeader->Id, pktHeader->BodySize, &session.pRecvBuffer[readPos]);

					readPos += pktHeader->BodySize;
				}


				continue;
			}

			// write ������ client���Դ� send�Լ� ȣ��
			if (FD_ISSET(iter.SocketFD, &writefd))
			{
				if (iter.IsConnected == false)
					CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, iter.SocketFD, iter.Index);

				if (iter.SendSize <= 0)
					continue;

				int sentSize = send(iter.SocketFD, iter.pSendBuffer, iter.SendSize, 0);
				if (sentSize <= 0)
				{
					CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, iter.SocketFD, iter.Index);
				}

				// ������ �� ���뺸�� ���� ���� ��� 
				// �ڿ� ���� �����͸� sendbuffer�� ������ ������.
				if (sentSize < iter.SendSize)
				{
					memmove(&iter.pSendBuffer[0], &iter.pSendBuffer[sentSize], iter.SendSize - sentSize);
				}
				iter.SendSize -= sentSize;
			}
		}
	}

	void TCPServer::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; ++i)
		{
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.Index = i;
			session.pRecvBuffer = new char[serverConfig.MaxClientRecvBufferSize];
			session.pSendBuffer = new char[serverConfig.MaxClientSendBufferSize];
			
			clientSessionPool.push_back(session);
			clientSessionPoolIndex.push_back(session.Index);
		}
	}

	int TCPServer::AllocNewSessionIndex()
	{
		if (clientSessionPoolIndex.empty())
			return -1;

		int index = clientSessionPoolIndex.front();
		clientSessionPoolIndex.pop_front();
		return index;
	}
	
	void TCPServer::ConnectSession(int sessionIndex, int clientSocket, const char* clientIP)
	{
		auto& session = clientSessionPool[sessionIndex];
		session.SocketFD = clientSocket;
		memcpy(session.IP, clientIP, sizeof(clientIP));

		//log ���
	}
	
	void TCPServer::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
	{
		// ���� �� ���̿� ������ ���������� �ʾҴ��� Ȯ���մϴ�
		if (clientSessionPool[sessionIndex].IsConnected == false)
			return;

		// sessionIndex�� �����ϴ� index���, �ش� session�� �����մϴ�
		if (sessionIndex > 0)
		{
			clientSessionPool[sessionIndex].Clear();
			clientSessionPoolIndex.push_back(sessionIndex);

			AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
		}

		closesocket(sockFD);
		FD_CLR(sockFD, &read_set);
	}
	
	void TCPServer::AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char * pDataPos)
	{
		RecvPacketInfo recvPacket;
		recvPacket.SessionIndex = sessionIndex;
		recvPacket.PacketId = pktId;
		recvPacket.PacketBodySize = bodySize;
		recvPacket.pRefData = pDataPos;

		packetQueue.push_back(recvPacket);
	}
}