#include "stdafx.h"
#include "../TCPServerLib/ILog.h"
#include "../TCPServerLib/TCPServer.h"

#include "PacketProcess.h"
#include "User.h"
#include "UserManager.h"
#include "Room.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

namespace NLogicLib
{
	void NLogicLib::PacketProcess::Init(TcpNet * pNetwork, UserManager * pUserMgr, LobbyManager * pLobbyMgr, ILog * pLogger)
	{
		pRefLogger = pLogger;
		pRefLobbyMgr = pLobbyMgr;
		pRefNetwork = pNetwork;
		pRefUserMgr = pUserMgr;

		for (int i = 0; i < static_cast<int>(NCommon::PACKET_ID::MAX); ++i)
		{
			PacketFuncArray[i] = nullptr;
		}

		PacketFuncArray[(int)ServerNetworkLib::PACKET_ID::NTF_SYS_CLOSE_SESSION] = &PacketProcess::NtfSysCloseSesson;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOGIN_IN_REQ] = &PacketProcess::Login;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_LIST_REQ] = &PacketProcess::LobbyList;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_ENTER_REQ] = &PacketProcess::LobbyEnter;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ] = &PacketProcess::LobbyRoomList;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_ENTER_USER_LIST_REQ] = &PacketProcess::LobbyUserList;
		PacketFuncArray[(int)NCommon::PACKET_ID::LOBBY_LEAVE_REQ] = &PacketProcess::LobbyLeave;
		PacketFuncArray[(int)NCommon::PACKET_ID::ROOM_ENTER_REQ] = &PacketProcess::RoomEnter;
		PacketFuncArray[(int)NCommon::PACKET_ID::ROOM_LEAVE_REQ] = &PacketProcess::RoomLeave;
		PacketFuncArray[(int)NCommon::PACKET_ID::ROOM_CHAT_REQ] = &PacketProcess::RoomChat;
	}

	void PacketProcess::Process(PacketInfo packetInfo)
	{
		auto packetId = packetInfo.PacketId;
		if (PacketFuncArray[packetId] == nullptr)
			return;
		(this->*PacketFuncArray[packetId])(packetInfo);
	}

	ERROR_CODE PacketProcess::NtfSysCloseSesson(PacketInfo packetInfo)
	{
		auto pUser = std::get<1>(pRefUserMgr->GetUser(packetInfo.SessionIndex));
		if (pUser)
		{
			auto pLobby = pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
			if (pLobby)
			{
				auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());

				if (pRoom)
				{
					pRoom->LeaveUser(pUser->GetIndex());
					pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
					pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

					pRefLogger->Write(ServerNetworkLib::LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Room Out", __FUNCTION__, packetInfo.SessionIndex);
				}

				pLobby->LeaveUser(pUser->GetIndex());

				if (pRoom == nullptr) {
					pLobby->NotifyLobbyLeaveUserInfo(pUser);
				}

				pRefLogger->Write(ServerNetworkLib::LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Lobby Out", __FUNCTION__, packetInfo.SessionIndex);
			}

			pRefUserMgr->RemoveUser(packetInfo.SessionIndex);
		}


		pRefLogger->Write(ServerNetworkLib::LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d)", __FUNCTION__, packetInfo.SessionIndex);
		return ERROR_CODE::NONE;
	}
}