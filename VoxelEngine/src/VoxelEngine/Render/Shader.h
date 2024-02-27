#pragma once

namespace VoxelEngine
{
  class Shader
  {
    Shader(const std::string& vertex, const std::string& fragment);
    ~Shader();

    void Bind() const;
    void UnBind() const;

    void SetBool(const std::string& name, const bool& value) const;
    void SetInt(const std::string& name, const int& value) const;
    void SetFloat(const std::string& name, const float& value) const;

  private:
    uint32_t m_RendererID;
  };
};