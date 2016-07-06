#include "stdafx.h"
#include "../../160622/Common/Packet.h"
#include "../../160622/Common/PacketID.h"
#include "../../nana_Server_chat/TCPServerLib/TCPServer.h"
#include "../../nana_Server_chat/TCPServerLib/ServerErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

namespace NLogicLib
{
	ERROR_CODE PacketProcess::Login(PacketInfo packetInfo)
	{
		ERROR_CODE result = ERROR_CODE::NONE;
		NCommon::PktLogInRes resPkt;

		auto reqPkt = (NCommon::PktLogInReq*)packetInfo.pRefData;

		result = pRefUserMgr->AddUser(packetInfo.SessionIndex, reqPkt->szID);

		resPkt.SetError(result);
		pRefNetwork->SendData(packetInfo.SessionIndex, (short)NCommon::PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return result;
	}

	ERROR_CODE PacketProcess::LobbyList(PacketInfo packetInfo)
	{
		ERROR_CODE result = ERROR_CODE::NONE;
		NCommon::PktLogInRes resPkt;

		auto pUserRet = pRefUserMgr->GetUser(packetInfo.SessionIndex);
		result = std::get<0>(pUserRet);

		if (result != ERROR_CODE::NONE)
		{
			resPkt.SetError(result);
			pRefNetwork->SendData(packetInfo.SessionIndex, (short)NCommon::PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLobbyListRes), (char*)&resPkt);
			return result;
		}

		auto pUser = std::get<1>(pUserRet);
		if (pUser->IsCurDomainInLogIn() == false)
		{
			resPkt.SetError(result);
			pRefNetwork->SendData(packetInfo.SessionIndex, (short)NCommon::PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLobbyListRes), (char*)&resPkt);
			return result;
		}

		pRefLobbyMgr->SendLobbyListInfo(packetInfo.SessionIndex);
		return ERROR_CODE::NONE;
	}
}