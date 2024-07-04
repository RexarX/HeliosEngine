#include "Texture.h"
#include "Renderer.h"

#include "vepch.h"

#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace VoxelEngine
{
	std::shared_ptr<Texture> Texture::Create(const TextureSpecification& specification)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture>(specification);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanTexture>(specification);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<Texture> Texture::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture>(path);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanTexture>(path);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<Texture> Texture::Create(const std::string& right, const std::string& left,
		const std::string& top, const std::string& bottom,
		const std::string& back, const std::string& front)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture>(right, left, top, bottom, back, front);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanTexture>(right, left, top, bottom, back, front);
		}
		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	void Texture::SetGenerateMipmaps(const bool value)
	{
		switch (Renderer::GetAPI())
		{
    case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return;
    case RendererAPI::API::OpenGL:  OpenGLTexture::SetGenerateMipmaps(value); break;
    case RendererAPI::API::Vulkan:  VulkanTexture::SetGenerateMipmaps(value); break;
		}
	}

	void Texture::SetAnisoLevel(const float value)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return;
		case RendererAPI::API::OpenGL:  OpenGLTexture::SetAnisoLevel(value); break;
		case RendererAPI::API::Vulkan:  VulkanTexture::SetAnisoLevel(value); break;
		}
	}
}