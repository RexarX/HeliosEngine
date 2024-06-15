#include "VulkanShader.h"

#include "VulkanUniformBuffer.h"

#include "vepch.h"

#include <shaderc/shaderc.hpp>

namespace VoxelEngine 
{
	static vk::ShaderStageFlagBits ShaderTypeFromString(const std::string_view type)
	{
		if (type == "vertex") { return vk::ShaderStageFlagBits::eVertex; }
		else if (type == "fragment") { return vk::ShaderStageFlagBits::eFragment; }
		else if (type == "compute") { return vk::ShaderStageFlagBits::eCompute; }
		
		VE_CORE_ASSERT(false, "Unknown shader type!");
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

		VE_CORE_ASSERT(false, "Unknown shader stage!");
	}

	const std::string getFileName(const std::string& name, const vk::ShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case vk::ShaderStageFlagBits::eVertex: return name + ".vert";
		case vk::ShaderStageFlagBits::eFragment: return name + ".frag";
		case vk::ShaderStageFlagBits::eCompute: return name + ".comp";
		case vk::ShaderStageFlagBits::eGeometry: return name + ".geom";
		case vk::ShaderStageFlagBits::eTessellationControl: return name + ".tesc";
		case vk::ShaderStageFlagBits::eTessellationEvaluation: return name + ".tese";
		}
	}

	bool VulkanShader::GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, const std::string& glslShader,
															 std::vector<uint32_t>& spvShader) const
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
			glslShader, translateShaderStage(shaderType),
			getFileName(m_Name, shaderType).c_str(),
			options
		);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
      VE_CORE_ERROR(module.GetErrorMessage());
			return false;
		}

		spvShader = { module.cbegin(), module.cend() };

		return true;
	}

	VulkanShader::VulkanShader(const std::string& filepath)
	{
		VulkanContext::Get().SetCurrentComputeEffect(m_Name);

		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);

		Compile(shaderSources);

		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		std::string nameSubstring = filepath.substr(lastSlash, count);
		m_Name = nameSubstring.c_str();
	}

	VulkanShader::VulkanShader(const char* name, const std::string& vertex,
																							 const std::string& fragment)
		: m_Name(name)
	{
		VulkanContext::Get().AddComputeEffect(name);
		VulkanContext::Get().SetCurrentComputeEffect(m_Name);

		if (vertex.rfind(".spv") != std::string::npos || fragment.rfind(".spv") != std::string::npos) {
			m_Compiled = true;
		}

		std::string vertexSrc = ReadFile(vertex);
    std::string fragmentSrc = ReadFile(fragment);

		std::unordered_map<vk::ShaderStageFlagBits, std::string> sources;
		sources[vk::ShaderStageFlagBits::eVertex] = vertexSrc;
		sources[vk::ShaderStageFlagBits::eFragment] = fragmentSrc;

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
			in.read(result.data(), result.size());
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
			VE_CORE_ASSERT(ShaderTypeFromString(type) != vk::ShaderStageFlagBits::eAll, "Invalid shader type specified!");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = source.substr(nextLinePos,
				pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void VulkanShader::Compile(std::unordered_map<vk::ShaderStageFlagBits, std::string>& shaderSources)
	{
		std::vector<uint32_t> spirv;
		for (const auto& src : shaderSources) {
			if (m_Compiled) {
				spirv.resize(src.second.size() / sizeof(uint32_t));
				memcpy(spirv.data(), src.second.data(), src.second.size());
			} else {
				bool result = GLSLtoSPV(src.first, src.second, spirv);
				
				VE_CORE_ASSERT(result, "Failed to compile shader!");
			}

			vk::ShaderModuleCreateInfo info;
			info.sType = vk::StructureType::eShaderModuleCreateInfo;
			info.flags = vk::ShaderModuleCreateFlags();
			info.codeSize = spirv.size() * sizeof(uint32_t);
			info.pCode = spirv.data();

			vk::ShaderModule shaderModule = VulkanContext::Get().GetDevice().createShaderModule(info);

			VE_CORE_ASSERT(shaderModule, "Failed to create shader module!");

			vk::PipelineShaderStageCreateInfo shaderStageCreateInfo;
			shaderStageCreateInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
			shaderStageCreateInfo.flags = vk::PipelineShaderStageCreateFlags();
			shaderStageCreateInfo.stage = src.first;
			shaderStageCreateInfo.module = shaderModule;
			shaderStageCreateInfo.pName = "main";
			
			VulkanContext::Get().GetComputeEffect(m_Name).
				pipelineBuilder.shaderStages.emplace_back(shaderStageCreateInfo);

			spirv.clear();
		}
	}

	void VulkanShader::Bind() const
	{
		VulkanContext::Get().SetCurrentComputeEffect(m_Name);
	}

	void VulkanShader::Unbind() const
	{
	}

	void VulkanShader::AddUniformBuffer(const std::shared_ptr<UniformBuffer>& uniformBuffer)
	{
		VulkanContext& context = VulkanContext::Get();

		context.GetComputeEffect(m_Name).descriptorLayoutBuilder.
			AddBinding(m_Binding, vk::DescriptorType::eUniformBufferDynamic, vk::ShaderStageFlagBits::eVertex);

		context.GetComputeEffect(m_Name).descriptorAllocator.AddRatios(
			{ vk::DescriptorType::eUniformBufferDynamic, 1 }
		);

		std::shared_ptr<VulkanUniformBuffer> vulkanUniformBuffer = std::static_pointer_cast<VulkanUniformBuffer>(uniformBuffer);

		void* data;
		vmaMapMemory(context.GetAllocator(), vulkanUniformBuffer->GetStagingBuffer().allocation, &data);

		context.ImmediateSubmit([&](const vk::CommandBuffer cmd) {
			vk::BufferCopy vertexCopy;
			vertexCopy.dstOffset = vulkanUniformBuffer->GetOffset();
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vulkanUniformBuffer->GetSize();

			cmd.copyBuffer(vulkanUniformBuffer->GetStagingBuffer().buffer, vulkanUniformBuffer->GetBuffer().buffer,
										 1, &vertexCopy);
			});

		vmaUnmapMemory(context.GetAllocator(), vulkanUniformBuffer->GetStagingBuffer().allocation);

		++m_Binding;
	}

	void VulkanShader::UploadUniformInt(const char* name, const int value)
	{
	}

	void VulkanShader::UploadUniformFloat(const char* name, const float value)
	{
	}

	void VulkanShader::UploadUniformFloat2(const char* name, const glm::vec2& value)
	{
	}

	void VulkanShader::UploadUniformFloat3(const char* name, const glm::vec3& value)
	{
	}

	void VulkanShader::UploadUniformFloat4(const char* name, const glm::vec4& value)
	{
	}

	void VulkanShader::UploadUniformMat3(const char* name, const glm::mat3& matrix)
	{
	}

	void VulkanShader::UploadUniformMat4(const char* name, const glm::mat4& matrix)
	{
		vk::PushConstantRange range;
		range.offset = m_Offset;
		range.size = sizeof(glm::mat4);
		range.stageFlags = vk::ShaderStageFlagBits::eVertex;

		VulkanContext::Get().GetComputeEffect(m_Name).
			pipelineBuilder.pushConstantRanges.emplace_back(range);

		m_Offset += sizeof(glm::mat4);
	}
}