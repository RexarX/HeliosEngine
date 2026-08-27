#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/memory/arena_allocator.hpp>

#include <functional>
#endif
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/param.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class AccessPolicy;
class AccessPolicyBuilder;

/**
 * @brief System-local arena allocator resource.
 * @details Inserted automatically into each system's `SystemLocalData`.
 * Request via `Local<LocalArena>` or `Local<const LocalArena>`.
 * Memory is reclaimed when the schedule applies deferred work or starts a new
 * run.
 */
struct LocalArena {
  /**
   * @brief Constructs a local arena referencing the given allocator.
   * @param allocator Per-system arena allocator
   */
  explicit LocalArena(mem::ArenaAllocator& allocator) noexcept
      : arena(allocator) {}

  LocalArena(const LocalArena&) noexcept = default;
  LocalArena(LocalArena&&) noexcept = default;
  ~LocalArena() noexcept = default;

  LocalArena& operator=(const LocalArena&) noexcept = default;
  LocalArena& operator=(LocalArena&&) noexcept = default;

  [[nodiscard]] mem::ArenaAllocator& operator*() const noexcept {
    return arena.get();
  }

  [[nodiscard]] mem::ArenaAllocator* operator->() const noexcept {
    return &arena.get();
  }

  /**
   * @brief Gets a pointer to the underlying arena allocator.
   * @return Pointer to the arena allocator
   */
  [[nodiscard]] mem::ArenaAllocator* GetPtr() const noexcept {
    return &arena.get();
  }

  std::reference_wrapper<mem::ArenaAllocator> arena;
};

static_assert(ResourceTrait<LocalArena>);

// Special case for `LocalArena` to ensure it is always available in system
// local data. The `LocalArena` is constructed with the system's arena
// allocator, allowing for efficient temporary allocations during system
// execution.

template <>
struct SystemParamTraits<Local<const LocalArena>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> Local<const LocalArena> {
    data.resource_manager.TryEmplace<LocalArena>(data.allocator);
    return Local<const LocalArena>(data.resource_manager.Get<LocalArena>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <>
struct SystemParamTraits<Local<LocalArena>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept
      -> Local<LocalArena> {
    data.resource_manager.TryEmplace<LocalArena>(data.allocator);
    return Local<LocalArena>(data.resource_manager.Get<LocalArena>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
