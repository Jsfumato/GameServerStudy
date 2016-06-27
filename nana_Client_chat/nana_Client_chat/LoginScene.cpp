#include "stdafx.h"
#include "LoginScene.h"

void LoginScene::Init()
{
	client = std::make_unique<TCPClient>();
	SetNetwork(client.get());
}

void LoginScene::SetGUI()
{
	// ip & port
	ipTextBox = std::make_unique<nana::textbox>((nana::form&)*sceneForm, nana::rectangle{ 43, 15, 128, 20 });
	ipTextBox->caption("127.0.0.1");

	portTextBox = std::make_unique<nana::textbox>((nana::form&)*sceneForm, nana::rectangle{ 43, 15, 128, 20 });
	portTextBox->caption("23452");

	connectButton = std::make_unique<nana::button>((nana::form&)*sceneForm, nana::rectangle(353, 54, 102, 23));
	connectButton->caption("Connect to Server");
	connectButton->events().click(
		[&]() {
		this->ConnectOrDisconnect();
	});

	// id & password
	idTextBox = std::make_unique<nana::textbox>((nana::form&)*sceneForm, nana::rectangle{ 43, 15, 128, 20 });
	idTextBox->caption("127.0.0.1");

	passwordTextBox = std::make_unique<nana::textbox>((nana::form&)*sceneForm, nana::rectangle{ 43, 15, 128, 20 });
	passwordTextBox->caption("23452");

	loginButton = std::make_unique<nana::button>((nana::form&)*sceneForm, nana::rectangle(353, 54, 102, 23));
	loginButton->caption("Login");
	loginButton->events().click(
		[&]() {
			this->LogInOut();
		});
	loginButton->enabled(false);
}

void LoginScene::Update()
{
	
}

void LoginScene::PacketProcess()
{
	auto packet = client->GetPacket();
	if (packet.PacketId != 0)
	{


		if (packet.pData != nullptr) {
			delete[] packet.pData;
		}
	}
}