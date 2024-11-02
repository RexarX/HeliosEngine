#include "VulkanShader.h"
#include "VulkanContext.h"

#include "Utils/Filesystem.h"

#include <shaderc/shaderc.hpp>

namespace Helios
{
	static VkShaderStageFlagBits translateStageToVulkan(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		CORE_ASSERT(false, "Unknown shader stage!")
	}

	static shaderc_shader_kind translateStageToShaderc(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_vertex_shader;
		case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
		case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_compute_shader;
		case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_geometry_shader;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return shaderc_tess_control_shader;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
		}

		CORE_ASSERT(false, "Unknown shader stage!")
	}

	static bool GLSLtoSPV(VkShaderStageFlagBits shaderType, const std::string& glslShader,
												const std::string& fileName, std::vector<uint32_t>& spvShader)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
			glslShader, translateStageToShaderc(shaderType),
			fileName.c_str(),
			options
		);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			CORE_ERROR(module.GetErrorMessage())
			return false;
		}

		spvShader = { module.cbegin(), module.cend() };

		return true;
	}

	VulkanShader::VulkanShader(const std::initializer_list<ShaderInfo>& infos)
	{
		for (const ShaderInfo& info : infos) {
			VulkanShaderInfo newInfo{};
			newInfo.path = info.path;
			newInfo.stage = translateStageToVulkan(info.stage);
			newInfo.shaderModule = VK_NULL_HANDLE;

			m_ShaderInfos.push_back(newInfo);
		}

		Load();
	}

	VulkanShader::~VulkanShader()
	{
		VkDevice device = VulkanContext::Get().GetDevice();

		for (VulkanShaderInfo& info : m_ShaderInfos) {
			vkDestroyShaderModule(device, info.shaderModule, nullptr);
		}
	}

	void VulkanShader::Load()
	{
		VkDevice device = VulkanContext::Get().GetDevice();

		for (VulkanShaderInfo& info : m_ShaderInfos) {
			std::vector<uint32_t> spvCode;

			std::filesystem::path path(info.path);
			std::string code = Utils::ReadFileToString(info.path);
			if (code.empty()) {
				CORE_ERROR("Failed to load shader code: {0}!", info.path)
				continue;
			}

			if (path.extension() != ".spv") {
				std::string name = path.filename().string();
				uint64_t index = name.find_last_of('.');
				if (index != std::string::npos) { name.resize(index); }
				if (!GLSLtoSPV(info.stage, code, name, spvCode)) {
					CORE_ERROR("Failed to compile shader: {0}!", info.path)
					continue;
				} else {
					spvCode.resize(code.size() / sizeof(uint32_t));
					memcpy(spvCode.data(), code.data(), code.size());
				}
			}

			VkShaderModuleCreateInfo shaderModuleInfo{};
			shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleInfo.flags = 0;
			shaderModuleInfo.codeSize = spvCode.size() * sizeof(uint32_t);
			shaderModuleInfo.pCode = spvCode.data();

			VkResult result = vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &info.shaderModule);
			CORE_ASSERT(result == VK_SUCCESS, "Failed to create shader module: {0}!", info.path)
		}
	}
}