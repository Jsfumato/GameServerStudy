#pragma once
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include "Common\Packet.h"
#include "Common\PacketID.h"
#include "Scene.h"

class LoginScene : public Scene
{
public:
	LoginScene() = default;
	~LoginScene() = default;

	void Init();
	void SetGUI();
	void Update();
	void PacketProcess();

private:
	std::unique_ptr<nana::textbox> ipTextBox;
	std::unique_ptr<nana::textbox> portTextBox;
	std::unique_ptr<nana::button> connectButton;

	std::unique_ptr<nana::textbox> idTextBox;
	std::unique_ptr<nana::textbox> passwordTextBox;
	std::unique_ptr<nana::button> loginButton;
};

