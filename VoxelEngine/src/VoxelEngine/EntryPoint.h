#pragma once

#ifdef VE_PLATFORM_WINDOWS

extern VoxelEngine::Application* VoxelEngine::CreateApplication();

int main(int argc, char** argv)
{
	VoxelEngine::Log::initialize();
	VE_CORE_WARN("Initialized Log!");

	auto app = VoxelEngine::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif