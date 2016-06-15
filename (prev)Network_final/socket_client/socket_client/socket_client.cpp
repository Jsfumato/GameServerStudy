// socket_client.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Ws2tcpip.h>

#define PORT_NUM 8260
#define SIZE_BUFFER 1024

void Errorhandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void PrintMsg(SOCKET hSocket, char* message)
{
	int strLen = 0;
	if ((strLen = recv(hSocket, message, SIZE_BUFFER - 1, 0)) != 0)
	{
		message[strLen - 1] = 0;
		printf("%s \n", message);
	}
}

int main()
{
	WSADATA wsaData;

	SOCKET hSocket;
	SOCKADDR_IN servAddr;
	
	char message[SIZE_BUFFER];

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		Errorhandling("WSAStartup() error");

	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		Errorhandling("socket() error");

	char ipAddress[32];

	fputs("서버 컴퓨터의 ip를 입력하세요 : ", stdout);
	fgets(ipAddress, 32, stdin);
	ipAddress[strlen(ipAddress) - 1] = '\0';

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ipAddress, &servAddr.sin_addr);
	servAddr.sin_port = htons(PORT_NUM);

	if (connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		Errorhandling("connect() error");
	else
		puts("채팅방에 연결되었습니다!\n채팅방 내용이 해당 페이지에 출력됩니다.");
	
	int strLen = 0;
	while (1)
	{
		if ((strLen = recv(hSocket, message, SIZE_BUFFER - 1, 0)) != 0)
		{
			message[strLen - 1] = 0;
			printf("%s \n", message);
		}
	}
	
	closesocket(hSocket);
	WSACleanup();

	return 0;
}

