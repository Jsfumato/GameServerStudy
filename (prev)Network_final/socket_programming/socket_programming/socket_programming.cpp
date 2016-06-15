// socket_programming.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <process.h>

#define PORT_NUM 8260
#define MAX_USER 4
#define SIZE_BUFFER 1024

int client_sock[200];

void Errorhandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main()
{
	char buffer[SIZE_BUFFER];

	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAddr, clntAddr;

	int szClntAddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		Errorhandling("WSAStartup() error");

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET)
		Errorhandling("socket() error");

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(PORT_NUM);

	if (bind(hServSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		Errorhandling("bind() error");
	if (listen(hServSock, 20) == SOCKET_ERROR)
		Errorhandling("listen() error");

	szClntAddr = sizeof(clntAddr);
	int strLength = 0;

//	멀티플렉싱을 위한 fd set

	int chatNum = 0;
	client_sock[chatNum++] = hServSock;

	TIMEVAL timeout;
	fd_set reads, cpyReads, cpyReads2;
	int fdNum;

	printf("서버가 열렸습니다!\n");

	while (1)
	{
		FD_ZERO(&reads);
		FD_ZERO(&cpyReads);
		FD_SET(hServSock, &reads);
		for (int i = 1; i < chatNum; ++i)
			FD_SET(client_sock[i], &reads);

		cpyReads = reads;
		timeout.tv_usec = 50;

		if ((fdNum = select(0, &cpyReads, 0, 0, &timeout)) == SOCKET_ERROR)
		{
			WSAGetLastError();
			break;
		}

		if (fdNum == 0)
			continue;

		FD_ZERO(&cpyReads2);
		for (int i = 0; i < fdNum; ++i)
			FD_SET(cpyReads.fd_array[i], &cpyReads2);
		cpyReads = cpyReads2;

		for (int i = 0; i < reads.fd_count; ++i)
		{
			if (FD_ISSET(reads.fd_array[i], &cpyReads))
			{
				if (reads.fd_array[i] == hServSock)
				{
					int addressSize = sizeof(clntAddr);
					hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addressSize);
					FD_SET(hClntSock, &reads);
					client_sock[chatNum++] = hClntSock;
					printf("connected client : %d,	chatNum : %d\n", hClntSock, (chatNum-1));
				}
				else
				{
					strLength = recv(reads.fd_array[i], buffer, SIZE_BUFFER-1, 0);
					if (strLength <= 0)
					{
						int index = 0;
						for (int j = 0; j < chatNum; ++j)
						{
							if ((client_sock[j] != hServSock) && (client_sock[j] == reads.fd_array[i]))
							{
								index = j;
								break;
							}
						}

						int fd = reads.fd_array[i];
						closesocket(reads.fd_array[i]);
						FD_CLR(reads.fd_array[i], &reads);

						for (int i = index; i < chatNum; ++i)
							client_sock[i] = client_sock[i+1];
						chatNum--;
						printf("closed client: %d\n", fd);
					}
					else
					{
						for (int j = 0; j < chatNum; j++)
							send(client_sock[j], buffer, strLength, 0);
					}
				}
			}
		}
	}

	closesocket(hServSock);
	WSACleanup();

	return 0;
}
