#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace helios::ecs {

/**
 * @brief Type ID for events.
 */
using EventTypeId = size_t;

/**
 * @brief Policy for event clearing behavior.
 */
enum class EventClearPolicy : uint8_t {
  kAutomatic,  ///< Events are automatically cleared after double buffer cycle
  kManual      ///< Events persist until manually cleared via ManualClear()
};

/**
 * @brief Concept for valid event types.
 * @details A valid event must be:
 * - Move constructible
 * - Move assignable
 * - Trivially copyable (required for safe storage in byte buffers)
 *
 * @note Events are stored as raw bytes using memcpy, so they must be trivially copyable.
 * This means events cannot contain:
 * - std::string, std::vector, or other types with dynamic allocation
 * - Virtual functions
 * - User-defined copy/move constructors/assignment operators
 *
 * Use fixed-size arrays (e.g., char[N]) instead of dynamic containers.
 */
template <typename T>
concept EventTrait = std::move_constructible<T> && std::is_move_assignable_v<T> && std::is_trivially_copyable_v<T>;

/**
 * @brief Concept for events with custom names.
 * @details An event with name trait must satisfy EventTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 */
template <typename T>
concept EventWithNameTrait = EventTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Concept for events with custom clear policy.
 * @details An event with clear policy trait must satisfy EventTrait and provide:
 * - `static constexpr EventClearPolicy GetClearPolicy() noexcept`
 */
template <typename T>
concept EventWithClearPolicy = EventTrait<T> && requires {
  { T::GetClearPolicy() } -> std::same_as<EventClearPolicy>;
};

/**
 * @brief Gets type ID for an event type.
 * @tparam T Event type
 * @return Unique type ID for the event
 */
template <EventTrait T>
[[nodiscard]] constexpr EventTypeId EventTypeIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of an event.
 * @details Returns provided name or type name as fallback.
 * @tparam T Event type
 * @return Event name
 */
template <EventTrait T>
[[nodiscard]] constexpr std::string_view EventNameOf() noexcept {
  if constexpr (EventWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

/**
 * @brief Gets the clear policy of an event.
 * @details Returns provided policy or kAutomatic as default.
 * @tparam T Event type
 * @return Event clear policy
 */
template <EventTrait T>
[[nodiscard]] constexpr EventClearPolicy EventClearPolicyOf() noexcept {
  if constexpr (EventWithClearPolicy<T>) {
    return T::GetClearPolicy();
  } else {
    return EventClearPolicy::kAutomatic;
  }
}

}  // namespace helios::ecs
