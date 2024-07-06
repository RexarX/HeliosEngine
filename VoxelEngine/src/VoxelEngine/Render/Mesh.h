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
    inline std::vector<std::string>& GetTextures() { return m_Textures; }

  private:
    std::vector<float> m_Vertices;
    std::vector<uint32_t> m_Indices;

    std::vector<std::string> m_Textures;
  };
};