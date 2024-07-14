#pragma once

#include "Application.h"

#ifdef PLATFORM_WINDOWS

int main(int argc, char** argv)
{
	Engine::Log::initialize();
	CORE_WARN("Initialized Log!");

	auto app = Engine::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif