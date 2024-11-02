#include "Material.h"
#include "Texture.h"

namespace Helios
{
  void Material::Load()
  {
    if (m_Albedo != nullptr) { m_Albedo->Load(); }
    if (m_NormalMap != nullptr) { m_NormalMap->Load(); }
    if (m_SpecularMap != nullptr) { m_SpecularMap->Load(); }
    if (m_RoughnessMap != nullptr) { m_RoughnessMap->Load(); }
    if (m_MetallicMap != nullptr) { m_MetallicMap->Load(); }
    if (m_AoMap != nullptr) { m_AoMap->Load(); }
  }

  void Material::Unload()
  {
    if (m_Albedo != nullptr) { m_Albedo->Unload(); }
    if (m_NormalMap != nullptr) { m_NormalMap->Unload(); }
    if (m_SpecularMap != nullptr) { m_SpecularMap->Unload(); }
    if (m_RoughnessMap != nullptr) { m_RoughnessMap->Unload(); }
    if (m_MetallicMap != nullptr) { m_MetallicMap->Unload(); }
    if (m_AoMap != nullptr) { m_AoMap->Unload(); }
  }
}