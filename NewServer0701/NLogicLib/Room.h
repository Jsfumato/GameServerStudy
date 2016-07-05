#pragma once
#include <vector>
#include <deque>
#include "User.h"
#include "../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NServerNetLib
{
	class ILog;
	class ITcpNetwork;
}

namespace NLogicLib
{
	class Room
	{
	public:
		Room() = default;
		~Room() = default;

		void Init(const short indexInLobby, const int maxRoomUserCount);
		void SetNetwork(NServerNetLib::ITcpNetwork* pNetwork, NServerNetLib::ILog* pLogger);
		void Clear();

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);
		ERROR_CODE EnterUser(User* pUser);
		ERROR_CODE LeaveUser(const short userIndex);
		
		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);
		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);

		void NotifyEnterUserInfo(const int userIndex, const char* pszUserID);
		void NotifyLeaveUserInfo(const char* pszUserID);

// Get / Set / Is function

		short GetIndex() { return m_Index; }
		bool IsUsed() { return m_IsUsed; }
		const wchar_t* GetTitle() { return m_Title.c_str(); }
		short MaxUserCount() { return m_MaxRoomUserCount; }
		short GetUserCount() { return (short)m_UserList.size(); }

	private:
		NServerNetLib::ILog* m_pRefLogger;
		NServerNetLib::ITcpNetwork* m_pRefNetwork;

		short m_Index = -1;
		int m_MaxRoomUserCount = 0;
		bool m_IsUsed = false;

		std::wstring m_Title;
		std::vector<User*> m_UserList;
	};
}