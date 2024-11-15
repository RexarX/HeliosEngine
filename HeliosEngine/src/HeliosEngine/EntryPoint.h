#pragma once

#include "Application.h"

int main(int argc, char** argv) {
	Helios::Log::Init();

	Helios::Application* app = Helios::CreateApplication();
	app->Run();
	delete app;

	return 0;
}