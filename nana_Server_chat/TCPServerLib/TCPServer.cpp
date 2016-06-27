#include "stdafx.h"
#include "TCPServer.h"

namespace ServerNetworkLib
{
	NET_ERROR_CODE ServerNetworkLib::TCPServer::Init(const ServerConfig * config, ILog * logger)
	{
		memcpy(&serverConfig, &config, sizeof(config));
		serverLogger = logger;

		// socket()
		// 서버 역할을 할 소켓의 fd를 받습니다.
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket == INVALID_SOCKET)
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;

		// 소켓 옵션 설정
		auto n = 1;
		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;

		// sockaddr_in struct init
		// 해당 구조체는 서버의 주소 정보를 저장합니다.
		// bind에 사용되며, Windows 환경에서는 SOCKADDR과 같은 크기의 구조체인 SOCKADDR_IN을 사용합니다.
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(serverConfig.Port);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// bind()
		// 서버 소켓에 서버 주소 구조체 정보를 bind 해줍니다.
		if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;

		// listen()
		// 서버 소켓이 클라이언트로부터 connect를 수신할 수 있도록
		// listen 함수를 실행해줍니다.
		if (listen(serverSocket, serverConfig.BackLogCount) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;

		// TCPServer가 멤버변수로 갖고 있는 fd_set을 초기화해줍니다
		// 
		FD_ZERO(&read_set);
		FD_SET(serverSocket, &read_set);

		// sessionPool 생성
		// client로부터 받아올 세션 정보를 저장할 풀을 생성
		// 처리할 client 수의 최대값 + 여분값을 할당한다
		// 중요한 점인데, 일단 여분까지 accept하여 각자의 방을 할당하되, 실제로 소통하는 방은 그보다 작게 구현하겠다는 것
		// 여분의 방에 들어간 client들은 pool이 꽉 찼으니 돌아가라는 소리를 듣게 된다.
		CreateSessionPool(serverConfig.MaxClientCount + serverConfig.ExtraClientCount);

		return NET_ERROR_CODE::NONE;
	}

	void TCPServer::Run()
	{
		fd_set readfd = read_set;
		fd_set writefd = read_set;
		fd_set execfd = read_set;
		timeval timeout{ 0, 1000 };

		// 멤버로 보유하고 있는 fdset을 체크한다.
		// 오류 발생 여부, client로부터 read/write이 "가능"한지 여부를 알려준다.
		int setCount = select(clientSessionPool.size() + 1, &readfd, &writefd, &execfd, &timeout);
		if (setCount < 0)
			return;

		// readfd에서 serverSocket이 set된 경우, 새로운 연결이 시도되는 것이므로
		// AcceptNewSession을 실행한다.
		if (FD_ISSET(serverSocket, &readfd))
			AcceptNewSession();
		
		// serverSocket외에 set된 것이 있는 경우 client에 대해서 write/read 수행
		if(setCount - 1 > 0)
			RunSelectedSocket(readfd, writefd, execfd);
	}

	NET_ERROR_CODE TCPServer::AcceptNewSession()
	{
		// client에 대해서 accept하여 client의 socket을 리턴합니다.
		SOCKADDR_IN client_addr;
		int addrLength = sizeof(client_addr);

		auto clientSocket = accept(serverSocket, (sockaddr*)&client_addr, &addrLength);
		if (clientSocket == SOCKET_ERROR)
			return NET_ERROR_CODE::ACCEPT_API_ERROR;

		// clientSocket의 정보를 입력할 빈 session의 번호를 알려줍니다.
		int clientIndex = AllocNewSessionIndex();

		// -1인 경우 빈 session이 존재하지 않는다는 뜻.
		// accept는 했으나 socket을 닫아줍니다.
		if (clientIndex < 0)
		{
			//m_pRefLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, client_sockfd);

			// 더 이상 수용할 수 없으므로 바로 짜른다.
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, clientSocket, clientIndex);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}
		
		char clientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, (SOCKADDR*)&client_addr, clientIP, MAX_IP_LEN);

		FD_SET(clientSocket, &read_set);
		
		// 해당 함수에서 클라이언트의 fd와 ip를 session이라는 방에 저장합니다
		ConnectSession(clientIndex, clientSocket, clientIP);

		return NET_ERROR_CODE::NONE;
	}
	
	void TCPServer::RunSelectedSocket(fd_set readfd, fd_set writefd, fd_set execfd)
	{
		// 각각의 session에 할당된 fd에 대해서 순회
		for (auto& iter : clientSessionPool)
		{
			// 먼저 에러 발생 여부를 체크하여, 에러가 발생한 경우 에러 케이스와 함께
			// 해당 fd와 session의 index를 전달하여 해당 session을 정리함
			if (FD_ISSET(iter.SocketFD, &execfd))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, iter.SocketFD, iter.Index);
				continue;
			}

			// read가 가능한 client가 존재하는지 check
			if (FD_ISSET(iter.SocketFD, &readfd))
			{
				auto& session = clientSessionPool[iter.Index];
				SOCKET clientSocket = session.SocketFD;

				if (session.IsConnected == false)
					continue;

				// 새로 받을 위치는 session에 아직 패킷이 되지 못하고 남아있는 데이터 바로 뒤입니다.
				// 따라서 받을 위치는 session의 remainDatasize가 됩니다.
				int recvPos = session.RemainingDataSize;
				int recvSize = recv(iter.SocketFD, &iter.pRecvBuffer[recvPos], MAX_PACKET_SIZE, 0);

				// recvSize에는 받아온 데이터의 사이즈가 저장됩니다.
				// 해당 사이즈가 음수라는 뜻은, 에러가 발생했음을 의미합니다.
				if (recvSize < 0)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, iter.SocketFD, iter.Index);
					}
					continue;
				}

				// 새로 데이터를 받아왔으니, 남은 데이터 사이즈에 recvSize를 추가
				// 남은 데이터 사이즈를 갱신
				session.RemainingDataSize += recvSize;

				/*
					FindThis
				*/
				// 각 session의 recvBuffer에 저장된 내용을 packet으로 변경한다.
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
							// 에러 발생, 리턴해줍니다.
							return;
						}
					}
					AddPacketQueue(iter.Index, pktHeader->Id, pktHeader->BodySize, &session.pRecvBuffer[readPos]);

					readPos += pktHeader->BodySize;
				}


				continue;
			}

			// write 가능한 client에게는 send함수 호출
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

				// 보내야 할 내용보다 적게 보낸 경우 
				// 뒤에 남은 데이터를 sendbuffer의 앞으로 보낸다.
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

		//log 출력
	}
	
	void TCPServer::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
	{
		// 먼저 그 사이에 연결이 끊어지지는 않았는지 확인합니다
		if (clientSessionPool[sessionIndex].IsConnected == false)
			return;

		// sessionIndex가 존재하는 index라면, 해당 session을 정리합니다
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