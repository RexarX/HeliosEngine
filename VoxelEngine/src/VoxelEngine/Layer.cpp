#include "vepch.h"

#include "Layer.h"

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