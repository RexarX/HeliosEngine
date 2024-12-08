#include "Texture.h"
#include "RendererAPI.h"

#include "Vulkan/VulkanTexture.h"

namespace Helios {
	std::shared_ptr<Texture> Texture::Create(Type type, std::string_view path, const Info& info) {
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::None: {
				CORE_ASSERT_CRITICAL(false, "RendererAPI::None is not supported!");
				return nullptr;
			}

			case RendererAPI::API::Vulkan: {
				return std::make_shared<VulkanTexture>(type, path, info);
			}

			default: CORE_ASSERT_CRITICAL(false, "Unknown RendererAPI!"); return nullptr;
		}
	}
}