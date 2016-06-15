#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
//#include <thread>
#include <process.h>
#include <iostream>
#include <mutex>

constexpr int SERVERPORT = 23451;
constexpr int BUFFERSIZE = 255;

std::mutex coutMutex;

void err_display(char *msg);
void msg(const char* message);
DWORD WINAPI ProcessClient(LPVOID arg);

int main()
{
	//winsock�� �ݵ�� WSAStartup�� �ؾ��Ѵ�.
	//WSADATA
	WSADATA wsa;
	int result;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		err_display("socket() error");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	result = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (result == SOCKET_ERROR)
		err_display("bind() error");

	// listen()
	result = listen(listen_sock, SOMAXCONN);
	if (result == SOCKET_ERROR)
		err_display("listen() error");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	HANDLE hThread;

	while (1)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		//printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(clientaddr.sin_addr), clientIP, 32 - 1);
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", clientIP, ntohs(clientaddr.sin_port));

		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) {
			closesocket(client_sock);
		}
		else {
			CloseHandle(hThread);
		}
	}

	closesocket(listen_sock);
	WSACleanup();
	return 0;
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << "[" << msg << "] " << (char *)lpMsgBuf << std::endl;
	//printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void PrintMessage(const char* message)
{
	coutMutex.lock();
	std::cout << message << std::endl;
	coutMutex.unlock();
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	int result;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFFERSIZE + 1];

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	char clientIP[32] = { 0, };
	inet_ntop(AF_INET, &(clientaddr.sin_addr), clientIP, 32 - 1);

	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ� = %s,  ��Ʈ ��ȣ = %d] %s\n", clientIP, ntohs(clientaddr.sin_port));

	while (true)
	{
		result = recv(client_sock, buf, BUFFERSIZE, 0);
		if (result == SOCKET_ERROR) 
		{
			err_display("recv()");
			break;
		}
		else if (result == 0)
			break;

		// ���� ������ ���
		buf[result] = '\0';
		printf("[TCP/%s:%d] %s\n", clientIP, ntohs(clientaddr.sin_port), buf);

		// ������ ������
		result = send(client_sock, buf, result, 0);
		if (result == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
	}

	closesocket(client_sock);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", clientIP, ntohs(clientaddr.sin_port));
}