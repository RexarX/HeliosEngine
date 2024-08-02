#pragma once

#include "Renderer/Texture.h"

namespace Helios
{
  struct HELIOSENGINE_API Material
  {
    void Load()
    {
      if (albedo != nullptr) { albedo->Load(); }
      if (normal != nullptr) { normal->Load(); }
      if (roughness != nullptr) { roughness->Load(); }
      if (metallic != nullptr) { metallic->Load(); }
      if (ao != nullptr) { ao->Load(); }
    }

    void Unload()
    {
      if (albedo != nullptr) { albedo->Unload(); }
      if (normal != nullptr) { normal->Unload(); }
      if (roughness != nullptr) { roughness->Unload(); }
      if (metallic != nullptr) { metallic->Unload(); }
      if (ao != nullptr) { ao->Unload(); }
    }

    std::string name;

    std::shared_ptr<Texture> albedo = nullptr;
    std::shared_ptr<Texture> normal = nullptr;
    std::shared_ptr<Texture> roughness = nullptr;
    std::shared_ptr<Texture> metallic = nullptr;
    std::shared_ptr<Texture> ao = nullptr;
  };
}