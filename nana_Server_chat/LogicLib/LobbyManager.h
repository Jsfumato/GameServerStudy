#pragma once
#include <vector>
#include <unordered_map>

namespace ServerNetworkLib
{
	class TcpNetwork;
	class ILog;
}

namespace NLogicLib
{
	struct LobbyManagerConfig
	{
		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	struct LobbySmallInfo
	{
		short Num;
		short UserCount;
	};

	class Lobby;

	class LobbyManager
	{
		using TcpNet = ServerNetworkLib::ITCPServer;
		using ILog = ServerNetworkLib::ILog;

	public:
		LobbyManager();
		virtual ~LobbyManager();

		void Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILog* pLogger);

		Lobby* GetLobby(short lobbyId);


	public:
		void SendLobbyListInfo(const int sessionIndex);

	private:
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;

		std::vector<Lobby> m_LobbyList;
	};
}