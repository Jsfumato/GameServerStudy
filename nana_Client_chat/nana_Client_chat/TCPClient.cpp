#include "stdafx.h"
#include "TCPClient.h"


bool TCPClient::IsConnected()
{
	return isConnected;
}

bool TCPClient::ConnectTo(std::string ipAddress, int portNum)
{
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	//socket 함수 호출
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return false;

	//sockaddr 구조체 정의
	//window 환경이기에 sockaddr_in 구조체로 초기화
	SOCKADDR_IN client_addr;
	ZeroMemory(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(portNum);
	inet_pton(AF_INET, ipAddress.c_str(), &client_addr.sin_addr.s_addr);

	//connect 함수 호출
	int result = connect(clientSocket, (sockaddr*)&client_addr, sizeof(client_addr));
	if (result != 0)
		return false;

	isConnected = true;

	return true;
}

void TCPClient::DisConnect()
{
	if (isConnected == true)
	{
		closesocket(clientSocket);
		isConnected = false;
	}
}

void TCPClient::Update()
{
	while (isConnected)
	{
		RecvData();
		RecvDataToQueue();
	}
}

void TCPClient::SendPacket(short headerInfo, std::string data)
{
	char buffer[MAX_BUFFER_SIZE] = { 0, };

	PacketHeader packetHeader;
	ZeroMemory(&packetHeader, sizeof(packetHeader));
	packetHeader.id = headerInfo;
	packetHeader.size = data.size();

	memcpy(&buffer[0], (char*)&packetHeader, sizeof(packetHeader));
	memcpy(&buffer[sizeof(packetHeader)], data.c_str(), data.size());
	
	send(clientSocket, buffer, sizeof(packetHeader) + data.size(), 0);
}

void TCPClient::RecvData()
{
	fd_set read_set;
	timeval interval;
	interval.tv_sec = 0;
	interval.tv_usec = 100;

	FD_ZERO(&read_set);
	FD_SET(clientSocket, &read_set);

	if (select(clientSocket + 1, &read_set, NULL, NULL, &interval) <= 0)
		return;

	if (FD_ISSET(clientSocket, &read_set))
	{
		char tmpbuffer[MAX_BUFFER_SIZE] = { 0, };
		int recvSize = recv(clientSocket, tmpbuffer, MAX_BUFFER_SIZE, 0);

		if (recvSize == 0)
			return;

		if (recvSize > 0)
		{
			memcpy(&buffer[offsetBuffer], &tmpbuffer, recvSize);
			offsetBuffer += recvSize;
		}
	}
}

// 최적화 필요할 듯
void TCPClient::RecvDataToQueue()
{
	auto readPos = 0;
	const auto dataSize = offsetBuffer;
	PacketHeader* pPktHeader;

	while ((dataSize - readPos) > sizeof(PacketHeader))
	{
		pPktHeader = (PacketHeader*)&buffer[readPos];
		readPos += sizeof(PacketHeader);

		if (pPktHeader->size > (dataSize - readPos))
			break;

		if (pPktHeader->size > MAX_BUFFER_SIZE)
			return;// NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;

		AddPacketQueue(pPktHeader->id, pPktHeader->size, &buffer[readPos]);

		readPos += pPktHeader->size;
	}

	offsetBuffer -= readPos;

	if (offsetBuffer > 0)
		memcpy(buffer, &buffer[readPos], offsetBuffer);
}

void TCPClient::AddPacketQueue(const short pktId, const short bodySize, char* pDataPos)
{
	RecvPacketInfo packetInfo;
	packetInfo.PacketId = pktId;
	packetInfo.PacketBodySize = bodySize;
	packetInfo.pData = new char[bodySize];
	memcpy(packetInfo.pData, pDataPos, bodySize);

	//std::lock_guard<std::mutex> guard(m_mutex);
	packetQueue.push_back(packetInfo);
}

RecvPacketInfo TCPClient::GetPacket()
{
	//패킷 큐가 비어있는 경우 빈 패킷 반환
	if (packetQueue.empty())
		return RecvPacketInfo();

	RecvPacketInfo packet = packetQueue.front();
	packetQueue.pop_front();
	
	return packet;
}

