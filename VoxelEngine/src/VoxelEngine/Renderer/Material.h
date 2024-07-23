#pragma once

#include "Renderer/Texture.h"

#include "pch.h"

namespace Engine
{
  struct Material
  {
    std::string name;

    std::shared_ptr<Texture> albedo = nullptr;
    std::shared_ptr<Texture> normal = nullptr;
    std::shared_ptr<Texture> roughness = nullptr;
    std::shared_ptr<Texture> metallic = nullptr;
    std::shared_ptr<Texture> ao = nullptr;
  };
}