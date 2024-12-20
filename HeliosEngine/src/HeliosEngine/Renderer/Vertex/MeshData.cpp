#include "MeshData.h"

namespace Helios {
  MeshData::MeshData(const VertexLayout& layout)
    : m_Layout(layout) {
  }
  
  void MeshData::AddVertex(const Vertex& vertex) {
    if (m_Layout.IsEmpty()) {
      CORE_ASSERT(false, "Failed to add vertex: Vertex layout is empty!");
      return;
    }

    if (vertex.GetLayout() != m_Layout) {
      CORE_ASSERT(false, "Failed to add vertex: Vertex layout mismatch!");
      return;
    }

    uint64_t index = FindVertex(vertex);
    if (index == GetVertexCount()) {
      m_Vertices.insert(m_Vertices.end(), vertex.GetData().begin(), vertex.GetData().end());
      m_Indices.clear();
    }
  }

  void MeshData::RemoveVertex(uint64_t index) {
    if (m_Layout.IsEmpty()) {
      CORE_ASSERT(false, "Failed to remove vertex: Vertex layout is empty!");
      return;
    }

    if (index < GetVertexCount()) {
      uint64_t stride = m_Layout.GetStride();
      uint64_t offset = index * stride;
      m_Vertices.erase( m_Vertices.begin() + offset, m_Vertices.begin() + offset + stride);
      m_Indices.clear();
    }
  }

  void MeshData::CalculateIndices() {
    if (m_Layout.IsEmpty()) {
      CORE_ASSERT(false, "Failed to calculate indices: Vertex layout is empty!");
      return;
    }

    if (m_Vertices.empty()) {
      CORE_ASSERT(false, "Failed to calculate indices: Vertex data is empty!");
      return;
    }

    uint64_t vertexCount = GetVertexCount();
    uint64_t stride = m_Layout.GetStride();

    m_Indices.clear();
    m_Indices.reserve(vertexCount);

    struct VertexHasher {
      inline size_t operator()(const std::span<std::byte>& vertex) const {
        return std::hash<std::string_view>{}(reinterpret_cast<const char*>(vertex.data()));
      }
    };

    struct VertexEqual {
      inline bool operator()(const std::span<std::byte>& lhs, const std::span<std::byte>& rhs) const {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
      }
    };

    std::unordered_map<std::span<std::byte>, uint32_t, VertexHasher, VertexEqual> uniqueVertices;
    uniqueVertices.reserve(vertexCount / 2);

    std::vector<std::byte> newVertexData;
    newVertexData.reserve(m_Vertices.size() / 2);

    for (uint64_t i = 0; i < vertexCount; ++i) {
      std::span<std::byte> currentVertex(m_Vertices.begin() + i * stride, m_Vertices.begin() + (i + 1) * stride);
      auto [it, inserted] = uniqueVertices.try_emplace(
        currentVertex,
        static_cast<uint32_t>(uniqueVertices.size())
      );

      m_Indices.push_back(it->second);

      if (inserted) {
        newVertexData.insert(newVertexData.end(), currentVertex.begin(), currentVertex.end());
      }
    }

    m_Vertices = std::move(newVertexData);
  }

  void MeshData::SetVertices(std::span<const std::byte> vertices) {
    if (vertices.empty()) {
      CORE_ASSERT(false, "Failed to set vertices: vertices is empty!");
      return;
    }

    m_Vertices.assign(vertices.begin(), vertices.end());
    m_Indices.clear();
  }

    void MeshData::SetIndices(std::span<const uint32_t> indices) {
    if (indices.empty()) {
      CORE_ASSERT(false, "Failed to set indices: indices is empty!");
      return;
    }
    
    m_Indices.assign(indices.begin(), indices.end());
  }

  uint64_t MeshData::FindVertex(const Vertex& vertex) const {
    uint64_t vertexCount = GetVertexCount();
    const std::byte* newVertexData = vertex.GetData().data();
    
    for (uint64_t i = 0; i < vertexCount; ++i) {
      if (CompareVertices(GetVertexAt(i), newVertexData)) {
        return i;
      }
    }
    return vertexCount;
  }
}