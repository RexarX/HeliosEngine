#pragma once


namespace VoxelEngine
{
  class Shader
  {
    Shader(const std::string& vertex, const std::string& fragment);
    ~Shader();

    void Bind() const;
    void UnBind() const;

    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;

  private:
    uint32_t m_RendererID;
  };
};