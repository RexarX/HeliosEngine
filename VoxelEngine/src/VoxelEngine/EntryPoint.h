#pragma once

#include "Application.h"

#ifdef VE_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
	VoxelEngine::Log::initialize();
	CORE_WARN("Initialized Log!");

	auto app = VoxelEngine::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif