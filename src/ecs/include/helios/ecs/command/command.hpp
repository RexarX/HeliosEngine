#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <concepts>
#include <type_traits>
#endif

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;

/**
 * @brief Concept that defines the requirements for a command trait.
 * @details A command must be destructible, move or copy constructible,
 * be an object, not polymorphic, and must have an `Execute` with signature
 * `(helios::ecs::World&) -> void`.
 * @tparam T The type to check for the command trait
 */
template <typename T>
concept CommandTrait =
    std::destructible<T> &&
    (std::move_constructible<T> || std::copy_constructible<T>) &&
    std::is_object_v<std::remove_cvref_t<T>> &&
    !std::is_polymorphic_v<std::remove_cvref_t<T>> &&
    requires(T command, helios::ecs::World& world) {
      { command.Execute(world) } -> std::same_as<void>;
    };

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
