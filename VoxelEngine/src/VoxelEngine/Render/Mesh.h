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
    inline const std::vector<float>& GetVertices() const { return m_Vertices; }

    inline std::vector<uint32_t>& GetIndices() { return m_Indices; }
    inline const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

    static std::shared_ptr<Mesh> Create();

  private:
    std::vector<float> m_Vertices;
    std::vector<uint32_t> m_Indices;
  };
};