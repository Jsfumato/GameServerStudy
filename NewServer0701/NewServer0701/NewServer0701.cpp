// NewServer0701.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include <thread>
#include <iostream>
#include "../NLogicLib/Main.h"

int main()
{
	NLogicLib::Main main;
	auto a = main.Init();

	std::thread logicThread(
		[&](){ main.Run(); }
	);

	std::cout << "press any key to exit...";
	getchar();

	main.Stop();
	logicThread.join();

	return 0;
}

