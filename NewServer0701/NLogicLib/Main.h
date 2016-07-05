#pragma once
#include <iostream>
#include <memory>

#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NServerNetLib
{
	struct ServerConfig;
	class ILog;
	class ITcpNetwork;
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
		bool isRunning = true;
		ERROR_CODE LoadConfig();

		std::unique_ptr<UserManager> m_UserManager;
		std::unique_ptr<LobbyManager> m_LobbyManager;
		std::unique_ptr<PacketProcess> m_PacketProcess;

		std::unique_ptr<NServerNetLib::ILog> m_pLogger;
		std::unique_ptr<NServerNetLib::ITcpNetwork> m_pNetwork;
		std::unique_ptr<NServerNetLib::ServerConfig> m_Config;
	};
}

// http://stackoverflow.com/questions/13414652/forward-declaration-with-unique-ptr