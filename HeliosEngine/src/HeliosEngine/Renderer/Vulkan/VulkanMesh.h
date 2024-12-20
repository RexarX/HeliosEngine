#pragma once

#include "Renderer/Mesh.h"

#include "VulkanUtils.h"

namespace Helios {
  class VulkanMesh final : public Mesh {
  public:
    VulkanMesh(Type type, const MeshData& meshData);
    VulkanMesh(Type type, MeshData&& meshData);
               
    virtual ~VulkanMesh();
    
    void Load() override;
    void Unload() override;

    void SetMeshData(const MeshData& meshData) override;

    inline Type GetType() const override { return m_Type; }

    inline bool IsLoaded() const override { return m_Loaded; }

    inline const std::vector<std::byte>& GetVertices() const override { return m_MeshData.GetVertices(); }
    inline const std::vector<uint32_t>& GetIndices() const override { return m_MeshData.GetIndices(); }

    inline uint32_t GetVertexCount() const override { return m_MeshData.GetVertexCount(); }
    inline uint32_t GetIndexCount() const override { return m_MeshData.GetIndexCount(); }

    inline uint64_t GetVertexSize() const override { return m_MeshData.GetVertexSize(); }
    inline uint64_t GetIndexSize() const override { return m_MeshData.GetIndexSize(); }

    inline const VertexLayout& GetVertexLayout() const override { return m_MeshData.GetLayout(); }

    inline const AllocatedBuffer& GetVertexBuffer() const { return m_VertexBuffer; }
    inline const AllocatedBuffer& GetIndexBuffer() const { return m_IndexBuffer; }

  private:
    void SetVertexData(const MeshData& meshData);
    void SetIndexData(const MeshData& meshData);

    void LoadVertexData();
    void UnloadVertexData();

    void LoadIndexData();
    void UnloadIndexData();

    void CreateStaticVertexBuffer();
    void CreateDynamicVertexBuffer();

    void CreateStaticIndexBuffer();
    void CreateDynamicIndexBuffer();

  private:
    bool m_Loaded = false;

    Type m_Type;

    MeshData m_MeshData;

    uint64_t m_AllocatedVertexSize = 0;
    uint64_t m_AllocatedIndexSize = 0;

    AllocatedBuffer m_VertexBuffer;
    AllocatedBuffer m_IndexBuffer;
  };
}