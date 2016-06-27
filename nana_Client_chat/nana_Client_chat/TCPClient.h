#pragma once
#include <string>

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>

// MTU는 보통 1500 바이트 (헤더 포함)
constexpr int MAX_BUFFER_SIZE = 1024;
constexpr int MAX_CLIENT_BUFFER_SIZE = 1024 * 10;

// 패킷의 정보를 담을 헤더를 정의
struct PacketHeader
{
	short id;
	short size;
};

struct RecvPacketInfo
{
	RecvPacketInfo() = default;

	short PacketId = 0;
	short PacketBodySize = 0;
	char* pData = nullptr;
};

class TCPClient
{
public:
	TCPClient() = default;
	~TCPClient()
	{
		if (isConnected == true)
			DisConnect();
	};

	bool ConnectTo(std::string ipAddress, int portNum);
	void DisConnect();
	void Update();

	bool IsConnected();
	
	void SendPacket(short headerInfo, std::string data);
	void RecvData();
	void RecvDataToQueue();

	void AddPacketQueue(const short pktId, const short bodySize, char * pDataPos);

	RecvPacketInfo GetPacket();

private:
	SOCKET clientSocket;
	WSADATA wsa;

	bool isConnected = false;
	char buffer[MAX_CLIENT_BUFFER_SIZE] = { 0, };
	int offsetBuffer = 0;
	std::deque<RecvPacketInfo> packetQueue;
};

