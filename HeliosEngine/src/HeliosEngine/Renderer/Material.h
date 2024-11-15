#pragma once

#include "pch.h"

#include <glm/glm.hpp>

namespace Helios {
  class Texture;

  class HELIOSENGINE_API Material {
  public:
    Material() = default;
    ~Material() = default;

    void Load();
    void Unload();

    void SetName(std::string_view name) { m_Name = name; }

    void SetAlbedo(const std::shared_ptr<Texture>& albedo) { m_Albedo = albedo; }
    void SetNormalMap(const std::shared_ptr<Texture>& normalMap) { m_NormalMap = normalMap; }
    void SetSpecularMap(const std::shared_ptr<Texture>& specularMap) { m_SpecularMap = specularMap; }
    void SetRoughnessMap(const std::shared_ptr<Texture>& roughnessMap) { m_RoughnessMap = roughnessMap; }
    void SetMetallicMap(const std::shared_ptr<Texture>& metallicMap) { m_MetallicMap = metallicMap; }
    void SetAoMap(const std::shared_ptr<Texture>& aoMap) { m_AoMap = aoMap; }

    void SetColor(const glm::vec4& color) { m_Color = color; }
    void SetSpecular(float specular) { m_Specular = specular; }
    void SetRoughness(float roughness) { m_Roughness = roughness; }
    void SetMetallic(float metallic) { m_Metallic = metallic; }

    inline const std::string& GetName() const { return m_Name; }

    inline const std::shared_ptr<Texture>& GetAlbedo() const { return m_Albedo; }
    inline const std::shared_ptr<Texture>& GetNormalMap() const { return m_NormalMap; }
    inline const std::shared_ptr<Texture>& GetSpecularMap() const { return m_SpecularMap; }
    inline const std::shared_ptr<Texture>& GetRoughnessMap() const { return m_RoughnessMap; }
    inline const std::shared_ptr<Texture>& GetMetallicMap() const { return m_MetallicMap; }
    inline const std::shared_ptr<Texture>& GetAoMap() const { return m_AoMap; }

    inline const glm::vec4& GetColor() const { return m_Color; }
    inline float GetSpecular() const { return m_Specular; }
    inline float GetRoughness() const { return m_Roughness; }
    inline float GetMetallic() const { return m_Metallic; }

  private:
    std::string m_Name = "Material";

    std::shared_ptr<Texture> m_Albedo = nullptr;
    std::shared_ptr<Texture> m_NormalMap = nullptr;
    std::shared_ptr<Texture> m_SpecularMap = nullptr;
    std::shared_ptr<Texture> m_RoughnessMap = nullptr;
    std::shared_ptr<Texture> m_MetallicMap = nullptr;
    std::shared_ptr<Texture> m_AoMap = nullptr;

    glm::vec4 m_Color = glm::vec4(-1.0f);
    float m_Specular = -1.0f;
    float m_Roughness = -1.0f;
    float m_Metallic = -1.0f;
  };
}