#pragma once

#include "pch.h"

namespace Helios {
  class HELIOSENGINE_API Shader {
  public:
    enum class Stage {
      Vertex,
      Fragment,
      Compute
    };

    struct Info {
      Stage stage;
      std::string path;
    };

    virtual ~Shader() = default;

    static std::shared_ptr<Shader> Create(const std::initializer_list<Info>& shaderInfos);
  };
}