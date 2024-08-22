#pragma once

#include "Texture.h"

#include <glm/glm.hpp>

namespace Helios
{
  struct HELIOSENGINE_API Material
  {
    void Load()
    {
      if (albedo != nullptr) { albedo->Load(); }
      if (normalMap != nullptr) { normalMap->Load(); }
      if (specularMap != nullptr) { specularMap->Load(); }
      if (roughnessMap != nullptr) { roughnessMap->Load(); }
      if (metallicMap != nullptr) { metallicMap->Load(); }
      if (aoMap != nullptr) { aoMap->Load(); }
    }

    void Unload()
    {
      if (albedo != nullptr) { albedo->Unload(); }
      if (normalMap != nullptr) { normalMap->Unload(); }
      if (specularMap != nullptr) { specularMap->Unload(); }
      if (roughnessMap != nullptr) { roughnessMap->Unload(); }
      if (metallicMap != nullptr) { metallicMap->Unload(); }
      if (aoMap != nullptr) { aoMap->Unload(); }
    }

    std::string name;

    std::shared_ptr<Texture> albedo = nullptr;
    std::shared_ptr<Texture> normalMap = nullptr;
    std::shared_ptr<Texture> specularMap = nullptr;
    std::shared_ptr<Texture> roughnessMap = nullptr;
    std::shared_ptr<Texture> metallicMap = nullptr;
    std::shared_ptr<Texture> aoMap = nullptr;

    glm::vec4 color = glm::vec4(-1.0f);
    float specular = -1.0f;
    float roughness = -1.0f;
    float metallic = -1.0f;
  };
}