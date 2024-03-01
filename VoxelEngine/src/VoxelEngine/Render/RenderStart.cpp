#include "RenderStart.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace VoxelEngine
{
	std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = std::make_unique<OpenGLRendererAPI>();
}