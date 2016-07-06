#include "../Common/Packet.h"
#include "../NServerNetLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcess.h"
#include "../NServerNetLib/ILog.h"

using PACKET_ID = NCommon::PACKET_ID;
using LOG_TYPE = NServerNetLib::LOG_TYPE;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::LobbyEnter(PacketInfo packetInfo)
	{
	CHECK_START
		// 현재 위치 상태는 로그인이 맞나?
		// 로비에 들어간다.
		// 기존 로비에 있는 사람에게 새 사람이 들어왔다고 알려준다

		auto reqPkt = (NCommon::PktLobbyEnterReq*)packetInfo.pData;
		NCommon::PktLobbyEnterRes resPkt;

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLogIn() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);
		}

		auto pLobby = m_pLobbyManager->GetLobby(reqPkt->LobbyId);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);
		}

		auto enterRet = pLobby->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}
		
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		resPkt.MaxUserCount = pLobby->MaxUserCount();
		resPkt.MaxRoomCount = pLobby->MaxRoomCount();
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
		m_pLogger->Write(LOG_TYPE::L_TRACE, "%s | Enter Lobby : %d", __FUNCTION__, packetInfo.sessionIndex);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
		m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | ERROR_CODE : %d", __FUNCTION__, resPkt.ErrorCode);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyRoomList(PacketInfo packetInfo)
	{
	CHECK_START
		// 현재 로비에 있는지 조사한다.
		// 룸 리스트를 보내준다.

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if ((pUser->IsCurDomainInLobby() == false) && (pUser->IsCurDomainInRoom() == false)) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);
		}

		auto pLobby = m_pLobbyManager->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyRoomListReq*)packetInfo.pData;

		auto sendRet = pLobby->SendRoomList(pUser->GetSessioIndex(), reqPkt->StartRoomIndex);
		if (sendRet != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		m_pLogger->Write(LOG_TYPE::L_TRACE, "%s | Req Lobby Room List : %d", __FUNCTION__, packetInfo.sessionIndex);
		return ERROR_CODE::NONE;

	CHECK_ERR :
		NCommon::PktLobbyRoomListRes resPkt;
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | ERROR_CODE : %d", __FUNCTION__, resPkt.ErrorCode);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyUserList(PacketInfo packetInfo)
	{
	CHECK_START
		// 현재 로비에 있는지 조사한다.
		// 유저 리스트를 보내준다.

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
		}

		auto pLobby = m_pLobbyManager->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyUserListReq*)packetInfo.pData;

		auto sendRet = pLobby->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex);
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
	

	ERROR_CODE PacketProcess::LobbyLeave(PacketInfo packetInfo)
	{
	CHECK_START
		// 현재 로비에 있는지 조사한다.
		// 로비에서 나간다
		// 기존 로비에 있는 사람에게 나가는 사람이 있다고 알려준다.
		NCommon::PktLobbyLeaveRes resPkt;

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_DOMAIN);
		}

		auto pLobby = m_pLobbyManager->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_LOBBY_INDEX);
		}

		auto enterRet = pLobby->LeaveUser(pUser->GetIndex());
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}

		pLobby->NotifyLobbyLeaveUserInfo(pUser);
				
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);

		return ERROR_CODE::NONE;
	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	// roomChat을 응용하여 lobbyChat을 구현
	ERROR_CODE PacketProcess::LobbyChat(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktLobbyChatReq*)packetInfo.pData;
		NCommon::PktLobbyChatRes resPkt;

		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_CHAT_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pLobbyManager->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_CHAT_INVALID_LOBBY_INDEX);
		}

		pLobby->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);

		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}