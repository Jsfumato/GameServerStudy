#include "Server.h"

bool Server::Initialize()
{
	int result;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_display("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	result = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (result == SOCKET_ERROR) err_display("bind()");

	// listen()
	result = listen(listen_sock, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		err_display("listen()");
	}

	std::cout << "Server Initialize Completed" << std::endl;
	std::cout << "Server IP : " << INADDR_ANY << std::endl;
	std::cout << "Server Port : " << SERVERPORT << std::endl;
}

void Server::DoServer()
{
	int result;
	
	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFFERSIZE + 1];
	
	while (true)
	{
		// accept
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept() error");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		//printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(clientaddr.sin_addr), clientIP, 32 - 1);
		// http://www.joinc.co.kr/w/Site/TCP_IP/IPv6/IPv6Prog
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", clientIP, ntohs(clientaddr.sin_port));

		// Ŭ���̾�Ʈ�� ������ ���
		while (1) {
			// ������ �ޱ�
			result = recv(client_sock, buf, BUFFERSIZE, 0);
			if (result == SOCKET_ERROR) {
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
}

void Server::err_display(char *message)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", message, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}