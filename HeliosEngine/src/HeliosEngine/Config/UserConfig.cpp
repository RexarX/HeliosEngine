#include "UserConfig.h"

namespace Helios {
  void UserConfig::Serialize(toml::table& output) const {
    const char* api = m_API == RendererAPI::API::Vulkan ? "Vulkan" :
                      m_API == RendererAPI::API::OpenGL ? "OpenGL" :
                      "Vulkan";

    const char* windowMode = m_WindowMode == Window::Mode::Windowed ? "Windowed" :
                             m_WindowMode == Window::Mode::Borderless ? "Borderless" :
                             m_WindowMode == Window::Mode::Fullscreen ? "Fullscreen" :
                             "Borderless";

    output = toml::table{
      { "Renderer", toml::table{
          { "api", api }, { "framerate_limit", m_FramerateLimit }, { "vsync", m_VSync }
        }
      },
      { "Window", toml::table{
          { "mode", windowMode },
          { "resolution", toml::array(m_WindowResolution.first, m_WindowResolution.second) },
          { "size", toml::array(m_WindowSize.first, m_WindowSize.second) },
          { "refresh_rate", m_WindowRefreshRate }
        }
      }
    };
  }

  void UserConfig::Deserialize(const toml::table& input) {
    if (auto renderer = input["Renderer"]) {
      std::string api = renderer["api"].value_or("Vulkan");
      m_API = api == "Vulkan" ? RendererAPI::API::Vulkan :
              api == "OpenGL" ? RendererAPI::API::OpenGL :
              RendererAPI::API::Vulkan;

      int framerateLimit = renderer["framerate_limit"].value_or(0);
      if (framerateLimit < 0) { m_FramerateLimit = 0; } 
      else { m_FramerateLimit = static_cast<uint32_t>(framerateLimit); }

      m_VSync = renderer["vsync"].value_or(true);
    } else {
      CORE_WARN("No Renderer config found!");
      CORE_WARN("Using default Renderer configuration!");

      m_API = RendererAPI::API::Vulkan;
      m_FramerateLimit = 0;
      m_VSync = true;
    }

    if (auto window = input["Window"]) {
      std::string windowMode = window["mode"].value_or("Borderless");
      m_WindowMode = windowMode == "Windowed" ? Window::Mode::Windowed :
                     windowMode == "Borderless" ? Window::Mode::Borderless :
                     windowMode == "Fullscreen" ? Window::Mode::Fullscreen :
                     Window::Mode::Borderless;

      const toml::array& windowSize = *window["size"].as_array();
      switch (windowSize.size()) {
        case 0: {
          m_WindowSize.first = 0;
          m_WindowSize.second = 0;
          break;
        }
        
        case 1: {
          int width = windowSize[0].value_or(0);
          m_WindowSize.first = width < 0 ? 0 : static_cast<uint32_t>(width);
          m_WindowSize.second = 0;
          break;
        }

        default: {
          int width = windowSize[0].value_or(0);
          int height = windowSize[1].value_or(0);
          m_WindowSize.first = width < 0 ? 0 : static_cast<uint32_t>(width);
          m_WindowSize.second = height < 0 ? 0 : static_cast<uint32_t>(height);
          break;
        }
      }

      const toml::array& windowResolution = *window["resolution"].as_array();
      switch (windowResolution.size()) {
        case 0: {
          m_WindowResolution.first = 0;
          m_WindowResolution.second = 0;
          break;
        }

        case 1: {
          int resX = windowResolution[0].value_or(0);
          m_WindowResolution.first = resX < 0 ? 0 : static_cast<uint32_t>(resX);
          m_WindowResolution.second = 0;
          break;
        }

        default: {
          int resX = windowResolution[0].value_or(0);
          int resY = windowResolution[1].value_or(0);
          m_WindowResolution.first = resX < 0 ? 0 : static_cast<uint32_t>(resX);
          m_WindowResolution.second = resY < 0 ? 0 : static_cast<uint32_t>(resY);
          break;
        }
      }
      m_WindowResolution.first = static_cast<uint32_t>(window["resolution_x"].value_or(0));
      m_WindowResolution.second = static_cast<uint32_t>(window["resolution_y"].value_or(0));

      int refreshRate = window["refresh_rate"].value_or(0);
      if (refreshRate < 0) { m_WindowRefreshRate = 0; }
      else { m_WindowRefreshRate = static_cast<uint32_t>(refreshRate); }
    } else {
      CORE_WARN("No Window config found!");
      CORE_WARN("Using default Window configuration!");

      m_WindowMode = Window::Mode::Borderless;
      m_WindowSize = { 0, 0 };
      m_WindowResolution = { 0, 0 };
      m_WindowRefreshRate = 0;
    }
  }

  void UserConfig::LoadDefaults() {
    m_API = RendererAPI::API::Vulkan;
    m_FramerateLimit = 0;
    m_VSync = true;

    m_WindowMode = Window::Mode::Borderless;
    m_WindowSize = { 0, 0 };
    m_WindowResolution = { 0, 0 };
    m_WindowRefreshRate = 0;
  }

  void UserConfig::LoadFromWindow(const Window& window) {
    SetValue(m_VSync, window.IsVSync());
    SetValue(m_WindowMode, window.GetMode());
    SetValue(m_WindowSize, window.GetSize());
    SetValue(m_WindowResolution, window.GetResolution());
    SetValue(m_WindowRefreshRate, window.GetRefreshRate());
  }
}