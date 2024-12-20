#include "Mesh.h"
#include "RendererAPI.h"

#include "Vulkan/VulkanMesh.h"

namespace Helios {
  std::shared_ptr<Mesh> Mesh::Create(Type type, const MeshData& vertexData) {
		switch (RendererAPI::GetAPI()) {
      case RendererAPI::API::None: {
        CORE_ASSERT_CRITICAL(false, "Failed to create Mesh: RendererAPI::None is not supported!");
        return nullptr;
      }

      case RendererAPI::API::Vulkan: {
        return std::make_shared<VulkanMesh>(type, vertexData);
      }

      default: CORE_ASSERT_CRITICAL(false, "Failed to create Mesh: Unknown RendererAPI!"); return nullptr;
		}
  }

  std::shared_ptr<Mesh> Mesh::Create(Type type, MeshData&& vertexData) {
		switch (RendererAPI::GetAPI()) {
      case RendererAPI::API::None: {
        CORE_ASSERT_CRITICAL(false, "Failed to create Mesh: RendererAPI::None is not supported!");
        return nullptr;
      }

      case RendererAPI::API::Vulkan: {
        return std::make_shared<VulkanMesh>(type, std::forward<MeshData>(vertexData));
      }

      default: CORE_ASSERT_CRITICAL(false, "Failed to create Mesh: Unknown RendererAPI!"); return nullptr;
		}
  }
}