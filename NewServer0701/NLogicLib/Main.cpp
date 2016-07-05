#include "Main.h"
#include <thread>
#include <chrono>

//#include "../Common/ErrorCode.h"
#include "../NServerNetLib/ServerNetErrorCode.h"
#include "../NServerNetLib/Define.h"
#include "../NServerNetLib/TcpNetwork.h"

#include "ConsoleLogger.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

using LOG_TYPE = NServerNetLib::LOG_TYPE;
using NET_ERROR_CODE = NServerNetLib::NET_ERROR_CODE;

namespace NLogicLib
{
	Main::Main()
	{
		std::cout << "Server ver0.1.1" << std::endl;
		std::cout << "CopyRight by jsfumato" << std::endl;
	}

	Main::~Main()
	{

	}

	ERROR_CODE Main::Init()
	{
		m_pLogger = std::make_unique<ConsoleLog>();
		
		LoadConfig();

		m_pNetwork = std::make_unique<NServerNetLib::TcpNetwork>();
		auto result = m_pNetwork->Init(m_Config.get(), m_pLogger.get());

		if (result != NServerNetLib::NET_ERROR_CODE::NONE)
			return NCommon::ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;

		m_UserManager = std::make_unique<UserManager>();
		m_UserManager->Init(m_Config->maxClientCount);

		m_LobbyManager = std::make_unique<LobbyManager>();
		m_LobbyManager->Init({ 
			m_Config->maxLobbyCount, 
			m_Config->maxLobbyUserCount, 
			m_Config->maxRoomCountByLobby, 
			m_Config->maxRoomUserCount }, 
			m_pNetwork.get(), m_pLogger.get()
			);
		
		m_PacketProcess = std::make_unique<PacketProcess>();
		m_PacketProcess->Init(m_pNetwork.get(), m_UserManager.get(), m_LobbyManager.get(), m_pLogger.get());

		isRunning = true;

		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);
		return ERROR_CODE::NONE;
	}

	void Main::Run()
	{
		while (isRunning == true)
		{
			m_pNetwork->Run();

			while (true)
			{
				auto& packetInfo = m_pNetwork->GetPacketInfo();
				if (static_cast<NCommon::PACKET_ID>(packetInfo.packetId) == NCommon::PACKET_ID::NONE)
				{
					break;
				}
				// Process로 넘긴다
				m_PacketProcess->Process(packetInfo);
			}
			// 이 코드는 무슨 일을 하는 코드?
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}
		return;
	}

	void Main::Stop()
	{
		isRunning = false;
		return;
	}

	ERROR_CODE Main::LoadConfig()
	{
		m_Config = std::make_unique<NServerNetLib::ServerConfig>();

		wchar_t sPath[MAX_PATH] = { 0, };
		::GetCurrentDirectory(MAX_PATH, sPath);

		wchar_t inipath[MAX_PATH] = { 0, };
		_snwprintf_s(inipath, _countof(inipath), _TRUNCATE, L"%s\\ServerConfig.ini", sPath);

		m_Config->port = (unsigned short)GetPrivateProfileInt(L"Config", L"Port", 0, inipath);
		m_Config->backLogCount = GetPrivateProfileInt(L"Config", L"BackLogCount", 0, inipath);
		m_Config->maxClientCount = GetPrivateProfileInt(L"Config", L"MaxClientCount", 0, inipath);

		m_Config->maxClientSockOptRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptRecvBufferSize", 0, inipath);
		m_Config->maxClientSockOptSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptSendBufferSize", 0, inipath);
		m_Config->maxClientRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientRecvBufferSize", 0, inipath);
		m_Config->maxClientSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSendBufferSize", 0, inipath);

		m_Config->extraClientCount = GetPrivateProfileInt(L"Config", L"ExtraClientCount", 0, inipath);
		m_Config->maxLobbyCount = GetPrivateProfileInt(L"Config", L"MaxLobbyCount", 0, inipath);
		m_Config->maxLobbyUserCount = GetPrivateProfileInt(L"Config", L"MaxLobbyUserCount", 0, inipath);
		m_Config->maxRoomCountByLobby = GetPrivateProfileInt(L"Config", L"MaxRoomCountByLobby", 0, inipath);
		m_Config->maxRoomUserCount = GetPrivateProfileInt(L"Config", L"MaxRoomUserCount", 0, inipath);

		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Port(%d), Backlog(%d)", __FUNCTION__, m_Config->port, m_Config->backLogCount);
		return ERROR_CODE::NONE;
	}
}