#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>

namespace helios::app {

class App;

// builder.AddSystem<InputSystem>(ExecutionStage::PreUpdate).After<RenderSystem>();
// builder.AddSystems<WindowSystem, ExampleSystem>(ExecutionStage::Render).After<RenderSystem>();
// builder.AddModule<PhysicsModule>(ExecutionStage::Update).After<InputSystem, ServerModule>();
// builder.AddModules<AnotherModule, ExampleModule>(ExecutionStage::PostUpdate).After<SomeModule, SomeSystem>();

/**
 * @brief Base class for all modules.
 * @details Derived classes must implement:
 * - `Build(helios::app::App&)` for initialization
 * - `Destroy(helios::app::App&)` for cleanup
 * - `static constexpr std::string_view GetName() noexcept` (optional)
 */
class Module {
public:
  virtual ~Module() = default;

  /**
   * @brief Builds the module.
   * @param app Application for initialization
   */
  virtual void Build(App& app) = 0;

  /**
   * @brief Destroys the module and cleans up resources.
   * @param app The application instance
   */
  virtual void Destroy(App& app) = 0;
};

/**
 * @brief Concept to ensure a type is a valid Module.
 * @details A valid Module must derive from Module and be default constructible.
 */
template <typename T>
concept ModuleTrait = std::derived_from<T, Module> && std::constructible_from<T>;

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
