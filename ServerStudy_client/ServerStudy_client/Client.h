#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <string>

constexpr int bufferSize = 511;

class Client
{
public:
	Client() = default;
	~Client()
	{
		closesocket(sock);
		WSACleanup();
	};

	void SetServerIP();
	void SetServerPort();

	bool ConnectServer();
	void SendMassageToServer();

private:
	WSADATA wsa;
	SOCKET sock;
	SOCKADDR_IN serverAddr;

	void err_quit(char* msg);
	void err_display(char *msg);
	int RecieveMassage(SOCKET s, char *buf, int len, int flags);

	std::string serverIP = "127.0.0.1";
	int	serverPort = 23452;
};

