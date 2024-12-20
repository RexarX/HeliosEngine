#pragma once

#include "Vertex.h"

namespace Helios {
  class MeshData {
  public:
    MeshData(const VertexLayout& layout);

    MeshData(const MeshData&) = default;
    MeshData(MeshData&&) noexcept = default;
    ~MeshData() = default;

    void AddVertex(const Vertex& vertex);

    void RemoveVertex(uint64_t index);

    void CalculateIndices();

    void ResizeVertices(uint64_t size) { m_Vertices.resize(size); }
    void ResizeIndices(uint64_t size) { m_Indices.resize(size); }

    void ClearVertices() { m_Vertices.clear(); }
    void ClearIndices() { m_Indices.clear(); }

    void SetVertices(std::span<const std::byte> vertices);
    void SetIndices(std::span<const uint32_t> indices);
    
    inline bool IsLayoutEmpty() const { return m_Layout.IsEmpty(); }
    inline bool IsVerticesEmpty() const { return m_Vertices.empty(); }
    inline bool IsIndicesEmpty() const { return m_Indices.empty(); }

    MeshData& operator=(const MeshData&) = default;
    MeshData& operator=(MeshData&&) noexcept = default;

    inline uint64_t GetVertexCount() const { return m_Vertices.size() / m_Layout.GetStride(); }
    inline uint64_t GetIndexCount() const { return m_Indices.size(); }

    inline uint64_t GetVertexSize() const { return m_Vertices.size(); }
    inline uint64_t GetIndexSize() const { return m_Indices.size() * sizeof(uint32_t); }

    inline const VertexLayout& GetLayout() const { return m_Layout; }

    inline const std::vector<std::byte>& GetVertices() const { return m_Vertices; }
    inline const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

  private:
    uint64_t FindVertex(const Vertex& vertex) const;

    inline bool CompareVertices(const std::byte* lhs, const std::byte* rhs) const {
      return std::memcmp(lhs, rhs, m_Layout.GetStride()) == 0;
    }

    inline const std::byte* GetVertexAt(uint64_t index) const {
      CORE_ASSERT(index < GetVertexCount(), "Failde to GetVertexAt: Vertex index out of bounds!");
      return m_Vertices.data() + (index * m_Layout.GetStride());
    }

  private:
    VertexLayout m_Layout;
    std::vector<std::byte> m_Vertices;
    std::vector<uint32_t> m_Indices;
  };
}