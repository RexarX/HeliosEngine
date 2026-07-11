#pragma once

#include <helios/app/plugin.hpp>
#include <helios/assert.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/utils/common_traits.hpp>

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace helios::app {

class App;

/**
 * @brief Base class for static plugin groups.
 * @details Stores per-plugin default instances, configuration overrides, and
 * disabled markers. Derived groups register their default plugins through
 * `Add` (typically in their constructor). `Build(App&)` is provided by this
 * base class and adds every non-disabled plugin — the configured override if
 * present, otherwise the registered default — into the given `App`.
 *
 * @code
 * struct DefaultPlugins final : public PluginGroup {
 *   DefaultPlugins() : PluginGroup(RenderPlugin{}, AudioPlugin{}) {}
 * };
 * @endcode
 */
class PluginGroup {
public:
  PluginGroup(const PluginGroup&) = delete;
  PluginGroup(PluginGroup&&) noexcept = default;
  constexpr ~PluginGroup() = default;

  PluginGroup& operator=(const PluginGroup&) = delete;
  constexpr PluginGroup& operator=(PluginGroup&&) noexcept = default;

  /**
   * @brief Configures a plugin override for this group.
   * @note Not thread-safe.
   * @warning Triggers assertion if the plugin is not a part of this group, or
   * if it was disabled before.
   * @tparam T Plugin type
   * @param plugin Plugin instance to use instead of the group's default
   * @return Reference to plugin group for method chaining
   */
  template <PluginTrait T>
  constexpr auto Configure(this auto&& self, T&& plugin)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Disables a plugin in this group.
   * @details Removes any configured plugin override of the same type.
   * @note Not thread-safe.
   * @warning Triggers assertion if the plugin is not a part of this group.
   * @tparam T Plugin type
   * @return Reference to plugin group for method chaining
   */
  template <PluginTrait T>
  constexpr auto Disable(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds every non-disabled plugin in this group to the given app.
   * @details For each registered plugin, uses the configured override if one
   * exists, otherwise the registered default. Disabled plugins are skipped.
   * @note Implemented in the `application.hpp`.
   * @warning Triggers assertion if an enabled plugin was already transferred
   * by an earlier call to `Build`.
   * @param app Application to add the group's plugins into
   */
  void Build(App& app);

protected:
  PluginGroup() = default;

  /**
   * @brief Constructs a group with the given default plugins.
   * @details This is the preferred way for derived groups to declare their
   * default plugin membership.
   * @tparam Ts Plugin types
   * @param plugins Default plugin instances for the group
   */
  template <PluginTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  explicit constexpr PluginGroup(Ts&&... plugins) {
    (Add(std::forward<Ts>(plugins)), ...);
  }

  /**
   * @brief Registers a default plugin instance as part of this group.
   * @note Not thread-safe. Intended to be called by derived groups, typically
   * from their constructor.
   * @warning Triggers assertion if a plugin of the same type is already
   * registered in this group.
   * @tparam T Plugin type
   * @param plugin Default plugin instance for the group
   */
  template <PluginTrait T>
  constexpr void Add(T&& plugin);

private:
  struct PluginStorage {
    std::unique_ptr<Plugin> plugin;
    PluginTypeId id;
    bool disabled = false;

    template <PluginTrait T, typename... Args>
      requires std::constructible_from<T, Args...>
    [[nodiscard]] static constexpr PluginStorage From(Args&&... args) {
      return {
          .plugin = std::make_unique<T>(std::forward<Args>(args)...),
          .id = PluginTypeId::From<T>(),
      };
    }
  };

  container::MultiTypeMap<PluginStorage> plugins_;
};

/// @brief Concept to ensure a type is a valid, concrete plugin group.
template <typename T>
concept PluginGroupTrait =
    std::derived_from<std::remove_cvref_t<T>, PluginGroup> &&
    !std::same_as<std::remove_cvref_t<T>, PluginGroup> &&
    !std::is_abstract_v<std::remove_cvref_t<T>>;

template <PluginTrait T>
constexpr auto PluginGroup::Configure(this auto&& self, T&& plugin)
    -> decltype(std::forward<decltype(self)>(self)) {
  using PluginType = std::remove_cvref_t<T>;
  auto& group = static_cast<PluginGroup&>(self);

  auto* storage = group.plugins_.template TryGet<PluginType>();
  HELIOS_ASSERT(storage != nullptr, "Plugin '{}' is not part of this group!",
                PluginNameOf<PluginType>());

  if (storage == nullptr) [[unlikely]] {
    return std::forward<decltype(self)>(self);
  }

  HELIOS_ASSERT(!storage->disabled, "Cannot configure disabled plugin '{}'!",
                PluginNameOf<PluginType>());
  if (storage->disabled) [[unlikely]] {
    return std::forward<decltype(self)>(self);
  }

  *storage = PluginStorage::From<PluginType>(std::forward<T>(plugin));

  return std::forward<decltype(self)>(self);
}

template <PluginTrait T>
constexpr auto PluginGroup::Disable(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  auto& group = static_cast<PluginGroup&>(self);

  auto* storage = group.plugins_.template TryGet<T>();
  HELIOS_ASSERT(storage != nullptr, "Plugin '{}' is not part of this group!",
                PluginNameOf<T>());

  if (storage == nullptr) [[unlikely]] {
    return std::forward<decltype(self)>(self);
  }

  storage->plugin.reset();
  storage->disabled = true;

  return std::forward<decltype(self)>(self);
}

template <PluginTrait T>
constexpr void PluginGroup::Add(T&& plugin) {
  using PluginType = std::remove_cvref_t<T>;

  auto new_storage = PluginStorage::From<PluginType>(std::forward<T>(plugin));
  [[maybe_unused]] const auto [it, inserted] =
      plugins_.template TryEmplace<PluginType>(std::move(new_storage));
  HELIOS_ASSERT(inserted, "Plugin '{}' is already registered in this group!",
                PluginNameOf<PluginType>());
}

}  // namespace helios::app
