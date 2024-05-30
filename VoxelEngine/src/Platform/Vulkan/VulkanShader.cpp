#include "vepch.h"

#include "VulkanShader.h"

#include "VulkanContext.h"

#include <glm/gtc/type_ptr.hpp>

namespace VoxelEngine 
{
	static vk::ShaderStageFlagBits ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex") { return vk::ShaderStageFlagBits::eVertex; }
		if (type == "fragment") { return vk::ShaderStageFlagBits::eFragment; }

		VE_CORE_ASSERT(false, "Unknown shader type!");
		return vk::ShaderStageFlagBits::eAll;
	}

	VulkanShader::VulkanShader(const std::string& filepath)
	{
		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);
		Compile(shaderSources);

		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertex,
														 const std::string& fragment)
		: m_Name(name)
	{
		std::unordered_map<vk::ShaderStageFlagBits, std::string> sources;
		sources[vk::ShaderStageFlagBits::eVertex] = vertex;
		sources[vk::ShaderStageFlagBits::eFragment] = fragment;
		Compile(sources);
	}

	VulkanShader::~VulkanShader()
	{
		
	}

	std::string VulkanShader::ReadFile(const std::string& filepath)
	{
		VE_INFO("Reading file '{0}'", filepath);
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in) {
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		} 
		else { VE_CORE_ERROR("Could not open file '{0}'", filepath); }
		return result;
	}

	std::unordered_map<vk::ShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& source)
	{
		std::unordered_map<vk::ShaderStageFlagBits, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos) {
			size_t eol = source.find_first_of("\r\n", pos);
			VE_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			VE_CORE_ASSERT(ShaderTypeFromString(type) != vk::ShaderStageFlagBits::eAll, "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = source.substr(nextLinePos,
				pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void VulkanShader::Compile(std::unordered_map<vk::ShaderStageFlagBits, std::string>& shaderSources)
	{
		auto vertShaderModule = VulkanContext::Get().GetDevice().createShaderModuleUnique({
								vk::ShaderModuleCreateFlags(),
								shaderSources[vk::ShaderStageFlagBits::eVertex].size(),
								reinterpret_cast<const uint32_t*>(shaderSources[vk::ShaderStageFlagBits::eVertex].data())
			});

		VE_CORE_ASSERT(vertShaderModule, "Failed to create vertex shader module!");

		auto fragShaderModule = VulkanContext::Get().GetDevice().createShaderModuleUnique({
								vk::ShaderModuleCreateFlags(),
								shaderSources[vk::ShaderStageFlagBits::eFragment].size(),
								reinterpret_cast<const uint32_t*>(shaderSources[vk::ShaderStageFlagBits::eFragment].data())
			});

		VE_CORE_ASSERT(fragShaderModule, "Failed to create fragment shader module!");

		VulkanContext::Get().GetPipelineData().shaderStages.push_back(
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				*vertShaderModule,
				m_Name.data()
			)
		);

		VulkanContext::Get().GetPipelineData().shaderStages.push_back(
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				*fragShaderModule,
				m_Name.data()
			)
		);
	}

	void VulkanShader::Bind() const
	{
		
	}

	void VulkanShader::Unbind() const
	{
		
	}

	void VulkanShader::UploadUniformInt(const std::string& name, const int value)
	{
		
	}

	void VulkanShader::UploadUniformFloat(const std::string& name, const float value)
	{
		
	}

	void VulkanShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		
	}

	void VulkanShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		
	}

	void VulkanShader::UploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		
	}

	void VulkanShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		
	}

	void VulkanShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		
	}
}