#include "vepch.h"

#include "VoxelEngine/Layer.h"

namespace VoxelEngine 
{
	Layer::Layer(const std::string& debugName)
		: m_DebugName(debugName)
	{
	}

	Layer::~Layer()
	{
	}
}