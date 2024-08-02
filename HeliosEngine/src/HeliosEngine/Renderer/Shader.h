#pragma once

#include "pch.h"

namespace Helios
{
  enum class ShaderStage
  {
    None = 0,
    Vertex = 1,
    Fragment = 2,
    Compute = 3
  };

  struct ShaderInfo
  {
    ShaderStage stage;
    std::string path;
  };

  class HELIOSENGINE_API Shader
  {
  public:
    virtual ~Shader() = default;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    static std::shared_ptr<Shader> Create(const std::initializer_list<ShaderInfo>& shaderInfos);
  };
}