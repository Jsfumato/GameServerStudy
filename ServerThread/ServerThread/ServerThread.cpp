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
	//winsock은 반드시 WSAStartup을 해야한다.
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

	// 데이터 통신에 사용할 변수
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

		// 접속한 클라이언트 정보 출력
		//printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(clientaddr.sin_addr), clientIP, 32 - 1);
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", clientIP, ntohs(clientaddr.sin_port));

		// 스레드 생성
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

// 소켓 함수 오류 출력
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

	printf("[TCP 서버] 클라이언트 접속: IP 주소 = %s,  포트 번호 = %d] %s\n", clientIP, ntohs(clientaddr.sin_port));

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

		// 받은 데이터 출력
		buf[result] = '\0';
		printf("[TCP/%s:%d] %s\n", clientIP, ntohs(clientaddr.sin_port), buf);

		// 데이터 보내기
		result = send(client_sock, buf, result, 0);
		if (result == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
	}

	closesocket(client_sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", clientIP, ntohs(clientaddr.sin_port));
}