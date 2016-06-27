#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

constexpr int SERVERPORT = 23452;
constexpr int BUFFERSIZE = 511;

class Server
{
public:
	Server() = default;
	~Server() 
	{
		closesocket(listen_sock);
		WSACleanup();
	};

	bool Initialize();
	void DoServer();

	// 소켓 함수 오류 출력
	void err_display(char *msg);

private:
	WSADATA wsa;
	SOCKET listen_sock;
};

