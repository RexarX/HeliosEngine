#include <VoxelEngine.h>
#include <VoxelEngine/EntryPoint.h>

#include "GameLayer.h"

class VoxelCraft : public VoxelEngine::Application
{
public:
	VoxelCraft()
		: VoxelEngine::Application()
	{
		PushLayer(new GameLayer());
	}

	~VoxelCraft()
	{
	}
};

VoxelEngine::Application* VoxelEngine::CreateApplication()
{
	return new VoxelCraft();
}