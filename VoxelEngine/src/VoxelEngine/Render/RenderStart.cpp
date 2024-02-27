#include "vepch.h"

#include "RenderStart.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace VoxelEngine
{
	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;
}