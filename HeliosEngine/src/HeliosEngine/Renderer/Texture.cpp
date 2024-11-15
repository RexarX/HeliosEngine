#include "Texture.h"
#include "RendererAPI.h"

#include "Vulkan/VulkanTexture.h"

namespace Helios {
	std::shared_ptr<Texture> Texture::Create(std::string_view path, uint32_t mipLevel,
																					 uint32_t anisoLevel, ImageFormat format) {
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::None: {
				CORE_ASSERT_CRITICAL(false, "RendererAPI::None is not supported!");
				return nullptr;
			}
			case RendererAPI::API::Vulkan: {
				return std::make_shared<VulkanTexture>(path, mipLevel, anisoLevel, format);
			}
		}

		CORE_ASSERT_CRITICAL(false, "Unknown RendererAPI!");
		return nullptr;
	}
}