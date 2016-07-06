#pragma once
#include <memory>

#include "../../160622/Common/Packet.h"
#include "../../160622/Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace ServerNetworkLib
{
	struct ServerConfig;
	class ILog;
	class ITCPServer;
}

namespace NLogicLib
{
	class UserManager;
	class LobbyManager;
	class PacketProcess;

	class Main
	{
	public:
		Main();
		~Main();

		ERROR_CODE Init();

		void Run();

		void Stop();


	private:
		ERROR_CODE LoadConfig();

		void Release();


	private:
		bool m_IsRun = false;

		std::unique_ptr<ServerNetworkLib::ServerConfig> m_pServerConfig;
		std::unique_ptr<ServerNetworkLib::ILog> m_pLogger;

		std::unique_ptr<ServerNetworkLib::ITCPServer> m_pNetwork;
		std::unique_ptr<PacketProcess> m_pPacketProc;
		std::unique_ptr<UserManager> m_pUserMgr;
		std::unique_ptr<LobbyManager> m_pLobbyMgr;
		
	};
}
