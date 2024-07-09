#pragma once

#include "Texture.h"

namespace VoxelEngine
{
  class Material
  {
  public:
    Material() = default;
    Material(const std::string& name);
    ~Material() = default;

    void SetDiffuseMap(const std::shared_ptr<Texture>& texture) { m_DiffuseMap = texture; }

    inline const std::shared_ptr<Texture>& GetDiffuseMap() const { return m_DiffuseMap; }

  private:
    std::string m_Name;

    std::shared_ptr<Texture> m_DiffuseMap = nullptr;
  };
}