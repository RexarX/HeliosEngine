#pragma once

#include "Application.h"

#ifdef PLATFORM_WINDOWS

int main(int argc, char** argv)
{
	Helios::Log::initialize();
	CORE_WARN("Initialized Log!");

	auto app = Helios::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif