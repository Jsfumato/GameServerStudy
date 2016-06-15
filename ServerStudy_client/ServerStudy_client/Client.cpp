#include "Client.h"
#include <iostream>

void Client::err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void Client::err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void Client::SetServerIP()
{
	std::string buffer;

	std::cout << "������ ������ IP�� �Է��ϼ���. ( �⺻�� : 127.0.0.1 )" << std::endl;
	std::cout << "[ Server IP �Է� ] : " << std::endl;
	//std::cin >> buffer;
	std::getline(std::cin, buffer);
	
	if(buffer != "")
		serverIP = buffer;

	std::cout << serverIP << std::endl;
}

void Client::SetServerPort()
{
	std::string buffer;

	std::cout << "������ ������ Port�� �Է��ϼ���. ( �⺻�� : 23452 )" << std::endl;
	std::cout << "[ Server Port �Է� ] : " << std::endl;
	//std::cin >> buffer;
	std::getline(std::cin, buffer);

	if (buffer != "")
		serverPort = atoi(buffer.c_str());

	std::cout << serverPort << std::endl;
}

int Client::RecieveMassage(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

bool Client::ConnectServer()
{
	int result;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		err_quit("WSAStartup Error!!");
	}

	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		err_display("socket()");

	// connect()
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;

	auto ret = inet_pton(AF_INET, serverIP.c_str(), (void *)&serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(serverPort);

	result = connect(sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR) 
	{
		err_display("connect()");
	}

	std::cout << "[ Connect to Server ]" << std::endl;
	std::cout << serverIP << ":" << serverPort << std::endl;
	return true;
}

void Client::SendMassageToServer()
{
	int result;
	char buf[bufferSize + 1];
	int len;

	// ������ ������ ���
	while (1) {
		// ������ �Է�
		printf("\n[���� ������] ");
		if (fgets(buf, bufferSize + 1, stdin) == NULL)
			break;

		// '\n' ���� ����
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		// ������ ������
		result = send(sock, buf, strlen(buf), 0);
		if (result == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n", result);

		// ������ �ޱ�
		result = RecieveMassage(sock, buf, result, 0);
		if (result == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (result == 0)
			break;

		// ���� ������ ���
		buf[result] = '\0';
		printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� �޾ҽ��ϴ�.\n", result);
		printf("[���� ������] %s\n", buf);
	}
}
