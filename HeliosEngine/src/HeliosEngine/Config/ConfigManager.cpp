#include "ConfigManager.h"

#include <argparse/argparse.hpp>

namespace Helios {
  void ConfigManager::ParseCommandLineArgs(int argc, char** argv) {
    argparse::ArgumentParser parser;

    auto& rendererGroup = parser.add_mutually_exclusive_group();
    rendererGroup.add_argument("-vulkan")
      .help("Enable Vulkan rendering API.");

    rendererGroup.add_argument("-opengl")
      .help("Enable OpenGL rendering API.");

    parser.add_argument("-framerate", "-fps")
      .help("Limits fps.")
      .default_value(0)
      .scan<'i', int>();

    parser.add_argument("-vsync")
      .help("Enable VSync.");

    auto& windowModeGroup = parser.add_mutually_exclusive_group();
    windowModeGroup.add_argument("-windowed")
      .help("Windowed window mode.");

    windowModeGroup.add_argument("-borderless")
      .help("Borderless window mode.");

    windowModeGroup.add_argument("-fullscreen")
      .help("Fullscreen window mode.");

    parser.add_argument("-width", "-w")
      .help("Set window width.")
      .default_value(0)
      .scan<'i', int>();

    parser.add_argument("-height", "-h")
      .help("Set window height.")
      .default_value(0)
      .scan<'i', int>();

    parser.add_argument("-resX")
      .help("Set window resolution X.")
      .default_value(0)
      .scan<'i', int>();

    parser.add_argument("-resY")
      .help("Set window resolution Y.")
      .default_value(0)
      .scan<'i', int>();

    parser.add_argument("-refreshrate", "-refresh")
      .help("Set window refresh rate.")
      .default_value(0)
      .scan<'i', int>();

    try {
      parser.parse_args(argc, argv);
      auto& userSettings = GetConfig<UserConfig>();
      if (parser.is_used("-vulkan")) {
        userSettings.SetRenderAPI(RendererAPI::API::Vulkan);
      } else if (parser.is_used("-opengl")) {
        userSettings.SetRenderAPI(RendererAPI::API::OpenGL);
      }

      if (parser.is_used("-framerate")) {
        int framerate = parser.get<int>("-framerate");
        if (framerate > 0) {
          userSettings.SetFramerateLimit(static_cast<uint32_t>(framerate));
        }
      }

      if (parser.is_used("-vsync")) {
        userSettings.SetVSync(true);
      }

      if (parser.is_used("-windowed")) {
        userSettings.SetWindowMode(Window::Mode::Windowed);
      } else if (parser.is_used("-borderless")) {
        userSettings.SetWindowMode(Window::Mode::Borderless);
      } else if (parser.is_used("-fullscreen")) {
        userSettings.SetWindowMode(Window::Mode::Fullscreen);
      }

      if (parser.is_used("-width")) {
        int width = parser.get<int>("-width");
        if (width > 0) {
          userSettings.SetWindowWidth(static_cast<uint32_t>(width));
        }
      }

      if (parser.is_used("-height")) {
        int height = parser.get<int>("-height");
        if (height > 0) {
          userSettings.SetWindowHeight(static_cast<uint32_t>(height));
        }
      }

      if (parser.is_used("-resX")) {
        int resX = parser.get<int>("-resX");
        if (resX > 0) {
          userSettings.SetWindowResolutionX(static_cast<uint32_t>(resX));
        }
      }

      if (parser.is_used("-resY")) {
        int resY = parser.get<int>("-resY");
        if (resY > 0) {
          userSettings.SetWindowResolutionY(static_cast<uint32_t>(resY));
        }
      }
    }
    catch (const std::exception& e) {
      CORE_ERROR("Error parsing command-line arguments: {}!", e.what());
    }
  }

  void ConfigManager::WriteConfigToFile(const Config& config, const std::filesystem::path& configPath) {
    toml::table configFile;
    config.Serialize(configFile);
    std::ofstream configFileOut(configPath);
    configFileOut << configFile;
  }
}