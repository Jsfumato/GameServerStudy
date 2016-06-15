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

char* AddNameAndMsg(char* name, char* msg)
{
	char newMsg[SIZE_BUFFER];
	for (int i = 0; i < SIZE_BUFFER; ++i)
		newMsg[i] = NULL;

	strcat_s(newMsg, 3, "[ ");
	strcat_s(newMsg, strlen(name) + strlen(newMsg) + 1, name);
	strcat_s(newMsg, strlen(newMsg) + 6, " ] : ");
	strcat_s(newMsg, strlen(newMsg) + strlen(msg) + 1, msg);

	return newMsg;
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
		puts("Connected!");

	char name[20];
	char cpyName[20];

	while (1)
	{
		fputs("이름을 입력하세요 : ", stdout);
		fgets(name, 20, stdin);
		name[strlen(name) - 1] = '\0';

		if (strlen(name) > 20)
		{
			system("cls");
			fputs("이름이 너무 깁니다! 다시 입력하세요", stdout);
		}
		else
		{
			fputs("입장합니다...", stdout);
			strcpy_s(cpyName, 20, name);
			char* message = "님이 입장하셨습니다.";

			char* newMsg = AddNameAndMsg(cpyName, message);
			int sendRet = send(hSocket, newMsg, strlen(newMsg), 0);

			if (sendRet == SOCKET_ERROR)
				continue;
			
			break;
		}
	}
	


	while (1)
	{
		system("cls");
		strcpy_s(cpyName, 20, name);

		fputs("이름 : ", stdout);
		fputs(cpyName, stdout);
		fputs("\n메시지를 입력하세요(Q to quit)\n256자를 넘길시 전송되지 않습니다! : \n", stdout);
		fgets(message, SIZE_BUFFER, stdin);

		if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
		{
			strcpy_s(cpyName, 20, name);
			char* message = "님이 퇴장하셨습니다.";

			char* newMsg = AddNameAndMsg(cpyName, message);
			int sendRet = send(hSocket, newMsg, strlen(newMsg), 0);
			break;
		}
			
		
		if (strlen(message) > 256)
			continue;

		char* newMsg = AddNameAndMsg(cpyName, message);

		int sendRet = send(hSocket, newMsg, strlen(newMsg), 0);
		
		if(sendRet == SOCKET_ERROR)
			fputs("전송 에러!", stdout);
	}

	closesocket(hSocket);
	WSACleanup();

	return 0;
}

