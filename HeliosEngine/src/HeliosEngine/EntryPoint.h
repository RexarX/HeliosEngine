#pragma once

#include "Application.h"

int main(int argc, char** argv)
{
	Helios::Log::Init();
	CORE_INFO("Initialized Log!");

	Helios::Application* app = Helios::CreateApplication();
	app->Run();
	delete app;

	return 0;
}