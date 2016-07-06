#pragma once

#include "Define.h"
#include "ServerErrorCode.h"
#include "ILog.h"

namespace ServerNetworkLib
{
	class ITCPServer
	{
	public:
		ITCPServer() {}
		virtual ~ITCPServer() {}

		virtual NET_ERROR_CODE Init(const ServerConfig* pConfig, ILog* pLogger) { return NET_ERROR_CODE::NONE; }
		virtual NET_ERROR_CODE SendData(const int sessionIndex, const short packetId,
			const short size, const char* pMsg) {
			return NET_ERROR_CODE::NONE;
		}

		virtual void Run() {}
		virtual RecvPacketInfo GetPacketInfo() { return RecvPacketInfo(); }
	};
}