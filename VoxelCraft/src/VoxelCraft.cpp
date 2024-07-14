#include <VoxelEngine.h>
#include <VoxelEngine/EntryPoint.h>

#include "GameLayer.h"

class VoxelCraft : public Engine::Application
{
public:
	VoxelCraft() : Engine::Application()
	{
		PushLayer(new GameLayer());
	}

	~VoxelCraft()
	{
	}
};

Engine::Application* Engine::CreateApplication()
{
	return new VoxelCraft();
}