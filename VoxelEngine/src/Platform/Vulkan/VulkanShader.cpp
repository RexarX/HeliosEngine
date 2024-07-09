#include "VulkanShader.h"
#include "VulkanContext.h"
#include "VulkanUniformBuffer.h"

#include <shaderc/shaderc.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace VoxelEngine 
{
	static vk::ShaderStageFlagBits ShaderTypeFromString(const std::string_view type)
	{
		if (type == "vertex") { return vk::ShaderStageFlagBits::eVertex; }
		else if (type == "fragment") { return vk::ShaderStageFlagBits::eFragment; }
		else if (type == "compute") { return vk::ShaderStageFlagBits::eCompute; }
		
		CORE_ASSERT(false, "Unknown shader type!");
		return vk::ShaderStageFlagBits::eAll;
	}

	shaderc_shader_kind translateShaderStage(const vk::ShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case vk::ShaderStageFlagBits::eVertex: return shaderc_vertex_shader;
		case vk::ShaderStageFlagBits::eFragment: return shaderc_fragment_shader;
		case vk::ShaderStageFlagBits::eCompute: return shaderc_compute_shader;
		case vk::ShaderStageFlagBits::eGeometry: return shaderc_geometry_shader;
		case vk::ShaderStageFlagBits::eTessellationControl: return shaderc_tess_control_shader;
		case vk::ShaderStageFlagBits::eTessellationEvaluation: return shaderc_tess_evaluation_shader;
		}

		CORE_ASSERT(false, "Unknown shader stage!");
	}

	bool VulkanShader::GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, const std::string& glslShader,
															 const std::string& fileName, std::vector<uint32_t>& spvShader) const
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
			glslShader, translateShaderStage(shaderType),
			fileName.c_str(),
			options
		);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
      CORE_ERROR(module.GetErrorMessage());
			return false;
		}

		spvShader = { module.cbegin(), module.cend() };

		return true;
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
	}

	VulkanShader::VulkanShader(const std::string& vertex, const std::string& fragment)
	{
		if (vertex.rfind(".spv") != std::string::npos && fragment.rfind(".spv") != std::string::npos) {
			m_Compiled = true;
		}

		std::string vertexSrc = ReadFile(vertex);
    std::string fragmentSrc = ReadFile(fragment);

		std::unordered_map<vk::ShaderStageFlagBits, std::pair<std::string, std::string>> sources;

    sources[vk::ShaderStageFlagBits::eVertex].first = vertex;
    sources[vk::ShaderStageFlagBits::eFragment].first = fragment;

		sources[vk::ShaderStageFlagBits::eVertex].second = vertexSrc;
		sources[vk::ShaderStageFlagBits::eFragment].second = fragmentSrc;

		Compile(sources);
	}

	VulkanShader::~VulkanShader()
	{
	}

	std::string VulkanShader::ReadFile(const std::string& filepath)
	{
		INFO("Reading file '{0}'", filepath);
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in) {
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(result.data(), result.size());
			in.close();
		} 
		else { CORE_ERROR("Could not open file '{0}'!", filepath); }
		return result;
	}

	std::unordered_map<vk::ShaderStageFlagBits, std::pair<std::string, std::string>> VulkanShader::PreProcess(const std::string& source)
	{
		std::unordered_map<vk::ShaderStageFlagBits, std::pair<std::string, std::string>> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos) {
			size_t eol = source.find_first_of("\r\n", pos);
			CORE_ASSERT(eol != std::string::npos, "Syntax error!");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			CORE_ASSERT(ShaderTypeFromString(type) != vk::ShaderStageFlagBits::eAll, "Invalid shader type specified!");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)].second = source.substr(nextLinePos,
				pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void VulkanShader::Compile(std::unordered_map<vk::ShaderStageFlagBits, std::pair<std::string, std::string>>& shaderSources)
	{
		std::vector<uint32_t> spirv;
		for (const auto& src : shaderSources) {
			if (m_Compiled) {
				spirv.resize(src.second.second.size() / sizeof(uint32_t));
				memcpy(spirv.data(), src.second.second.data(), src.second.second.size());
			} else {
				bool result = GLSLtoSPV(src.first, src.second.second, src.second.first, spirv);
				CORE_ASSERT(result, "Failed to compile shader!");
			}

			vk::ShaderModuleCreateInfo info;
			info.sType = vk::StructureType::eShaderModuleCreateInfo;
			info.flags = vk::ShaderModuleCreateFlags();
			info.codeSize = spirv.size() * sizeof(uint32_t);
			info.pCode = spirv.data();

			vk::ShaderModule shaderModule = VulkanContext::Get().GetDevice().createShaderModule(info);
			CORE_ASSERT(shaderModule, "Failed to create shader module!");
			
			vk::PipelineShaderStageCreateInfo shaderStageCreateInfo;
			shaderStageCreateInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
			shaderStageCreateInfo.flags = vk::PipelineShaderStageCreateFlags();
			shaderStageCreateInfo.stage = src.first;
			shaderStageCreateInfo.module = shaderModule;
			shaderStageCreateInfo.pName = "main";

			m_ShaderStageCreateInfo.emplace_back(shaderStageCreateInfo);

			spirv.clear();
		}
	}

	void VulkanShader::Bind() const
	{
	}

	void VulkanShader::Unbind() const
	{
	}

	void VulkanShader::AddUniform(const void* data, const uint32_t size)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		vk::PushConstantRange range;
		range.offset = 0;
		range.size = size;
		range.stageFlags = vk::ShaderStageFlagBits::eVertex;

		pipeline.pipelineBuilder.pushConstantRanges.emplace_back(range);

		pipeline.pushConstant = data;
		pipeline.pushConstantSize = size;*/
	}

	void VulkanShader::UploadUniformInt(const std::string& name, const int value)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = &value;
		pipeline.pushConstantSize = sizeof(int);*/
	}

	void VulkanShader::UploadUniformFloat(const std::string& name, const float value)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = &value;
		pipeline.pushConstantSize = sizeof(float);*/
	}

	void VulkanShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = glm::value_ptr(value);
		pipeline.pushConstantSize = sizeof(glm::vec2);*/
	}

	void VulkanShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = glm::value_ptr(value);
		pipeline.pushConstantSize = sizeof(glm::vec3);*/
	}

	void VulkanShader::UploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = glm::value_ptr(value);
		pipeline.pushConstantSize = sizeof(glm::vec4);*/
	}

	void VulkanShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = glm::value_ptr(matrix);
		pipeline.pushConstantSize = sizeof(glm::mat3);*/
	}

	void VulkanShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = glm::value_ptr(matrix);
		pipeline.pushConstantSize = sizeof(glm::mat4);*/
	}

	void VulkanShader::UploadUniformData(const std::string& name, const void* data, const uint32_t size)
	{
		/*Pipeline& pipeline = VulkanContext::Get().GetPipeline(m_ID);

		pipeline.pushConstant = data;
		pipeline.pushConstantSize = size;*/
	}
}