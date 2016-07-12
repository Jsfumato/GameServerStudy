#include "../Common/Packet.h"
#include "../NServerNetLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "Room.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;
using LOG_TYPE = NServerNetLib::LOG_TYPE;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pData;
		NCommon::PktRoomEnterRes resPkt;
		
		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pLobbyManager->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}
		
		Room* pRoom = nullptr;

		// ���� ����� ����� ���� �����
		if (reqPkt->IsCreate == true)
		{
			pRoom = pLobby->CreateRoom();
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
			}

			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE) {
				CHECK_ERROR(ret);
			}
		}
		else
		{
			pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
			}
		}

		auto enterRet = pRoom->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}
		
		// ���� ������ �뿡 ���Դٰ� �����Ѵ�.
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		// �κ� ������ �������� �˸���
		pLobby->NotifyLobbyLeaveUserInfo(pUser);
		
		// �κ� �� ������ �뺸�Ѵ�.
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		// �뿡 �� ���� ���Դٰ� �˸���
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());
		
		resPkt.SetError(ERROR_CODE::NONE);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktRoomLeaveRes resPkt;

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pLobbyManager->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) {
			CHECK_ERROR(leaveRet);
		}
		
		// ���� ������ �κ�� ����
		pUser->EnterLobby(lobbyIndex);

		// �뿡 ������ �������� �뺸
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		// �κ� ���ο� ������ �������� �뺸
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		// �κ� �ٲ� �� ������ �뺸
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());
		
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pData;
		NCommon::PktRoomChatRes resPkt;

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		
		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pLobbyManager->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);
				
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomUserList(PacketInfo packetInfo)
	{
	CHECK_START
		// ���� �κ� �ִ��� �����Ѵ�.
		// ���� ����Ʈ�� �����ش�.

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInRoom() == false) {
			// ���� ��ġ�� �ʾ���
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
		}

		auto pLobby = m_pLobbyManager->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_LOBBY_INDEX);
		}
		
		auto reqPkt = (NCommon::PktRoomUserListReq*)packetInfo.pData;

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		
		auto sendRet = pRoom->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex);
		if (sendRet != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		m_pLogger->Write(LOG_TYPE::L_TRACE, "%s | Req Room User List : %d", __FUNCTION__, packetInfo.sessionIndex);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyUserListRes resPkt;
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | ERROR_CODE : %d", __FUNCTION__, resPkt.ErrorCode);
		return (ERROR_CODE)__result;
	}

}