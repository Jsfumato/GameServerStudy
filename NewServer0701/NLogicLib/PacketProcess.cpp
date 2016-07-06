#include "PacketProcess.h"
#include "UserManager.h"
#include "User.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "../NServerNetLib/ILog.h"
#include "../NServerNetLib/TcpNetwork.h"

using LOG_TYPE = NServerNetLib::LOG_TYPE;
//using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	PacketProcess::PacketProcess()
	{
	}

	PacketProcess::~PacketProcess()
	{
	}

	void PacketProcess::Init(NServerNetLib::ITcpNetwork * pNetwork, UserManager * pUserMgr, LobbyManager * pLobbyMgr, NServerNetLib::ILog * pLogger)
	{
		m_pNetwork = pNetwork;
		m_pUserManager = pUserMgr;
		m_pLobbyManager = pLobbyMgr;
		m_pLogger = pLogger;

		for (int i = 0; i < static_cast<int>(NCommon::PACKET_ID::MAX); ++i)
		{
			PacketFuncArray[i] = nullptr;
		}

		PacketFuncArray[(int)NServerNetLib::PACKET_ID::NTF_SYS_CLOSE_SESSION] = &PacketProcess::NtfSysCloseSession;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOGIN_IN_REQ] = &PacketProcess::Login;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOGIN_OUT_REQ] = &PacketProcess::LogOut;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_LIST_REQ] = &PacketProcess::LobbyList;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_ENTER_REQ] = &PacketProcess::LobbyEnter;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ] = &PacketProcess::LobbyRoomList;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_ENTER_USER_LIST_REQ] = &PacketProcess::LobbyUserList;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_LEAVE_REQ] = &PacketProcess::LobbyLeave;
		PacketFuncArray[(int)NCommon::PACKET_ID::ROOM_ENTER_REQ] = &PacketProcess::RoomEnter;
		PacketFuncArray[(int)NCommon::PACKET_ID::ROOM_LEAVE_REQ] = &PacketProcess::RoomLeave;
		PacketFuncArray[(int)NCommon::PACKET_ID::ROOM_CHAT_REQ] = &PacketProcess::RoomChat;

		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_CHAT_REQ] = &PacketProcess::LobbyChat;
	}

	void PacketProcess::Process(PacketInfo packetInfo)
	{
		auto packetId = packetInfo.packetId;

		if (PacketFuncArray[packetId] == nullptr)
		{
			m_pLogger->Write(LOG_TYPE::L_WARN, "%s | Invalid PacketId from session. id : %d", __FUNCTION__, packetInfo.sessionIndex);
			return;
		}

		//std::function을 사용하도록 수정해보자
		(this->*PacketFuncArray[packetId])(packetInfo);
	}

	ERROR_CODE PacketProcess::NtfSysCloseSession(PacketInfo packetInfo)
	{
		auto pUser = std::get<1>(m_pUserManager->GetUser(packetInfo.sessionIndex));

		if (pUser != nullptr)
		{
			auto pLobby = m_pLobbyManager->GetLobby(pUser->GetLobbyIndex());
			if (pLobby != nullptr)
			{
				auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());

				if (pRoom != nullptr)
				{
					pRoom->LeaveUser(pUser->GetIndex());
					pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
					pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

					m_pLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Room Out", __FUNCTION__, packetInfo.sessionIndex);
				}

				pLobby->LeaveUser(pUser->GetIndex());

				if (pRoom == nullptr) {
					pLobby->NotifyLobbyLeaveUserInfo(pUser);
				}
				m_pLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Lobby Out", __FUNCTION__, packetInfo.sessionIndex);
			}
			m_pUserManager->RemoveUser(packetInfo.sessionIndex);
		}

		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d)", __FUNCTION__, packetInfo.sessionIndex);
		return ERROR_CODE::NONE;
	}

}