#pragma once

#include "Renderer/Mesh.h"

#include "VulkanUtils.h"

namespace Helios
{
  class VulkanMesh final : public Mesh
  {
  public:
    VulkanMesh(MeshType type, const std::vector<std::byte>& vertices,
               uint32_t vertexCount, const std::vector<uint32_t>& indices = {});

    VulkanMesh(MeshType type, uint32_t vertexCount, uint32_t indexCount);
    virtual ~VulkanMesh();

    void Load() override;
    void Unload() override;

    void SetData(const std::vector<std::byte>& vertices, uint32_t vertexCount,
                 const std::vector<uint32_t>& indices = {}) override;

    void SetVertexLayout(const VertexLayout& layout) override { m_VertexLayout = layout; }

    inline MeshType GetType() const override { return m_Type; }

    inline bool IsLoaded() const override { return m_Loaded; }

    inline const std::vector<std::byte>& GetVertices() const override { return m_VertexData; }
    inline const std::vector<uint32_t>& GetIndices() const override { return m_Indices; }

    inline uint32_t GetVertexCount() const override { return m_VertexCount; }
    inline uint32_t GetIndexCount() const override { return m_Indices.size(); }

    inline uint64_t GetVertexSize() const override { return m_VertexData.size(); }
    inline uint64_t GetIndexSize() const override { return m_Indices.size() * sizeof(uint32_t); }

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

  private:
    MeshType m_Type;

    bool m_Loaded = false;

    std::vector<std::byte> m_VertexData;
    uint32_t m_VertexCount = 0;

    std::vector<uint32_t> m_Indices;

    VertexLayout m_VertexLayout;

    AllocatedBuffer m_VertexBuffer;
    AllocatedBuffer m_IndexBuffer;
  };
}