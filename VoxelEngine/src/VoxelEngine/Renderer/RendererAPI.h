#pragma once

namespace Engine
{
  class RendererAPI
  {
  public:
    enum class API
    {
      None = 0,
      Vulkan = 1
    };

    static void SetAPI(const API api) { m_API = api; }
    static inline const API GetAPI() { return m_API; }

  private:
    static API m_API;
  };
}