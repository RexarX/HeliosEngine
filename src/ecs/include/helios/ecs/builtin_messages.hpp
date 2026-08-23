#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>

namespace helios::ecs {

/// @brief Message sent when an entity is added.
class EntityAddedMsg {
public:
  static constexpr std::string_view kName = "EntityAddedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs entity added message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Destroyed entity
   */
  explicit constexpr EntityAddedMsg(Entity entity) noexcept : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr EntityAddedMsg(const EntityAddedMsg&) noexcept = default;
  constexpr EntityAddedMsg(EntityAddedMsg&&) noexcept = default;
  constexpr ~EntityAddedMsg() noexcept = default;

  constexpr EntityAddedMsg& operator=(const EntityAddedMsg&) noexcept = default;
  constexpr EntityAddedMsg& operator=(EntityAddedMsg&&) noexcept = default;

  /**
   * @brief Gets the entity that was added.
   * @return Entity that was added
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that was added
};

/// @brief Message sent when an entity is destroyed.
class EntityDestroyedMsg {
public:
  static constexpr std::string_view kName = "EntityDestroyedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs entity destroyed message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Destroyed entity
   */
  explicit constexpr EntityDestroyedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr EntityDestroyedMsg(const EntityDestroyedMsg&) noexcept = default;
  constexpr EntityDestroyedMsg(EntityDestroyedMsg&&) noexcept = default;
  constexpr ~EntityDestroyedMsg() noexcept = default;

  constexpr EntityDestroyedMsg& operator=(const EntityDestroyedMsg&) noexcept =
      default;
  constexpr EntityDestroyedMsg& operator=(EntityDestroyedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that was destroyed.
   * @return Entity that was destroyed
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that was destroyed
};

/// @brief Message sent when a component is added.
template <ComponentTrait T>
class ComponentAddedMsg {
public:
  static constexpr std::string_view kName = "ComponentAddedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs component added message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity that component was added to
   */
  explicit constexpr ComponentAddedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ComponentAddedMsg(const ComponentAddedMsg&) noexcept = default;
  constexpr ComponentAddedMsg(ComponentAddedMsg&&) noexcept = default;
  constexpr ~ComponentAddedMsg() noexcept = default;

  constexpr ComponentAddedMsg& operator=(const ComponentAddedMsg&) noexcept =
      default;
  constexpr ComponentAddedMsg& operator=(ComponentAddedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that the component was added to.
   * @return Entity that the component was added to
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that the component was added to
};

/// @brief Message sent when a component is removed.
template <ComponentTrait T>
class ComponentRemovedMsg {
public:
  static constexpr std::string_view kName = "ComponentRemovedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs component removed message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity that component was removed from
   */
  explicit constexpr ComponentRemovedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ComponentRemovedMsg(const ComponentRemovedMsg&) noexcept = default;
  constexpr ComponentRemovedMsg(ComponentRemovedMsg&&) noexcept = default;
  constexpr ~ComponentRemovedMsg() noexcept = default;

  constexpr ComponentRemovedMsg& operator=(
      const ComponentRemovedMsg&) noexcept = default;
  constexpr ComponentRemovedMsg& operator=(ComponentRemovedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that the component was removed from.
   * @return Entity that the component was removed from
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that the component was removed from
};

/// @brief Message sent when all components are cleared from an entity.
class ComponentsClearedMsg {
public:
  static constexpr std::string_view kName = "ComponentsClearedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs components cleared message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity that components were cleared from
   */
  explicit constexpr ComponentsClearedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ComponentsClearedMsg(const ComponentsClearedMsg&) noexcept =
      default;
  constexpr ComponentsClearedMsg(ComponentsClearedMsg&&) noexcept = default;
  constexpr ~ComponentsClearedMsg() noexcept = default;

  constexpr ComponentsClearedMsg& operator=(
      const ComponentsClearedMsg&) noexcept = default;
  constexpr ComponentsClearedMsg& operator=(ComponentsClearedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that the components were cleared from.
   * @return Entity that the components were cleared from
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that the components were cleared from
};

/// @brief Message sent when a resource is inserted.
template <ResourceTrait T>
struct ResourceInsertedMsg {
  static constexpr std::string_view kName = "ResourceInsertedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;
};

/// @brief Message sent when a resource is removed.
template <ResourceTrait T>
struct ResourceRemovedMsg {
  static constexpr std::string_view kName = "ResourceRemovedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;
};

/**
 * @brief Formats `EntityAddedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `EntityAddedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, EntityAddedMsg msg) {
  return std::format_to(out, "EntityAddedMsg{{entity={}}}", msg.GetEntity());
}

/**
 * @brief Formats `EntityAddedMsg` message as a string.
 * @param msg `EntityAddedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(EntityAddedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats `EntityAddedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `EntityAddedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(EntityAddedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `EntityAddedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `EntityAddedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, EntityAddedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `EntityDestroyedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `EntityDestroyedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, EntityDestroyedMsg msg) {
  return std::format_to(out, "EntityDestroyedMsg{{entity={}}}",
                        msg.GetEntity());
}

/**
 * @brief Formats `EntityDestroyedMsg` message as a string.
 * @param msg `EntityDestroyedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(EntityDestroyedMsg msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats `EntityDestroyedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `EntityDestroyedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(EntityDestroyedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `EntityDestroyedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `EntityDestroyedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, EntityDestroyedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ComponentAddedMsg` message using an output iterator.
 * @tparam T Component type
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ComponentAddedMsg` message
 * @return The output iterator after writing
 */
template <ComponentTrait T, typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ComponentAddedMsg<T> msg) {
  return std::format_to(out, "ComponentAddedMsg<{}>{{entity={}}}",
                        ComponentNameOf<T>(), msg.GetEntity());
}

/**
 * @brief Formats `ComponentAddedMsg` message as a string.
 * @tparam T Component type
 * @param msg `ComponentAddedMsg` message
 * @return Formatted settings string
 */
template <ComponentTrait T>
[[nodiscard]] inline std::string ToString(ComponentAddedMsg<T> msg) {
  std::string result;
  result.reserve(128);
  ToString(msg, std::back_inserter(result));
  return result;
}

/**
 * @brief Formats `ComponentAddedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @tparam T Component type
 * @param msg `ComponentAddedMsg` message
 * @return Formatted string
 */
template <ComponentTrait T>
[[nodiscard]] inline std::pmr::string TempToString(ComponentAddedMsg<T> msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `ComponentAddedMsg` message to an output stream.
 * @tparam T Component type
 * @param os Output stream
 * @param msg `ComponentAddedMsg` message
 * @return Reference to the output stream
 */
template <ComponentTrait T>
inline std::ostream& operator<<(std::ostream& os, ComponentAddedMsg<T> msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ComponentRemovedMsg` message using an output iterator.
 * @tparam T Component type
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ComponentRemovedMsg` message
 * @return The output iterator after writing
 */
template <ComponentTrait T, typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ComponentRemovedMsg<T> msg) {
  return std::format_to(out, "ComponentRemovedMsg<{}>{{entity={}}}",
                        ComponentNameOf<T>(), msg.GetEntity());
}

/**
 * @brief Formats `ComponentRemovedMsg` message as a string.
 * @tparam T Component type
 * @param msg `ComponentRemovedMsg` message
 * @return Formatted settings string
 */
template <ComponentTrait T>
[[nodiscard]] inline std::string ToString(ComponentRemovedMsg<T> msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats `ComponentRemovedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @tparam T Component type
 * @param msg `ComponentRemovedMsg` message
 * @return Formatted string
 */
template <ComponentTrait T>
[[nodiscard]] inline std::pmr::string TempToString(ComponentRemovedMsg<T> msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `ComponentRemovedMsg` message to an output stream.
 * @tparam T Component type
 * @param os Output stream
 * @param msg `ComponentRemovedMsg` message
 * @return Reference to the output stream
 */
template <ComponentTrait T>
inline std::ostream& operator<<(std::ostream& os, ComponentRemovedMsg<T> msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ComponentsClearedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ComponentsClearedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ComponentsClearedMsg msg) {
  return std::format_to(out, "ComponentsClearedMsg{{entity={}}}",
                        msg.GetEntity());
}

/**
 * @brief Formats `ComponentsClearedMsg` message as a string.
 * @param msg `ComponentsClearedMsg` message
 * @return Formatted settings string
 */
[[nodiscard]] inline std::string ToString(ComponentsClearedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats `ComponentsClearedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ComponentsClearedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ComponentsClearedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `ComponentsClearedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ComponentsClearedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ComponentsClearedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ResourceInsertedMsg` message using an output iterator.
 * @tparam T Resource type
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ResourceInsertedMsg` message
 * @return The output iterator after writing
 */
template <ResourceTrait T, typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ResourceInsertedMsg<T> /*msg*/) {
  return std::format_to(out, "ResourceInsertedMsg<{}>", ResourceNameOf<T>());
}

/**
 * @brief Formats `ResourceInsertedMsg` message as a string.
 * @tparam T Resource type
 * @param msg `ResourceInsertedMsg` message
 * @return Formatted settings string
 */
template <ResourceTrait T>
[[nodiscard]] inline std::string ToString(ResourceInsertedMsg<T> msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats `ResourceInsertedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @tparam T Resource type
 * @param msg `ResourceInsertedMsg` message
 * @return Formatted string
 */
template <ResourceTrait T>
[[nodiscard]] inline std::pmr::string TempToString(ResourceInsertedMsg<T> msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `ResourceInsertedMsg` message to an output stream.
 * @tparam T Resource type
 * @param os Output stream
 * @param msg `ResourceInsertedMsg` message
 * @return Reference to the output stream
 */
template <ResourceTrait T>
inline std::ostream& operator<<(std::ostream& os, ResourceInsertedMsg<T> msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ResourceRemovedMsg` message using an output iterator.
 * @tparam T Resource type
 * @tparam It Output iterator type
 * @param msg `ResourceRemovedMsg` message
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <ResourceTrait T, typename It>
  requires std::output_iterator<It, char>
inline It ToString(ResourceRemovedMsg<T> /*msg*/, It out) {
  return std::format_to(out, "ResourceRemovedMsg<{}>", ResourceNameOf<T>());
}

/**
 * @brief Formats `ResourceRemovedMsg` message as a string.
 * @tparam T Resource type
 * @param msg `ResourceRemovedMsg` message
 * @return Formatted settings string
 */
template <ResourceTrait T>
[[nodiscard]] inline std::string ToString(ResourceRemovedMsg<T> msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats `ResourceRemovedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @tparam T Resource type
 * @param msg `ResourceRemovedMsg` message
 * @return Formatted string
 */
template <ResourceTrait T>
[[nodiscard]] inline std::pmr::string TempToString(ResourceRemovedMsg<T> msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs `ResourceRemovedMsg` message to an output stream.
 * @tparam T Resource type
 * @param os Output stream
 * @param msg `ResourceRemovedMsg` message
 * @return Reference to the output stream
 */
template <ResourceTrait T>
inline std::ostream& operator<<(std::ostream& os, ResourceRemovedMsg<T> msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::ecs

namespace std {

template <>
struct formatter<helios::ecs::EntityAddedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::EntityAddedMsg& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::ecs::EntityDestroyedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::EntityDestroyedMsg& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

template <helios::ecs::ComponentTrait T>
struct formatter<helios::ecs::ComponentAddedMsg<T>> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::ComponentAddedMsg<T>& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

template <helios::ecs::ComponentTrait T>
struct formatter<helios::ecs::ComponentRemovedMsg<T>> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::ComponentRemovedMsg<T>& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::ecs::ComponentsClearedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::ComponentsClearedMsg& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

template <helios::ecs::ResourceTrait T>
struct formatter<helios::ecs::ResourceInsertedMsg<T>> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::ResourceInsertedMsg<T>& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

template <helios::ecs::ResourceTrait T>
struct formatter<helios::ecs::ResourceRemovedMsg<T>> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::ecs::ResourceRemovedMsg<T>& msg,
                     format_context& ctx) {
    return helios::ecs::ToString(ctx.out(), msg);
  }
};

}  // namespace std
