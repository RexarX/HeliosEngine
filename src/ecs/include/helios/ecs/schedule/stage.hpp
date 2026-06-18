#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

/// @brief Type index for stages.
using StageTypeIndex = utils::TypeIndex;

/// @brief Type id for stages.
using StageTypeId = utils::TypeId;

/**
 * @brief Concept for stages.
 * @details A stage must be an object and be empty.
 */
template <typename T>
concept StageTrait = std::is_object_v<std::remove_cvref_t<T>> &&
                     std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for stages that provide a name.
 * @details A stage with name trait must satisfy `StageTrait` and provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept StageWithNameTrait = StageTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Stage name for debugging and serialization.
 * @tparam T Stage type
 * @param instance Stage instance
 * @return Stage name
 */
template <StageTrait T>
[[nodiscard]] constexpr std::string_view StageNameOf(
    const T& /*instance*/ = {}) noexcept {
  if constexpr (StageWithNameTrait<T>) {
    return std::remove_cvref_t<T>::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

}  // namespace helios::ecs
