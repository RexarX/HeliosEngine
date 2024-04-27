#pragma once

namespace VoxelEngine
{
  class Chunk
  {
  public:
    Chunk() = default;
    ~Chunk();

    float* GetVertices() const noexcept { return m_Vertices; }
    uint32_t* GetIndices() const noexcept { return m_Indices; }

    uint32_t GetIndicesCount() const noexcept { return m_IndexCount; }
    uint32_t GetVerticesCount() const noexcept { return m_VerticesCount; }

  private:
    float* m_Vertices = nullptr;
    uint32_t* m_Indices = nullptr;

    uint32_t m_VerticesCount = 0;
    uint32_t m_IndexCount = 0;
  };
}