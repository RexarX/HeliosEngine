#include <HeliosEngine.h>
#include <HeliosEngine/EntryPoint.h>

#include "Layers/GameLayer.h"

class VoxelCraft : public Helios::Application
{
public:
	VoxelCraft() : Helios::Application()
	{
		PushLayer(new GameLayer());
	}

	~VoxelCraft()
	{
	}
};

Helios::Application* Helios::CreateApplication()
{
	return new VoxelCraft();
}