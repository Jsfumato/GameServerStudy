#pragma once
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "../NServerNetLib/Define.h"

using ERROR_CODE = NCommon::ERROR_CODE;

#define CHECK_START  ERROR_CODE __result=ERROR_CODE::NONE;
#define CHECK_ERROR(f) __result=f; goto CHECK_ERR;

namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}

namespace NLogicLib
{
	class UserManager;
	class LobbyManager;

	class PacketProcess
	{
	public:
		PacketProcess();
		~PacketProcess();

		using ILog = NServerNetLib::ILog;
		using ITcpNetwork = NServerNetLib::ITcpNetwork;
		using PacketInfo = NServerNetLib::RecvPacketInfo;

		void Init(ITcpNetwork* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, ILog* pLogger);

		void Process(PacketInfo packetInfo);

	private:
		ILog* m_pLogger;
		ITcpNetwork* m_pNetwork;
		UserManager* m_pUserManager;
		LobbyManager* m_pLobbyManager;

		typedef ERROR_CODE(PacketProcess::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];

		ERROR_CODE NtfSysCloseSession(PacketInfo packetInfo);
		ERROR_CODE Login(PacketInfo packetInfo);
		ERROR_CODE LogOut(PacketInfo packetInfo);
		ERROR_CODE LobbyList(PacketInfo packetInfo);
		ERROR_CODE LobbyEnter(PacketInfo packetInfo);
		ERROR_CODE LobbyRoomList(PacketInfo packetInfo);
		ERROR_CODE LobbyUserList(PacketInfo packetInfo);
		ERROR_CODE LobbyLeave(PacketInfo packetInfo);
		ERROR_CODE RoomEnter(PacketInfo packetInfo);
		ERROR_CODE RoomLeave(PacketInfo packetInfo);
		ERROR_CODE RoomChat(PacketInfo packetInfo);
		
		ERROR_CODE LobbyChat(PacketInfo packetInfo);
		ERROR_CODE WhisperChat(PacketInfo packetInfo);
	};
}