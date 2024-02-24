#pragma once


namespace VoxelEngine
{
  class Shader
  {
    Shader(const std::string& vertex, const std::string& fragment);
    ~Shader();

    void Bind() const;
    void UnBind() const;

  private:
    uint32_t m_RendererID;
  };
};