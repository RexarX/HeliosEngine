﻿#include <HeliosEngine.h>
#include <HeliosEngine/EntryPoint.h>

#include "Layers/GameLayer.h"

class Game final : public Helios::Application
{
public:
	Game() : Helios::Application()
	{
		PushLayer(new GameLayer());
	}

	~Game()
	{
	}
};

Helios::Application* Helios::CreateApplication()
{
	return new Game();
}