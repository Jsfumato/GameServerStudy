#pragma once

namespace NServerNetLib
{
	struct ServerConfig
	{
		unsigned short port;
		int backLogCount;

		int maxClientCount;
		int extraClientCount;

		int maxClientRecvBufferSize;
		int maxClientSendBufferSize;
		int maxClientSockOptRecvBufferSize;
		int maxClientSockOptSendBufferSize;

		int maxLobbyCount;
		int maxLobbyUserCount;
		int maxRoomCountByLobby;
		int maxRoomUserCount;
	};

	enum class SOCKET_CLOSE_CASE : short
	{
		SESSION_POOL_IS_FULL = 1,
		SELECT_ERROR = 2,
		SOCKET_RECV_ERROR = 3,
		SOCKET_RECV_BUFFER_PROCESS_ERROR = 4,
		SOCKET_SEND_ERROR = 5,
	};

	enum class PACKET_ID : short
	{
		NTF_SYS_CLOSE_SESSION = 3
	};

	constexpr int MAX_IP_LEN = 32;
	constexpr int MAX_PACKET_SIZE = 1024; // 최대 패킷 크기
	constexpr int MAX_BUFFER_LENGTH = MAX_PACKET_SIZE * 8;
	

	struct ClientSession
	{
		bool IsConnected()
		{
			if (socketFD > 0)
				return true;
			return false;
		};

		void Clear()
		{
			index = 0;
			socketFD = 0;
			
			remainedDataSize = 0;
			sendDataSize = 0;

			for (int i = 0; i < MAX_IP_LEN; ++i)
				ip[i] = 0;
		};

		int index;
		int socketFD;
		char ip[MAX_IP_LEN] = { 0, };
		
		char* pRecvBuffer = nullptr;
		char* pSendBuffer = nullptr;

		int remainedDataSize = 0;
		int sendDataSize = 0;
	};

	struct RecvPacketInfo
	{
		int sessionIndex = 0;
		short packetId = 0;
		short dataSize = 0;
		char* pData = 0;
	};
	
#pragma pack(push, 1)
	constexpr int PACKET_HEADER_SIZE = 4;

	struct PacketHeader
	{
		short id;
		short bodySize;
	};
#pragma pack(pop)
}