#pragma once
#include "../../160622/Common/Packet.h"
#include "../TCPServerLib/Define.h"
#include "../../160622/Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace ServerNetworkLib
{
	class ITCPServer;
	class ILog;
}

namespace NLogicLib
{
	class UserManager;
	class LobbyManager;

	class PacketProcess
	{
		using PacketInfo = ServerNetworkLib::RecvPacketInfo;
		typedef ERROR_CODE(PacketProcess::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];

		using TcpNet = ServerNetworkLib::ITCPServer;
		using ILog = ServerNetworkLib::ILog;

	public:
		PacketProcess() = default;
		~PacketProcess() = default;
		
		void Init(TcpNet* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, ILog* pLogger);

		void Process(PacketInfo packetInfo);

	private:
		ILog* pRefLogger;
		TcpNet* pRefNetwork;
		UserManager* pRefUserMgr;
		LobbyManager* pRefLobbyMgr;

		ERROR_CODE NtfSysCloseSesson(PacketInfo packetInfo);
		ERROR_CODE Login(PacketInfo packetInfo);
		
		ERROR_CODE LobbyList(PacketInfo packetInfo);
		ERROR_CODE LobbyEnter(PacketInfo packetInfo);
		ERROR_CODE LobbyRoomList(PacketInfo packetInfo);
		ERROR_CODE LobbyUserList(PacketInfo packetInfo);
		ERROR_CODE LobbyLeave(PacketInfo packetInfo);
		ERROR_CODE RoomEnter(PacketInfo packetInfo);
		ERROR_CODE RoomLeave(PacketInfo packetInfo);
		ERROR_CODE RoomChat(PacketInfo packetInfo);
	};
}