#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace helios::ecs {

/// @brief Type index for messages.
using MessageTypeIndex = utils::TypeIndex;

/// @brief Policy for message clearing behavior.
enum class MessageClearPolicy : uint8_t {
  kAutomatic,  ///< Messages are automatically cleared after double buffer cycle
  kManual      ///< Messages persist until manually cleared
};

/**
 * @brief Concept for valid message types.
 * @details A message must be destructible, move or copy constructible, not
 * polymorphic, be an object, and have a `static constexpr bool kAsync`
 * variable set to false or not defined.
 */
template <typename T>
concept MessageTrait =
    std::destructible<T> &&
    (std::move_constructible<T> || std::copy_constructible<T>) &&
    std::is_object_v<std::remove_cvref_t<T>> &&
    !std::is_polymorphic_v<std::remove_cvref_t<T>> && requires {
      requires !requires { requires std::remove_cvref_t<T>::kAsync; };
    };

/**
 * @brief Concept for valid async message types.
 * @details An async message trait must be destructible, default initializable,
 * move constructible and be an object and have a `static constexpr bool kAsync`
 * variable set to true.
 */
template <typename T>
concept AsyncMessageTrait =
    std::destructible<T> && std::default_initializable<T> &&
    std::move_constructible<T> && std::is_object_v<std::remove_cvref_t<T>> &&
    requires { requires std::remove_cvref_t<T>::kAsync; };

/**
 * @brief Concept for any valid message type.
 * @details A message type must satisfy either `MessageTrait` or
 * `AsyncMessageTrait`.
 */
template <typename T>
concept AnyMessageTrait = MessageTrait<T> || AsyncMessageTrait<T>;

/**
 * @brief Concept for messages with custom names.
 * @details A message with name trait must satisfy `MessageTrait` and provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept MessageWithNameTrait = AnyMessageTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Concept for messages with custom clear policy.
 * @details A message with clear policy trait must satisfy `MessageTrait` and
 * provide:
 * - `static constexpr MessageClearPolicy kClearPolicy` variable
 */
template <typename T>
concept MessageWithClearPolicy = AnyMessageTrait<T> && requires {
  {
    std::remove_cvref_t<T>::kClearPolicy
  } -> std::convertible_to<MessageClearPolicy>;
};

/**
 * @brief Concept for consumable message types.
 * @details A consumable message trait must satisfy `MessageTrait` and have a
 * `static constexpr bool kConsumable` variable set to true.
 */
template <typename T>
concept ConsumableMessageTrait = MessageTrait<T> && requires {
  requires std::remove_cvref_t<T>::kConsumable;
};

/**
 * @brief Gets the name of an message.
 * @details Returns provided name or type name as fallback.
 * @tparam T Message type
 * @return Message name
 */
template <AnyMessageTrait T>
[[nodiscard]] constexpr std::string_view MessageNameOf() noexcept {
  if constexpr (MessageWithNameTrait<T>) {
    return T::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

/**
 * @brief Gets the name of an message.
 * @details Returns provided name or type name as fallback.
 * @tparam T Message type
 * @param instance Message instance
 * @return Message name
 */
template <AnyMessageTrait T>
[[nodiscard]] constexpr std::string_view MessageNameOf(
    const T& /*instance*/) noexcept {
  return MessageNameOf<std::remove_cvref_t<T>>();
}

/**
 * @brief Gets the clear policy of an message.
 * @details Returns provided policy or `MessageClearPolicy::kAutomatic` as
 * default.
 * @tparam T Message type
 * @return Message clear policy
 */
template <AnyMessageTrait T>
[[nodiscard]] consteval MessageClearPolicy MessageClearPolicyOf() noexcept {
  if constexpr (MessageWithClearPolicy<T>) {
    return T::kClearPolicy;
  } else {
    return MessageClearPolicy::kAutomatic;
  }
}

/**
 * @brief Gets the clear policy of an message.
 * @details Returns provided policy or `MessageClearPolicy::kAutomatic` as
 * default.
 * @tparam T Message type
 * @param instance Message instance
 * @return Message clear policy
 */
template <AnyMessageTrait T>
[[nodiscard]] consteval MessageClearPolicy MessageClearPolicyOf(
    const T& /*instance*/) noexcept {
  return MessageClearPolicyOf<std::remove_cvref_t<T>>();
}

}  // namespace helios::ecs
