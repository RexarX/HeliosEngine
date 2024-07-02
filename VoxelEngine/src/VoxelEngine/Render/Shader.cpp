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
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(filepath);
		case RendererAPI::API::Vulkan:  return std::make_shared<VulkanShader>(filepath);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::shared_ptr<Shader> Shader::Create(const char* name, const std::string& vertex,
																													 const std::string& fragment)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    VE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(name, vertex, fragment);
    case RendererAPI::API::Vulkan:  return std::make_shared<VulkanShader>(name, vertex, fragment);
		}

		VE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	void ShaderLibrary::Add(const char* name, const std::shared_ptr<Shader>& shader)
	{
		VE_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const std::shared_ptr<Shader>& shader)
	{
		Add(shader->GetName(), shader);
	}

	std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(Shader::Create(filepath));
		return shader;
	}

	std::shared_ptr<Shader> ShaderLibrary::Load(const char* name,
																							const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	std::shared_ptr<Shader> ShaderLibrary::Get(const char* name)
	{
		VE_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	bool ShaderLibrary::Exists(const char* name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}
};