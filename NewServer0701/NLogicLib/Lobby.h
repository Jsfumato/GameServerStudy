#pragma once
#include <unordered_map>
#include <vector>
#include "Room.h"
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
	struct LobbyUser
	{
		short Index = 0;
		User* pUser = nullptr;
	};

	class Lobby
	{
	public:
		Lobby() = default;
		virtual ~Lobby() = default;

		void Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount);
		void SetNetwork(NServerNetLib::ITcpNetwork* pNetwork, NServerNetLib::ILog* pLogger);
		
		ERROR_CODE EnterUser(User* pUser);
		ERROR_CODE LeaveUser(const int userIndex);
			
		ERROR_CODE SendRoomList(const int sessionId, const short startRoomId);
		ERROR_CODE SendUserList(const int sessionId, const short startUserIndex);

		void NotifyLobbyEnterUserInfo(User* pUser);
		void NotifyLobbyLeaveUserInfo(User* pUser);

		short GetUserCount();
		short GetIndex() { return m_LobbyIndex; }
		Room* GetRoom(const short roomIndex);
		void NotifyChangedRoomInfo(const short roomIndex);
		Room * CreateRoom();
		void NotifyChat(const int sessionIndex, const char * pszUserID, const wchar_t * pszMsg);
		auto MaxUserCount() { return (short)m_MaxUserCount; }
		auto MaxRoomCount() { return (short)m_RoomList.size(); }

	protected:
		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

		User* FindUser(const int userIndex);
		ERROR_CODE AddUser(User* pUser);
		void RemoveUser(const int userIndex);



		NServerNetLib::ILog* m_pRefLogger;
		NServerNetLib::ITcpNetwork* m_pRefNetwork;

		std::vector<LobbyUser> m_UserList;
		std::unordered_map<int, User*> m_UserIndexDic;
		std::unordered_map<const char*, User*> m_UserIDDic;
		
		std::vector<Room> m_RoomList;

		short m_LobbyIndex = 0;
		short m_MaxUserCount = 0;
	};
}