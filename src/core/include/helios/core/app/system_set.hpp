#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace helios::app {

/**
 * @brief Type alias for system set type IDs.
 * @details Used to uniquely identify system set types at runtime.
 */
using SystemSetId = size_t;

/**
 * @brief Trait to identify valid system set types.
 * @details A valid system set type must be an empty struct or class.
 * This ensures system sets are zero-cost type markers with no runtime overhead.
 * @tparam T Type to check
 */
template <typename T>
concept SystemSetTrait = std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Trait to identify system sets with custom names.
 * @details A system set with name trait must satisfy SystemSetTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 * @tparam T Type to check
 */
template <typename T>
concept SystemSetWithNameTrait = SystemSetTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Gets unique type ID for a system set type.
 * @details Uses CTTI to generate a unique hash for the system set type.
 * The ID is consistent across compilation units.
 * @tparam T System set type
 * @return Unique type ID for the system set
 */
template <SystemSetTrait T>
constexpr SystemSetId SystemSetIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of a system set type.
 * @details Returns the custom name if GetName() is provided,
 * otherwise returns the CTTI-generated type name.
 * @tparam T System set type
 * @return Name of the system set
 */
template <SystemSetTrait T>
constexpr std::string_view SystemSetNameOf() noexcept {
  if constexpr (SystemSetWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

}  // namespace helios::app