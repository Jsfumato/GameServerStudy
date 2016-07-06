#include "../../160622/Common/Packet.h"
#include "../../nana_Server_chat/TCPServerLib/TCPServer.h"
#include "../../nana_Server_chat/TCPServerLib/ServerErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

namespace NLogicLib
{
	ERROR_CODE PacketProcess::LobbyEnter(PacketInfo packetInfo)
	{
		ERROR_CODE result = ERROR_CODE::NONE;
		NCommon::PktLobbyEnterRes resPkt;

		// CheckError lambda �Լ�
		auto CheckError = [&](ERROR_CODE error)
		{
			resPkt.SetError(error);
			pRefNetwork->SendData(packetInfo.SessionIndex, static_cast<int>(NCommon::PACKET_ID::LOBBY_ENTER_RES), sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
			return error;
		};

		auto reqPkt = (NCommon::PktLobbyEnterReq*)packetInfo.pRefData;
		auto pUserRet = pRefUserMgr->GetUser(packetInfo.SessionIndex);
		
		result = std::get<0>(pUserRet);

		// error check, �޾ƿ� pUserRet�� ������ �ִ��� Ȯ��
		if (result != ERROR_CODE::NONE)
			return CheckError(result);

		auto pUser = std::get<1>(pUserRet);
		// error check, �޾ƿ� pUser�� ���� ���� ������ Ȯ��
		if (pUser->IsCurDomainInLogIn == false)
			return CheckError(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);

		auto pLobby = pRefLobbyMgr->GetLobby(reqPkt->LobbyId);
		// error check, pLobby�� �ùٸ��� �޾ƿԴ��� Ȯ��
		if (pLobby == nullptr)
			return CheckError(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);

		result = pLobby->EnterUser(pUser);
		// error check, pLobby�� pUser�� �� ������ Ȯ��
		if (result != ERROR_CODE::NONE)
			return CheckError(result);

		pLobby->NotifyLobbyEnterUserInfo(pUser);
		resPkt.MaxUserCount = pLobby->MaxUserCount();
		resPkt.MaxRoomCount = pLobby->MaxRoomCount();
		
		// ERROR_CODE::NONE ����
		return CheckError(result);
	}

	ERROR_CODE PacketProcess::LobbyRoomList(PacketInfo packetInfo)
	{
		ERROR_CODE result = ERROR_CODE::NONE;
		NCommon::PktLobbyEnterRes resPkt;

		// CheckError lambda �Լ�
		auto CheckError = [&](ERROR_CODE error)
		{
			resPkt.SetError(error);
			pRefNetwork->SendData(packetInfo.SessionIndex, static_cast<int>(NCommon::PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES), sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
			return error;
		};

		auto reqPkt = (NCommon::PktLobbyRoomListReq*)packetInfo.pRefData;
		auto pUserRet = pRefUserMgr->GetUser(packetInfo.SessionIndex);
		
		result = std::get<0>(pUserRet);
		// error check, pUserRet�� �ùٸ��� �޾ƿԴ°�
		if (result != ERROR_CODE::NONE)
			return CheckError(result);

		auto pUser = std::get<1>(pUserRet);
		// error check, pUser�� ���� ���ΰ�
		if (pUser->IsCurDomainInLobby == false)
			return CheckError(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);

		auto pLobby = pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		// error check, pLobby�� �ùٸ��� �޾ƿԴ���
		if (pLobby == nullptr)
			return CheckError(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);

		/*##!! Find This !!##*/
		pLobby->SendRoomList(pUser->GetSessioIndex(), reqPkt->StartRoomIndex);
	
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::LobbyUserList(PacketInfo packetInfo)
	{
		ERROR_CODE result = ERROR_CODE::NONE;
		NCommon::PktLobbyUserListRes resPkt;

		auto CheckError = [&](ERROR_CODE error) 
		{
			resPkt.SetError(error);
			pRefNetwork->SendData(packetInfo.SessionIndex, packetInfo.PacketId, sizeof(NCommon::PktLobbyUserListRes), (char*)&resPkt);
			return error;
		};

		auto reqPkt = (NCommon::PktLobbyUserListReq*)packetInfo.pRefData;
		auto pUserRet = pRefUserMgr->GetUser(packetInfo.SessionIndex);

		// pUserRet�� �ùٸ��� �����Դ��� üũ
		result = std::get<0>(pUserRet);
		if (result != ERROR_CODE::NONE)
			CheckError(result);

		// user�� �κ� �ִ��� üũ
		auto pUser = std::get<1>(pUserRet);
		if (pUser->IsCurDomainInLobby == false)
			CheckError(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);

		// user�� �ִ� lobby�� �ùٸ��� �޾ƿ����� ġũ
		auto pLobby = pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex);
		if (pLobby == nullptr)
			CheckError(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);

		pLobby->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex);
		return result;
	}

	ERROR_CODE PacketProcess::LobbyLeave(PacketInfo packetInfo)
	{
		ERROR_CODE result = ERROR_CODE::NONE;
		NCommon::PktLobbyLeaveRes resPkt;

		auto CheckError = [&](ERROR_CODE error)
		{
			resPkt.SetError(error);
			pRefNetwork->SendData(packetInfo.SessionIndex, packetInfo.PacketId, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);
			return error;
		};

		auto pUserRet = pRefUserMgr->GetUser(packetInfo.SessionIndex);
		result = std::get<0>(pUserRet);
		if (result != ERROR_CODE::NONE)
			CheckError(result);

		auto pUser = std::get<1>(pUserRet);
		if (pUser->IsCurDomainInLobby == false)
			CheckError(ERROR_CODE::LOBBY_LEAVE_INVALID_DOMAIN);

		auto pLobby = pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex);
		if (pLobby == nullptr)
			CheckError(ERROR_CODE::LOBBY_LEAVE_INVALID_LOBBY_INDEX);

		pUser->LeaveLobby();
		return ERROR_CODE::NONE;
	}
}