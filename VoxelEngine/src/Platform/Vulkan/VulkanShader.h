#pragma once

#include "VoxelEngine/Render/Shader.h"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

namespace VoxelEngine
{
	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const std::string& filepath);
		VulkanShader(const std::string& name, const std::string& vertex,
								 const std::string& fragment);
		virtual ~VulkanShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual const std::string& GetName() const override { return m_Name; }

		virtual void UploadUniformInt(const std::string& name, const int value) override;

		virtual void UploadUniformFloat(const std::string& name, const float value) override;
		virtual void UploadUniformFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void UploadUniformFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void UploadUniformFloat4(const std::string& name, const glm::vec4& value) override;

		virtual void UploadUniformMat3(const std::string& name, const glm::mat3& matrix) override;
		virtual void UploadUniformMat4(const std::string& name, const glm::mat4& matrix) override;

	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<vk::ShaderStageFlagBits, std::string> PreProcess(const std::string& source);
		void Compile(std::unordered_map<vk::ShaderStageFlagBits, std::string>& shaderSources);

	private:
		uint32_t m_RendererID;
		std::string m_Name;
	};
}