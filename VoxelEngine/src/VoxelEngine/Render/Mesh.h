#pragma once

#include "vepch.h"

namespace VoxelEngine
{
  class Mesh
  {
  public:
    Mesh() = default;
    ~Mesh() = default;

    bool LoadObj(const std::string& path);

    inline std::vector<float>& GetVertices() { return m_Vertices; }
    inline std::vector<uint32_t>& GetIndices() { return m_Indices; }

  private:
    std::vector<float> m_Vertices;
    std::vector<uint32_t> m_Indices;
  };
};