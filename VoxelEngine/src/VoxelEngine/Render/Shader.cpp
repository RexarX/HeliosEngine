#include "Shader.h"
#include "Renderer.h"

#include "vepch.h"

#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace VoxelEngine
{
	std::shared_ptr<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(filepath);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanShader>(filepath);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<Shader> Shader::Create(const std::string& vertex, const std::string& fragment)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(vertex, fragment);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanShader>(vertex, fragment);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	void ShaderLibrary::Add(const uint32_t id, const std::shared_ptr<Shader>& shader)
	{
		CORE_ASSERT(!Exists(id), "Shader already exists!");
		m_Shaders[id] = shader;
	}

	void ShaderLibrary::Add(const std::shared_ptr<Shader>& shader)
	{
		Add(shader->GetID(), shader);
	}

	std::shared_ptr<Shader> ShaderLibrary::Load(const uint32_t id, const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(id, shader);
		return shader;
	}

	std::shared_ptr<Shader> ShaderLibrary::Get(const uint32_t id)
	{
		CORE_ASSERT(Exists(id), "Shader not found!");
		return m_Shaders[id];
	}

	bool ShaderLibrary::Exists(const uint32_t id) const
	{
		return m_Shaders.find(id) != m_Shaders.end();
	}
};