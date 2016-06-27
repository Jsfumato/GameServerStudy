// nana_Client_chat.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <nana/gui.hpp>
#include <nana\/gui/widgets/label.hpp>
#include <memory>
#include <unordered_map>
#include <functional>
#include "LoginScene.h"
#include "TCPClient.h"

int main()
{
	std::unordered_map<clientState, std::function<void(void)>> stateDictionary;
	
	TCPClient myclient = TCPClient();
	myclient.ConnectTo("127.0.0.1", 23452);
	
	std::unique_ptr<LoginScene> loginScene = std::make_unique<LoginScene>();
	loginScene->Init();
	loginScene->Update();

    return 0;
}

