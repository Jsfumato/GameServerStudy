#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "Server.h"

int main()
{
	Server server = Server();

	server.Initialize();
	server.DoServer();

    return 0;
}