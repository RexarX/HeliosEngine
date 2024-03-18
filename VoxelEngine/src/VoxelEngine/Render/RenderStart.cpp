#include "RenderStart.h"

namespace VoxelEngine
{
	std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}