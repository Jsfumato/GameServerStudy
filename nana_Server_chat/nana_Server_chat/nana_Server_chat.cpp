// nana_Server_chat.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
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

