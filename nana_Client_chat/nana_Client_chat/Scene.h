#pragma once
#include <nana/gui.hpp>
#include <nana/gui/timer.hpp>
#include <memory>
#include "TCPClient.h"

enum class clientState : short
{
	LOGIN_SESSION = 0
};

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	virtual void Init() = 0;
	virtual void Update() = 0;

//	inline
	void SetNetwork(TCPClient* networkClient)
	{
		client = std::make_unique<TCPClient>(networkClient);
	}

protected:
	std::unique_ptr<TCPClient> client = nullptr;

	std::unique_ptr<nana::form> sceneForm;

	nana::timer m_timer;
};

