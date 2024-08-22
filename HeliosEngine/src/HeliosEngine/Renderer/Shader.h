#pragma once

#include "pch.h"

namespace Helios
{
  enum class ShaderStage
  {
    Vertex,
    Fragment,
    Compute
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

    static std::shared_ptr<Shader> Create(const std::initializer_list<ShaderInfo>& shaderInfos);
  };
}