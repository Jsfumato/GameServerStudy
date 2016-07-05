#pragma once
#include "Lobby.h"

namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}

namespace NLogicLib
{
	struct LobbyManagerConfig
	{
		int maxLobbyCount;
		int maxLobbyUserCount;
		int maxRoomCountByLobby;
		int maxRoomUserCount;
	};

	class LobbyManager
	{
	public:
		LobbyManager() = default;
		~LobbyManager() = default;

		void Init(const LobbyManagerConfig config, NServerNetLib::ITcpNetwork* pNetwork, NServerNetLib::ILog* pLogger );

		Lobby* GetLobby(const short lobbyId);
		void SendLobbyListInfo(const int sessionIndex);

	private:
		NServerNetLib::ILog* m_pRefLogger;
		NServerNetLib::ITcpNetwork* m_pRefNetwork;

		std::vector<Lobby> m_LobbyList;
	};
}