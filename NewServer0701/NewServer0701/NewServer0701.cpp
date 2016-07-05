// NewServer0701.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
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

