#pragma once

#include "Config.h"

#include "Window.h"

namespace Helios {
  class HELIOSENGINE_API UserConfig final : public Config {
    CONFIG_CLASS(UserConfig)

  public:
    void Serialize(toml::table& output) const override;
    void Deserialize(const toml::table& input) override;

    void LoadDefaults() override;

    void SetRenderAPI(RendererAPI::API api) { SetValue(m_API, api); }
    void SetFramerateLimit(uint32_t limit) { SetValue(m_FramerateLimit, limit); }
    void SetVSync(bool enabled) { SetValue(m_VSync, enabled); }
    
    void SetWindowMode(Window::Mode mode) { SetValue(m_WindowMode, mode); }
    void SetWindowSize(uint32_t width, uint32_t height) { SetValue(m_WindowSize, { width, height }); }
    void SetWindowWidth(uint32_t width) { SetValue(m_WindowSize.first, width); }
    void SetWindowHeight(uint32_t height) { SetValue(m_WindowSize.second, height); }
    void SetWindowResolution(uint32_t resX, uint32_t resY) { SetValue(m_WindowResolution, { resX,  resY}); }
    void SetWindowResolutionX(uint32_t resolutionX) { SetValue(m_WindowResolution.first, resolutionX); }
    void SetWindowResolutionY(uint32_t resolutionY) { SetValue(m_WindowResolution.second, resolutionY); }
    void SetWindowRefreshRate(uint32_t refreshRate) { SetValue(m_WindowRefreshRate, refreshRate); }

    void LoadFromWindow(const Window& window);

    inline RendererAPI::API GetRenderAPI() const { return m_API; }
    inline uint32_t GetFramerateLimit() const { return m_FramerateLimit; }
    inline bool IsVSync() const { return m_VSync; }

    inline Window::Mode GetWindowMode() const { return m_WindowMode; }
    inline std::pair<uint32_t, uint32_t> GetWindowSize() const { return m_WindowSize; }
    inline std::pair<uint32_t, uint32_t> GetWindowResolution() const { return m_WindowResolution; }
    inline uint32_t GetWindowRefreshRate() const { return m_WindowRefreshRate; }

  private:
    RendererAPI::API m_API = RendererAPI::API::Vulkan;
    uint32_t m_FramerateLimit = 0;
    bool m_VSync = true;

    Window::Mode m_WindowMode = Window::Mode::Borderless;
    std::pair<uint32_t, uint32_t> m_WindowSize{ 0, 0 };
    std::pair<uint32_t, uint32_t> m_WindowResolution{ 0, 0 };
    uint32_t m_WindowRefreshRate = 0;
  };
}