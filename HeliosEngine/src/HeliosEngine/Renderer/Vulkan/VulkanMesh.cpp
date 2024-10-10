#include "VulkanMesh.h"
#include "VulkanContext.h"

namespace Helios
{
  VulkanMesh::VulkanMesh(MeshType type, const std::vector<std::byte>& vertices,
                         uint32_t vertexCount, const std::vector<uint32_t>& indices)
    : m_Type(type), m_VertexData(vertices), m_VertexCount(vertexCount), m_Indices(indices)
  {
  }

  VulkanMesh::VulkanMesh(MeshType type, uint32_t vertexCount, uint32_t indexCount)
    : m_Type(type), m_VertexCount(vertexCount), m_Indices(indexCount)
  {
  }

  VulkanMesh::~VulkanMesh()
  {
    Unload();
  }

  void VulkanMesh::Load()
  {
    if (m_Loaded) { return; }

    LoadVertexData();
    LoadIndexData();

    m_Loaded = true;
  }

  void VulkanMesh::Unload()
  {
    if (!m_Loaded) { return; }

    UnloadVertexData();
    UnloadIndexData();

    m_Loaded = false;
  }

  void VulkanMesh::SetData(const std::vector<std::byte>& vertices, uint32_t vertexCount,
                           const std::vector<uint32_t>& indices)
  {
    if (m_Type == MeshType::Static) { CORE_ASSERT(false, "Cannot modify static mesh!"); return; }

    if (vertexCount <= m_VertexCount) {
      memcpy(m_VertexBuffer.info.pMappedData, vertices.data(), vertices.size());
      memset(static_cast<std::byte*>(m_VertexBuffer.info.pMappedData) + vertices.size(), 0,
             m_VertexData.size() - vertices.size());

      m_VertexData = vertices;
      m_VertexCount = vertexCount;
    } else {
      VulkanContext& context = VulkanContext::Get();
      VmaAllocator allocator = context.GetAllocator();

      uint64_t newSize = m_VertexBuffer.info.size * 2;

      UnloadVertexData();

      VkBufferCreateInfo bufferInfo{};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = static_cast<uint64_t>(vertices.size() * 1.25) > newSize ?
                        static_cast<uint64_t>(vertices.size() * 1.25) : newSize;

      bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo vmaallocInfo{};
      vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
      vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

      VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &m_VertexBuffer.buffer,
                                    &m_VertexBuffer.allocation, &m_VertexBuffer.info);

      CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");

      memcpy(m_VertexBuffer.info.pMappedData, vertices.data(), vertices.size());

      m_Loaded = true;
      m_VertexData = vertices;
      m_VertexCount = vertexCount;
    }

    if (!indices.empty()) {
      if (indices.size() <= m_Indices.size()) {
        memcpy(m_IndexBuffer.info.pMappedData, indices.data(), indices.size() * sizeof(uint32_t));
        memset(static_cast<std::byte*>(m_IndexBuffer.info.pMappedData) + indices.size() * sizeof(uint32_t),
               0, (m_Indices.size() - indices.size()) * sizeof(uint32_t));

        m_Indices = indices;
      } else {
        VulkanContext& context = VulkanContext::Get();
        VmaAllocator allocator = context.GetAllocator();

        uint64_t newSize = m_IndexBuffer.info.size * 2;

        UnloadIndexData();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = static_cast<uint64_t>(indices.size() * 1.25) > newSize ?
                          static_cast<uint64_t>(indices.size() * 1.25) : newSize;

        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo vmaallocInfo{};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &m_IndexBuffer.buffer,
                                          &m_IndexBuffer.allocation, &m_IndexBuffer.info);

        CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");

        memcpy(m_IndexBuffer.info.pMappedData, indices.data(), indices.size() * sizeof(uint32_t));

        m_Loaded = true;
        m_Indices = indices;
      }
    }
  }

  void VulkanMesh::LoadVertexData()
  {
    switch (m_Type)
    {
    case MeshType::Static: CreateStaticVertexBuffer(); return;
    case MeshType::Dynamic: CreateDynamicVertexBuffer(); return;
    }
  }

  void VulkanMesh::UnloadVertexData()
  {
    VmaAllocator allocator = VulkanContext::Get().GetAllocator();

    vmaUnmapMemory(allocator, m_VertexBuffer.allocation);
    vmaDestroyBuffer(allocator, m_VertexBuffer.buffer, m_VertexBuffer.allocation);
  }

  void VulkanMesh::LoadIndexData()
  {
    switch (m_Type)
    {
    case MeshType::Static: CreateStaticIndexBuffer(); return;
    case MeshType::Dynamic: CreateDynamicIndexBuffer(); return;
    }
  }

  void VulkanMesh::UnloadIndexData()
  {
    VmaAllocator allocator = VulkanContext::Get().GetAllocator();

    vmaUnmapMemory(allocator, m_IndexBuffer.allocation);
    vmaDestroyBuffer(allocator, m_IndexBuffer.buffer, m_IndexBuffer.allocation);
  }

  void VulkanMesh::CreateStaticVertexBuffer()
  {
    CORE_ASSERT(m_VertexLayout.GetElements().size() != 0, "Vertex Buffer has no layout!");

    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    AllocatedBuffer stagingVertexBuffer;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_VertexData.size();
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaallocInfo{};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &stagingVertexBuffer.buffer,
                                  &stagingVertexBuffer.allocation, &stagingVertexBuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging vertex buffer!");

    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
                             &m_VertexBuffer.buffer, &m_VertexBuffer.allocation, &m_VertexBuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");

    memcpy(stagingVertexBuffer.info.pMappedData, m_VertexData.data(), m_VertexData.size());

    context.ImmediateSubmit([&](const VkCommandBuffer cmd) {
      VkBufferCopy vertexCopy;
      vertexCopy.dstOffset = 0;
      vertexCopy.srcOffset = 0;
      vertexCopy.size = m_VertexData.size();

      vkCmdCopyBuffer(cmd, stagingVertexBuffer.buffer, m_VertexBuffer.buffer, 1, &vertexCopy);
    });

    vmaUnmapMemory(allocator, stagingVertexBuffer.allocation);
    vmaDestroyBuffer(allocator, stagingVertexBuffer.buffer, stagingVertexBuffer.allocation);
  }

  void VulkanMesh::CreateDynamicVertexBuffer()
  {
    CORE_ASSERT(m_VertexLayout.GetElements().size() != 0, "Vertex Buffer has no layout!");

    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = static_cast<uint64_t>(m_VertexData.size() * 1.5);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaallocInfo{};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &m_VertexBuffer.buffer,
                                  &m_VertexBuffer.allocation, &m_VertexBuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");

    memcpy(m_VertexBuffer.info.pMappedData, m_VertexData.data(), m_VertexData.size()); 
  }

  void VulkanMesh::CreateStaticIndexBuffer()
  {
    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    AllocatedBuffer stagingIndexBuffer;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_Indices.size() * sizeof(uint32_t);
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaallocInfo{};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &stagingIndexBuffer.buffer,
                                  &stagingIndexBuffer.allocation, &stagingIndexBuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create staging index buffer!");

    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &m_IndexBuffer.buffer,
                             &m_IndexBuffer.allocation, &m_IndexBuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");

    memcpy(stagingIndexBuffer.info.pMappedData, m_Indices.data(), m_Indices.size() * sizeof(uint32_t));

    context.ImmediateSubmit([&](const VkCommandBuffer cmd) {
      VkBufferCopy vertexCopy;
      vertexCopy.dstOffset = 0;
      vertexCopy.srcOffset = 0;
      vertexCopy.size = m_Indices.size() * sizeof(uint32_t);

      vkCmdCopyBuffer(cmd, stagingIndexBuffer.buffer, m_IndexBuffer.buffer, 1, &vertexCopy);
    });

    vmaUnmapMemory(allocator, stagingIndexBuffer.allocation);
    vmaDestroyBuffer(allocator, stagingIndexBuffer.buffer, stagingIndexBuffer.allocation);
  }

  void VulkanMesh::CreateDynamicIndexBuffer()
  {
    VulkanContext& context = VulkanContext::Get();
    VmaAllocator allocator = context.GetAllocator();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = static_cast<uint64_t>(m_Indices.size() * sizeof(uint32_t) * 1.25);
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaallocInfo{};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &m_IndexBuffer.buffer,
                                  &m_IndexBuffer.allocation, &m_IndexBuffer.info);

    CORE_ASSERT(result == VK_SUCCESS, "Failed to create index buffer!");

    memcpy(m_IndexBuffer.info.pMappedData, m_Indices.data(), m_Indices.size() * sizeof(uint32_t));
  }
}