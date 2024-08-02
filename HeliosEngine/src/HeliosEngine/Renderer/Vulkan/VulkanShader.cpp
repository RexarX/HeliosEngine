#include "Renderer/Vulkan/VulkanShader.h"
#include "Renderer/Vulkan/VulkanContext.h"

#include "Utils/Filesystem.h"

#include <shaderc/shaderc.hpp>

namespace Helios
{
	const VkShaderStageFlagBits translateStageToVulkan(const ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::None: break;
		case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		CORE_ASSERT(false, "Unknown shader stage!");
	}

	const shaderc_shader_kind translateStageToShaderc(const VkShaderStageFlagBits stage)
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

		CORE_ASSERT(false, "Unknown shader stage!");
	}

	const bool GLSLtoSPV(const VkShaderStageFlagBits shaderType, const std::string& glslShader,
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
			CORE_ERROR(module.GetErrorMessage());
			return false;
		}

		spvShader = { module.cbegin(), module.cend() };

		return true;
	}

	VulkanShader::VulkanShader(const std::initializer_list<ShaderInfo>& infos)
	{
		VulkanContext::Get().GetDeletionQueue().PushFunction([this]() { Unload(); });

		for (const auto& info : infos) {
			VulkanShaderInfo newInfo{};
			newInfo.path = info.path;
			newInfo.stage = translateStageToVulkan(info.stage);
			newInfo.shaderModule = VK_NULL_HANDLE;

			m_ShaderInfos.push_back(newInfo);
		}
	}

	void VulkanShader::Load()
	{
		if (m_Loaded) { return; }

		VkDevice device = VulkanContext::Get().GetDevice();

		for (auto& info : m_ShaderInfos) {
			if (!info.code.empty()) { continue; }

			std::filesystem::path path(info.path);
			std::string code = Utils::ReadFileToString(info.path);
			if (code.empty()) {
				CORE_ERROR("Failed to load shader code: {0}!", info.path);
				continue;
			}

			if (path.extension() != ".spv") {
				std::string name = path.filename().string();
				uint64_t index = name.find_last_of('.');
				if (index != std::string::npos) {
					name = name.substr(0, index + 1);
				}
				if (!GLSLtoSPV(info.stage, code, name, info.code)) {
					CORE_ERROR("Failed to compile shader: {0}!", info.path);
					continue;
				}
				else {
					info.code.resize(code.size() / sizeof(uint32_t));
					memcpy(info.code.data(), code.data(), code.size());
				}
			}

			VkShaderModuleCreateInfo shaderModuleInfo{};
			shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleInfo.flags = 0;
			shaderModuleInfo.codeSize = info.code.size() * sizeof(uint32_t);
			shaderModuleInfo.pCode = info.code.data();

			VkShaderModule shaderModule;
			vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule);
			CORE_ASSERT(shaderModule, "Failed to create shader module: {0}!", info.path);
		}
	}

	void VulkanShader::Unload()
	{
		if (!m_Loaded) { return; }

		VkDevice device = VulkanContext::Get().GetDevice();

		for (auto& info : m_ShaderInfos) {
			vkDestroyShaderModule(device, info.shaderModule, nullptr);
		}
	}
}