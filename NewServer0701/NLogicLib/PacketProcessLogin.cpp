#include "../Common/Packet.h"
#include "../NServerNetLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::Login(PacketInfo packetInfo)
	{
	CHECK_START
		//TODO: ���� �����Ͱ� PktLogInReq ũ�⸸ŭ���� �����ؾ� �Ѵ�.
		// �н������ ������ pass ���ش�.
		// ID �ߺ��̶�� ���� ó���Ѵ�.

		NCommon::PktLogInRes resPkt;
		auto reqPkt = (NCommon::PktLogInReq*)packetInfo.pData;

		auto addRet = m_pUserManager->AddUser(packetInfo.sessionIndex, reqPkt->szID);

		if (addRet != ERROR_CODE::NONE) {
			CHECK_ERROR(addRet);
		}

		resPkt.ErrorCode = (short)addRet;
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LogOut(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktLogOutRes resPkt;
		auto reqPkt = (NCommon::PktLogOutReq*)packetInfo.pData;
		
		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<ERROR_CODE>(pUserRet);
		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<User*>(pUserRet);
		if (pUser->GetID() != reqPkt->szID) {
			CHECK_ERROR(ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION);
		}

		auto addRet = m_pUserManager->RemoveUser(packetInfo.sessionIndex);	/*AddUser(packetInfo.sessionIndex, reqPkt->szID);*/

		if (addRet != ERROR_CODE::NONE) {
			CHECK_ERROR(addRet);
		}

		resPkt.SetError(addRet);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOGIN_OUT_RES, sizeof(NCommon::PktLogOutRes), (char*)&resPkt);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOGIN_OUT_RES, sizeof(NCommon::PktLogOutRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyList(PacketInfo packetInfo)
	{
	CHECK_START
		// ���� ���� �����ΰ�?
		// ���� �κ� ���� ���� �����ΰ�?
		
		auto pUserRet = m_pUserManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}
	
		auto pUser = std::get<1>(pUserRet);
		
		if (pUser->IsCurDomainInLogIn() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN);
		}
		
		m_pLobbyManager->SendLobbyListInfo(packetInfo.sessionIndex);
		
		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyListRes resPkt;
		resPkt.SetError(__result);
		m_pNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}