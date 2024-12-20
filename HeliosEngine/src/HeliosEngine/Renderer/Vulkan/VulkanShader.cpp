#include "VulkanShader.h"
#include "VulkanContext.h"

#include "Utils/Filesystem.h"

namespace Helios {
	VulkanShader::VulkanShader(const std::initializer_list<Info>& infos) {
		for (const Info& info : infos) {
			if (!std::filesystem::exists(info.path)) {
				CORE_ERROR("Shader not found: {}!", info.path);
				continue;
			}
			m_VulkanInfos.emplace_back(info.path, TranslateStageToVulkan(info.stage));
		}

		Load();
	}

	VulkanShader::~VulkanShader() {
		VkDevice device = VulkanContext::Get().GetDevice();

		for (VulkanInfo& info : m_VulkanInfos) {
			vkDestroyShaderModule(device, info.shaderModule, nullptr);
		}
	}

	void VulkanShader::Load() {
		VkDevice device = VulkanContext::Get().GetDevice();

		for (VulkanInfo& info : m_VulkanInfos) {
			std::vector<uint32_t> spvCode;
			auto result = Utils::readFileToString(info.path);
			if (!result) {
				CORE_ASSERT(false, "Failed to load shader '{}' code: {}!", info.path, result.error());
				continue;
			}
			std::string code(std::move(*result));
			std::string_view extension = Utils::getFileExtension(info.path);
			if (extension == ".vert" || Utils::getFileExtension(info.path) == ".frag") {
				std::string_view name(Utils::getFileName(info.path));
				auto conversionResult = GLSLtoSPV(info.stage, code, name);
				if (!conversionResult) {
					CORE_ASSERT(false, "Failed to load shader: Failed to compile shader '{}': {}!", info.path,
											conversionResult.error());
					continue;
				}
				spvCode = std::move(*conversionResult);
			} else if (Utils::getFileExtension(info.path) == ".spv") {
				spvCode.resize(code.size() / sizeof(uint32_t));
				std::memcpy(spvCode.data(), code.data(), code.size());
			}

			VkShaderModuleCreateInfo shaderModuleInfo{};
			shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleInfo.flags = 0;
			shaderModuleInfo.codeSize = spvCode.size() * sizeof(uint32_t);
			shaderModuleInfo.pCode = spvCode.data();

			VkResult moduleResult = vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &info.shaderModule);
			CORE_ASSERT(moduleResult == VK_SUCCESS, "Failed to load shader: Failed to create shader module '{}'!",
									info.path);
		}
	}

	VkShaderStageFlagBits VulkanShader::TranslateStageToVulkan(Stage stage) {
		switch (stage) {
			case Stage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case Stage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case Stage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
			default: CORE_ASSERT(false, "Failed to translate shader stage to Vulkan: Unknown shader stage!");
		}
	}

	shaderc_shader_kind VulkanShader::TranslateStageToShaderc(VkShaderStageFlagBits stage) {
		switch (stage) {
			case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_vertex_shader;
			case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
			case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_compute_shader;
			case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_geometry_shader;
			case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return shaderc_tess_control_shader;
			case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
			default: CORE_ASSERT(false, "Failed to translate shader stage to Shaderc: Unknown shader stage!");
		}
	}

	std::expected<std::vector<uint32_t>, std::string> VulkanShader::GLSLtoSPV(VkShaderStageFlagBits shaderType,
																																						const std::string& glslShader,
																																						std::string_view fileName) {
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
			glslShader, TranslateStageToShaderc(shaderType),
			fileName.data(),
			options
		);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			return std::unexpected(module.GetErrorMessage());
		}

		std::vector<uint32_t> spvShader = { module.cbegin(), module.cend() };
		return spvShader;
	}
}