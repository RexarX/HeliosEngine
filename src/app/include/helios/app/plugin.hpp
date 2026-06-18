#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace helios::app {

class App;

/// @brief Base class for all plugins.
class Plugin {
public:
  virtual ~Plugin() = default;

  /**
   * @brief Builds the plugin.
   * @details Called during application initialization to set up the plugin.
   * This is where you should register systems, plugins, and events.
   * @param app Application for initialization
   */
  virtual void Build(App& app) = 0;

  /**
   * @brief Finishes adding this plugin to the App, once all plugins are ready.
   * @details This can be useful for plugins that depend on another plugin's
   * asynchronous setup, like the renderer. Called after all plugins' `Build`
   * methods have been called and all plugins return true from `IsReady`.
   * @param app The application instance
   */
  virtual void Finish(App& /*app*/) {}

  /**
   * @brief Destroys the plugin and cleans up plugins.
   * @details Called during application shutdown.
   * @param app The application instance
   */
  virtual void Destroy(App& /*app*/) {}

  /**
   * @brief Advances asynchronous setup while the app waits for readiness.
   * @details Called from `App::WaitForPluginsReady` between readiness checks.
   * Override to pump GPU/window/network init on the main thread or submit
   * follow-up work to the app executor.
   * @param app The application instance
   */
  virtual void Poll(App& /*app*/) {}

  /**
   * @brief Checks if the plugin is ready for finalization.
   * @details This can be useful for plugins that need something asynchronous to
   * happen before they can finish their setup, like the initialization of a
   * renderer. Once the plugin is ready, `Finish` will be called.
   * @param app The application instance (const)
   * @return True if the plugin is ready, false otherwise
   */
  [[nodiscard]] virtual bool IsReady(const App& /*app*/) const noexcept {
    return true;
  }
};

/// @brief Type index for plugins.
using PluginTypeIndex = utils::TypeIndex;

/// @brief Type id for plugins.
using PluginTypeId = utils::TypeId;

/**
 * @brief Concept to ensure a type is a valid Plugin.
 * @details A valid Plugin must derive from `Plugin` and must not be abstract
 * (i.e., it must implement all pure virtual methods).
 */
template <typename T>
concept PluginTrait =
    std::derived_from<T, Plugin> && !std::is_abstract_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for plugins that provide a name.
 * @details A plugin with name trait must satisfy `PluginTrait` and provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept PluginWithNameTrait = PluginTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Gets name for a plugin type.
 * @tparam T Plugin type
 * @return Name of the plugin
 */
template <PluginTrait T>
[[nodiscard]] constexpr std::string_view PluginNameOf() noexcept {
  if constexpr (PluginWithNameTrait<T>) {
    return T::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

/**
 * @brief Gets name for a plugin type.
 * @tparam T Plugin type
 * @param plugin Plugin instance
 * @return Name of the plugin
 */
template <PluginTrait T>
[[nodiscard]] constexpr std::string_view PluginNameOf(
    const T& /*plugin*/) noexcept {
  return PluginNameOf<std::remove_cvref_t<T>>();
}

}  // namespace helios::app
