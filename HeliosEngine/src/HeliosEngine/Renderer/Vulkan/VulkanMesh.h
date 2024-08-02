#pragma once

#include "Renderer/Mesh.h"

#include "Renderer/Vulkan/VulkanUtils.h"

namespace Helios
{
  class VulkanMesh : public Mesh
  {
  public:
    VulkanMesh(const MeshType type, const std::vector<std::byte>& vertices,
               const uint32_t vertexCount, const std::vector<uint32_t>& indices = {});

    VulkanMesh(const MeshType type, const uint32_t vertexCount, const uint32_t indexCount);

    virtual ~VulkanMesh() = default;

    void Load() override;
    void Unload() override;

    void SetData(const std::vector<std::byte>& vertices, const uint32_t vertexCount,
                 const std::vector<uint32_t>& indices = {}) override;

    void SetVertexLayout(const VertexLayout& layout) override { m_VertexLayout = layout; }

    inline const MeshType GetType() const override { return m_Type; }

    inline const bool IsLoaded() const override { return m_Loaded; }

    inline const std::vector<std::byte>& GetVertices() const override { return m_VertexData; }
    inline const std::vector<uint32_t>& GetIndices() const override { return m_Indices; }

    inline const uint32_t GetVertexCount() const override { return m_VertexCount; }
    inline const uint32_t GetIndexCount() const override { return m_Indices.size(); }

    inline const uint64_t GetVertexSize() const override { return m_VertexData.size(); }
    inline const uint64_t GetIndexSize() const override { return m_Indices.size() * sizeof(uint32_t); }

    inline const VertexLayout& GetVertexLayout() const override { return m_VertexLayout; }

    inline AllocatedBuffer& GetVertexBuffer() { return m_VertexBuffer; }
    inline AllocatedBuffer& GetIndexBuffer() { return m_IndexBuffer; }

  private:
    void LoadVertexData();
    void UnloadVertexData();

    void LoadIndexData();
    void UnloadIndexData();

    void CreateStaticVertexBuffer();
    void CreateDynamicVertexBuffer();

    void CreateStaticIndexBuffer();
    void CreateDynamicIndexBuffer();

    void Resize();

  private:
    MeshType m_Type = MeshType::None;

    bool m_Loaded = false;

    std::vector<std::byte> m_VertexData;
    uint32_t m_VertexCount = 0;

    std::vector<uint32_t> m_Indices;

    VertexLayout m_VertexLayout;

    AllocatedBuffer m_VertexBuffer;
    AllocatedBuffer m_IndexBuffer;
  };
}