#pragma once

#include "Application.h"
#include "Config/ConfigManager.h"

extern inline Helios::Application* Helios::CreateApplication();

int main(int argc, char** argv) {
	Helios::Log::Init();
	PROFILE_BEGIN_SESSION("Initialization");

	Helios::ConfigManager::Get().ParseCommandLineArgs(argc, argv);

	Helios::Application* app = Helios::CreateApplication();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Runtime");
	app->Run();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Shutdown");
	delete app;
	PROFILE_END_SESSION();

	return 0;
}