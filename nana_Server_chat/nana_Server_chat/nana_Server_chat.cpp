// nana_Server_chat.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include <thread>
#include "../LogicLib/Main.h"
#include <iostream>

int main()
{
	NLogicLib::Main main;
	main.Init();

	std::thread logicThread([&]() {
		main.Run(); }
	);

	std::cout << "press any key to exit...";
	getchar();

	main.Stop();
	logicThread.join();

	return 0;
}

