#include "RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace VoxelEngine 
{
	RendererAPI::API s_API = RendererAPI::API::OpenGL;
}