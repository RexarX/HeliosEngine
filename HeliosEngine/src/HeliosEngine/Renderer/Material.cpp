#include "Material.h"
#include "Texture.h"

namespace Helios
{
  void Material::Load()
  {
    if (albedo != nullptr) { albedo->Load(); }
    if (normalMap != nullptr) { normalMap->Load(); }
    if (specularMap != nullptr) { specularMap->Load(); }
    if (roughnessMap != nullptr) { roughnessMap->Load(); }
    if (metallicMap != nullptr) { metallicMap->Load(); }
    if (aoMap != nullptr) { aoMap->Load(); }
  }

  void Material::Unload()
  {
    if (albedo != nullptr) { albedo->Unload(); }
    if (normalMap != nullptr) { normalMap->Unload(); }
    if (specularMap != nullptr) { specularMap->Unload(); }
    if (roughnessMap != nullptr) { roughnessMap->Unload(); }
    if (metallicMap != nullptr) { metallicMap->Unload(); }
    if (aoMap != nullptr) { aoMap->Unload(); }
  }
}