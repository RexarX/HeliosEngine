#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>

namespace helios::app {

class App;

/**
 * @brief Base class for all modules.
 * @details Derived classes must implement:
 * - `Build(helios::app::App&)` for initialization
 * - `Destroy(helios::app::App&)` for cleanup (optional)
 * - `IsReady(const helios::app::App&)` to check if ready (optional, default: true)
 * - `Finish(helios::app::App&)` for finalization after all modules are ready (optional)
 * - `static constexpr std::string_view GetName() noexcept` (optional)
 */
class Module {
public:
  virtual ~Module() = default;

  /**
   * @brief Builds the module.
   * @details Called during application initialization to set up the module.
   * This is where you should register systems, resources, and events.
   * @param app Application for initialization
   */
  virtual void Build(App& app) = 0;

  /**
   * @brief Finishes adding this module to the App, once all modules are ready.
   * @details This can be useful for modules that depend on another module's asynchronous
   * setup, like the renderer. Called after all modules' `Build` methods have been called
   * and all modules return true from `IsReady`.
   * @param app The application instance
   */
  virtual void Finish(App& /*app*/) {}

  /**
   * @brief Destroys the module and cleans up resources.
   * @details Called during application shutdown.
   * @param app The application instance
   */
  virtual void Destroy(App& /*app*/) {}

  /**
   * @brief Checks if the module is ready for finalization.
   * @details This can be useful for modules that need something asynchronous to happen
   * before they can finish their setup, like the initialization of a renderer.
   * Once the module is ready, `Finish` will be called.
   * @param app The application instance (const)
   * @return True if the module is ready, false otherwise
   */
  [[nodiscard]] virtual bool IsReady(const App& /*app*/) const noexcept { return true; }
};

/**
 * @brief Concept to ensure a type is a valid Module.
 * @details A valid Module must derive from Module.
 */
template <typename T>
concept ModuleTrait = std::derived_from<T, Module>;

/**
 * @brief Concept for modules that can be default constructed.
 * @details Used by AddModule<T>() overload that creates the module internally.
 */
template <typename T>
concept DefaultConstructibleModuleTrait = ModuleTrait<T> && std::default_initializable<T>;

/**
 * @brief Concept to check if a Module provides a GetName() method.
 * @details A Module with name trait must satisfy ModuleTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 */
template <typename T>
concept ModuleWithNameTrait = ModuleTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

using ModuleTypeId = size_t;

template <typename T>
constexpr ModuleTypeId ModuleTypeIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of a module.
 * @details Returns provided name or type name as fallback.
 * @tparam T Module type
 * @return Module name
 */
template <ModuleTrait T>
[[nodiscard]] constexpr std::string_view ModuleNameOf() noexcept {
  if constexpr (ModuleWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

}  // namespace helios::app
