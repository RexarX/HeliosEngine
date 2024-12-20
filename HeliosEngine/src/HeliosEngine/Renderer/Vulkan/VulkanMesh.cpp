#include "VulkanMesh.h"
#include "VulkanContext.h"

namespace Helios {
  VulkanMesh::VulkanMesh(Type type, const MeshData& meshData)
  : m_Type(type), m_MeshData(meshData) {
  }

  VulkanMesh::VulkanMesh(Type type, MeshData&& meshData)
  : m_Type(type), m_MeshData(meshData) {
  }
  
  VulkanMesh::~VulkanMesh() {
    Unload();
  }

  void VulkanMesh::Load() {
    if (m_Loaded) { return; }

    if (m_MeshData.IsVerticesEmpty()) {
      CORE_ASSERT(false, "Failed to load mesh: No vertex data!");
      return;
    }

    if (m_MeshData.IsLayoutEmpty()) {
      CORE_ASSERT(false, "Failed to load mesh: No layout!");
      return;
    }

    LoadVertexData();
    LoadIndexData();

    m_Loaded = true;
  }

  void VulkanMesh::Unload() {
    if (!m_Loaded) { return; }

    UnloadVertexData();
    UnloadIndexData();

    m_Loaded = false;
  }

  void VulkanMesh::SetMeshData(const MeshData& meshData) {
    if (m_Type == Type::Static) {
      CORE_ASSERT(false, "Failed to set mesh data: Cannot modify static mesh!");
      return;
    }

    if (meshData.IsVerticesEmpty()) {
      CORE_ASSERT(false, "Failed to set mesh data: No vertices provided!");
      return;
    }

    SetVertexData(meshData);

    if (meshData.IsIndicesEmpty()) { UnloadIndexData(); }
    else { SetIndexData(meshData); }

    m_MeshData = meshData;
  }

  void VulkanMesh::SetVertexData(const MeshData& meshData) {
    VulkanContext& context = VulkanContext::Get();
    uint64_t newSize = meshData.GetVertexSize();
    uint64_t newVertexCount = meshData.GetVertexCount();
    const std::vector<std::byte>& newData = meshData.GetVertices();
    if (newVertexCount <= m_MeshData.GetVertexCount()) {
      context.ImmediateSubmit([this, newVertexCount](const VkCommandBuffer cmd) {
        VkBufferMemoryBarrier barrier{
          .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
          .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
          .buffer = m_VertexBuffer.buffer,
          .offset = 0,
          .size = newVertexCount
        };
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &barrier, 0, nullptr);
      });

      std::memcpy(m_VertexBuffer.info.pMappedData, newData.data(), newSize);
      std::memset(static_cast<std::byte*>(m_VertexBuffer.info.pMappedData) + newSize, 0,
                  m_MeshData.GetVertexSize() - newSize);
      m_MeshData.ResizeVertices(newSize);
      
      m_MeshData.SetVertices({ newData.data(), newData.size() });
    } else {
      VmaAllocator allocator = context.GetAllocator();

      m_AllocatedVertexSize = std::max(static_cast<uint64_t>(newSize * 1.25),
                                       static_cast<uint64_t>(m_VertexBuffer.info.size * 2));
      
      if (m_Loaded) {
        UnloadVertexData();
        
        auto vertexBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                         m_AllocatedVertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        if (!vertexBuffer) {
          CORE_ASSERT(false, "Failed to set vertex data: {}!", vertexBuffer.error());
          return;
        }

        m_VertexBuffer = std::move(*vertexBuffer);
        std::memcpy(m_VertexBuffer.info.pMappedData, newData.data(), newSize);
      }

      m_MeshData.ResizeVertices(newSize);
      m_MeshData.SetVertices({ newData.data(), newData.size() });

      m_MeshData.ClearIndices();
    }
  }

  void VulkanMesh::SetIndexData(const MeshData& meshData) {
    VulkanContext& context = VulkanContext::Get();
    uint64_t indexSize = meshData.GetIndexSize();
    const std::vector<uint32_t>& indices = meshData.GetIndices();
    if (indexSize <= m_MeshData.GetIndexSize()) {
      context.ImmediateSubmit([this, indexSize](const VkCommandBuffer cmd) {
        VkBufferMemoryBarrier barrier{
          .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_INDEX_READ_BIT,
          .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
          .buffer = m_IndexBuffer.buffer,
          .offset = 0,
          .size = indexSize
        };
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &barrier, 0, nullptr);
      });

      std::memcpy(m_IndexBuffer.info.pMappedData, indices.data(), indexSize);
      std::memset(static_cast<std::byte*>(m_IndexBuffer.info.pMappedData) + indexSize, 0,
                  (m_MeshData.GetIndexCount() - indices.size()) * sizeof(uint32_t));
    } else {
      VmaAllocator allocator = context.GetAllocator();

      m_AllocatedIndexSize = std::max(static_cast<uint64_t>(indexSize * 1.25),
                                      static_cast<uint64_t>(m_IndexBuffer.info.size * 2));
      
      if (m_Loaded) {
        UnloadIndexData();
        
        auto indexBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                        m_AllocatedIndexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        if (!indexBuffer) {
          CORE_ASSERT(false, "Failed to set index data: {}!", indexBuffer.error());
          return;
        }

        m_IndexBuffer = std::move(*indexBuffer);
        std::memcpy(m_IndexBuffer.info.pMappedData, indices.data(), indexSize);
      }
    }

    m_MeshData.SetIndices({ indices.data(), indices.size() });
  }

  void VulkanMesh::LoadVertexData() {
    switch (m_Type) {
      case Type::Static: CreateStaticVertexBuffer(); return;
      case Type::Dynamic: CreateDynamicVertexBuffer(); return;
      default: CORE_ASSERT(false, "Failed to load vertex data: Unknown mesh type!"); return;
    } 
  }

  void VulkanMesh::UnloadVertexData() {
    if (m_VertexBuffer.buffer != VK_NULL_HANDLE) {
      VmaAllocator allocator = VulkanContext::Get().GetAllocator();
      if (m_Type == Type::Dynamic) {
        vmaUnmapMemory(allocator, m_VertexBuffer.allocation);
      }
      m_VertexBuffer.Destroy(allocator);
    }
  }

  void VulkanMesh::LoadIndexData() {
    switch (m_Type) {
      case Type::Static: CreateStaticIndexBuffer(); return;
      case Type::Dynamic: CreateDynamicIndexBuffer(); return;
      default: CORE_ASSERT(false, "Failed to load index data: Unknown mesh type!"); return;
    }
  }

  void VulkanMesh::UnloadIndexData() {
    if (m_IndexBuffer.buffer != VK_NULL_HANDLE) {
      VmaAllocator allocator = VulkanContext::Get().GetAllocator();
      if (m_Type == Type::Dynamic) {
        vmaUnmapMemory(allocator, m_IndexBuffer.allocation);
      }
      m_IndexBuffer.Destroy(allocator);
    }
  }

  void VulkanMesh::CreateStaticVertexBuffer() {
    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    auto stagingBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                      m_MeshData.GetVertexSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    if (!stagingBuffer) {
      CORE_ASSERT(false, "Failed to create vertex staging buffer: {}!", stagingBuffer.error());
      return;
    }

    auto vertexBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_MeshData.GetVertexSize(),
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    if (!vertexBuffer) {
      CORE_ASSERT(false, "Failed to create vertex buffer: {}!", vertexBuffer.error());
      UnloadVertexData();
      return;
    }

    m_VertexBuffer = std::move(*vertexBuffer);
    std::memcpy(stagingBuffer->info.pMappedData, m_MeshData.GetVertices().data(), m_MeshData.GetVertexSize());

    context.ImmediateSubmit([this, &stagingBuffer](const VkCommandBuffer cmd) {
      VkBufferCopy vertexCopy{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = m_MeshData.GetVertexSize()
      };
      vkCmdCopyBuffer(cmd, stagingBuffer->buffer, m_VertexBuffer.buffer, 1, &vertexCopy);
    });

    vmaUnmapMemory(allocator, stagingBuffer->allocation);
    vmaDestroyBuffer(allocator, stagingBuffer->buffer, stagingBuffer->allocation);
  }

  void VulkanMesh::CreateDynamicVertexBuffer() {
    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    m_AllocatedVertexSize = m_MeshData.GetVertexSize() * 1.25;
    auto vertexBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                     m_AllocatedVertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    if (!vertexBuffer) {
      CORE_ASSERT(false, "Failed to create vertex buffer: {}!", vertexBuffer.error());
      return;
    }

    m_VertexBuffer = std::move(*vertexBuffer);
    std::memcpy(m_VertexBuffer.info.pMappedData, m_MeshData.GetVertices().data(), m_MeshData.GetVertexSize());
  }

  void VulkanMesh::CreateStaticIndexBuffer() {
    if (m_MeshData.IsIndicesEmpty()) { return; }

    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    m_AllocatedIndexSize = m_MeshData.GetIndexSize();

    auto stagingBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                      m_AllocatedIndexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    if (!stagingBuffer) {
      CORE_ASSERT(false, "Failed to create index staging buffer: {}!", stagingBuffer.error());
      return;
    }

    auto indexBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_AllocatedIndexSize,
                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    if (!indexBuffer) {
      CORE_ASSERT(false, "Failed to create index buffer: {}!", indexBuffer.error());
      UnloadIndexData();
      return;
    }

    m_IndexBuffer = std::move(*indexBuffer);
    std::memcpy(stagingBuffer->info.pMappedData, m_MeshData.GetIndices().data(), m_AllocatedIndexSize);

    context.ImmediateSubmit([this, &stagingBuffer](const VkCommandBuffer cmd) {
      VkBufferCopy indexCopy{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = m_AllocatedIndexSize
      };
      vkCmdCopyBuffer(cmd, stagingBuffer->buffer, m_IndexBuffer.buffer, 1, &indexCopy);
    });

    vmaUnmapMemory(allocator, stagingBuffer->allocation);
    vmaDestroyBuffer(allocator, stagingBuffer->buffer, stagingBuffer->allocation);
  }

  void VulkanMesh::CreateDynamicIndexBuffer() {
    if (m_MeshData.IsIndicesEmpty()) { return; }

    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    m_AllocatedIndexSize = m_MeshData.GetIndexSize() * 1.25;
    auto indexBuffer = CreateBuffer(allocator, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                    m_AllocatedIndexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    if (!indexBuffer) {
      CORE_ASSERT(false, "Failed to create index buffer: {}!", indexBuffer.error());
      return;
    }

    m_IndexBuffer = std::move(*indexBuffer);
    std::memcpy(m_IndexBuffer.info.pMappedData, m_MeshData.GetIndices().data(), m_MeshData.GetIndexSize());
  }
}