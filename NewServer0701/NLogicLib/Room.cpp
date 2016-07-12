#include <algorithm>

#include "../NServerNetLib/ILog.h"
#include "../NServerNetLib/TcpNetwork.h"
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

#include "User.h"
#include "Room.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	void NLogicLib::Room::Init(const short indexInLobby, const int maxRoomUserCount)
	{
		m_Index = indexInLobby;
		m_MaxRoomUserCount = maxRoomUserCount;

		//for (int i = 0; i < maxRoomUserCount; ++i)
		//{
		//	User user;
		//	user.Init(i);

		//	m_UserPool.push_back(user);
		//	m_UserPoolIndex.push_back(i);
		//}
	}
	void Room::SetNetwork(NServerNetLib::ITcpNetwork* pNetwork, NServerNetLib::ILog* pLogger)
	{
		m_pRefLogger = pLogger;
		m_pRefNetwork = pNetwork;
	}

	void Room::Clear()
	{
		m_IsUsed = false;
		m_Title = L"";
		m_UserList.clear();
	}

	ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
	{
		if (m_IsUsed) {
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
		}

		m_IsUsed = true;
		m_Title = pRoomTitle;

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::EnterUser(User* pUser)
	{
		if (m_IsUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		if (m_UserList.size() == m_MaxRoomUserCount) {
			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
		}

		m_UserList.push_back(pUser);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::LeaveUser(const short userIndex)
	{
		if (m_IsUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		auto iter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
		if (iter == std::end(m_UserList)) {
			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
		}

		m_UserList.erase(iter);

		if (m_UserList.empty())
		{
			Clear();
		}

		return ERROR_CODE::NONE;
	}

	void Room::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
	{
		for (auto pUser : m_UserList)
		{
			//if (pUser->GetIndex() == passUserindex) {
			if (pUser->GetSessioIndex() == passUserindex) {
				continue;
			}

			m_pRefNetwork->SendData(pUser->GetSessioIndex(), packetId, dataSize, pData);
		}
	}

	void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
	{
		NCommon::PktRoomEnterUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
	}

	void Room::NotifyLeaveUserInfo(const char* pszUserID)
	{
		if (m_IsUsed == false) {
			return;
		}

		NCommon::PktRoomLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
	}

	void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
	{
		NCommon::PktRoomChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
	}



	ERROR_CODE Room::SendUserList(const int sessionId, const short startUserIndex)
	{
		if (startUserIndex < 0 || startUserIndex > (m_UserList.size() - 1)) {
			return ERROR_CODE::LOBBY_USER_LIST_INVALID_START_USER_INDEX;
		}

		int lastCheckedIndex = 0;
		NCommon::PktRoomUserListRes pktRes;
		short userCount = 0;

		for (int i = startUserIndex; i < m_UserList.size(); ++i)
		{
			auto& roomUser = m_UserList[i];
			lastCheckedIndex = i;

			pktRes.UserInfo[userCount].LobbyUserIndex = (short)i;
			strncpy_s(pktRes.UserInfo[userCount].UserID, NCommon::MAX_USER_ID_SIZE + 1, roomUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

			++userCount;

			if (userCount >= NCommon::MAX_SEND_LOBBY_USER_LIST_COUNT) {
				break;
			}
		}

		pktRes.Count = userCount;

		if (userCount <= 0 || (lastCheckedIndex + 1) == m_UserList.size()) {
			pktRes.IsEnd = true;
		}

		m_pRefNetwork->SendData(sessionId, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}
}