#pragma once

#include <helios/utils/hash.hpp>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

// C++26 reflection support detection.
// __cpp_impl_reflection guards the language feature (^^ operator),
// __cpp_lib_reflection guards the library (<meta> / std::meta::*).
// Both must be present and the library version must meet the minimum revision.
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection) && \
    __cpp_lib_reflection >= 202406L
#include <meta>
#define HELIOS_HAS_REFLECTION 1
#else
#define HELIOS_HAS_REFLECTION 0
#endif

namespace helios::utils {

namespace details {

#if HELIOS_HAS_REFLECTION

// C++26 reflection path
// std::meta::display_string_of / identifier_of return string_views backed by
// static compiler-managed storage, so no FixedString buffering is needed.

/**
 * @brief Retrieves the fully qualified type name of `T` via C++26 reflection.
 * @tparam T The type to retrieve the name for
 * @return The fully qualified type name
 */
template <typename T>
[[nodiscard]] consteval std::string_view GetFullTypeName() noexcept {
  return std::meta::display_string_of(^^T);
}

/**
 * @brief Retrieves the unqualified type name of `T` via C++26 reflection.
 * @tparam T The type to retrieve the name for
 * @return The unqualified type name
 */
template <typename T>
[[nodiscard]] consteval std::string_view GetUnqualifiedTypeName() noexcept {
  return std::meta::identifier_of(^^T);
}

#else  // fallback: compiler-signature scraping

/**
 * @brief A fixed-size string that can be constructed at compile time.
 * @tparam N The size of the string (excluding the null terminator)
 */
template <size_t N>
struct FixedString {
  explicit consteval FixedString(std::string_view sv) noexcept {
    std::copy_n(sv.begin(), N, data);
    data[N] = '\0';
  }

  consteval operator std::string_view() const noexcept { return {data, N}; }

  char data[N + 1];
};

/**
 * @brief Extracts the type name of `T` from the compiler-specific function
 * signature.
 * @tparam T The type to extract the name for
 * @return The extracted type name
 */
template <typename T>
[[nodiscard]] consteval std::string_view ExtractTypeName() noexcept {
#if defined(__clang__)
  constexpr auto prefix = std::string_view{"[T = "};
  constexpr auto suffix = std::string_view{"]"};
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
  constexpr auto prefix = std::string_view{"with T = "};
  constexpr auto suffix = std::string_view{"]"};
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
  constexpr auto prefix = std::string_view{"ExtractTypeName<"};
  constexpr auto suffix = std::string_view{">(void)"};
  constexpr auto function = std::string_view{__FUNCSIG__};
#else
#error Unsupported compiler
#endif

  constexpr auto start = function.find(prefix) + prefix.size();
  constexpr auto end = function.rfind(suffix);

  static_assert(start < end, "Compiler function signature parsing failed!");

#if defined(__GNUC__) && !defined(__clang__)
  // GCC may append alias expositions after the type parameter, e.g.:
  //   [with T = int; std::string_view = std::basic_string_view<char>]
  // We need to stop at the first ';' after 'start' if present.
  constexpr auto semicolon = function.find(';', start);
  constexpr auto actual_end =
      (semicolon != std::string_view::npos && semicolon < end) ? semicolon
                                                               : end;
  return function.substr(start, actual_end - start);
#else
  return function.substr(start, end - start);
#endif
}

/**
 * @brief Counts the number of qualifiers (namespaces and class scopes) in a
 * fully qualified type name.
 * @param full_name The fully qualified type name to analyze
 * @return The number of qualifiers in the type name
 */
[[nodiscard]] consteval size_t CountQualifiers(
    std::string_view full_name) noexcept {
  size_t count = 0;
  for (size_t i = 0; i < full_name.size(); ++i) {
    if (full_name[i] == ':' && i + 1 < full_name.size() &&
        full_name[i + 1] == ':') {
      ++count;
      ++i;
    }
  }
  return count;
}

/**
 * @brief Extracts the unqualified type name from a fully qualified type name.
 * @param full_name The fully qualified type name to extract from
 * @return The unqualified type name
 */
[[nodiscard]] constexpr std::string_view ExtractUnqualifiedName(
    std::string_view full_name) noexcept {
  size_t last_separator = 0;

  for (size_t i = 0; i < full_name.size(); ++i) {
    if (full_name[i] == ':' && i + 1 < full_name.size() &&
        full_name[i + 1] == ':') {
      last_separator = i + 2;
      ++i;
    }
  }

  if (last_separator < full_name.size()) {
    return full_name.substr(last_separator);
  }

  return full_name;
}

template <typename T>
inline constexpr auto kTypeNameStorage =
    FixedString<ExtractTypeName<T>().size()>(ExtractTypeName<T>());

/**
 * @brief Retrieves a null-terminated fully qualified type name for `T`.
 * @tparam T The type to retrieve the name for
 * @return Pointer to static null-terminated storage
 */
template <typename T>
[[nodiscard]] consteval const char* GetFullTypeNameCString() noexcept {
  return kTypeNameStorage<T>.data;
}

/**
 * @brief Retrieves the fully qualified type name of `T`.
 * @tparam T The type to retrieve the name for
 * @return The fully qualified type name
 */
template <typename T>
[[nodiscard]] constexpr std::string_view GetFullTypeName() noexcept {
  return ExtractTypeName<T>();
}

/**
 * @brief Retrieves the unqualified type name of `T`.
 * @tparam T The type to retrieve the name for
 * @return The unqualified type name
 */
template <typename T>
[[nodiscard]] constexpr std::string_view GetUnqualifiedTypeName() noexcept {
  return ExtractUnqualifiedName(GetFullTypeName<T>());
}

#endif  // HELIOS_HAS_REFLECTION

/**
 * @brief Computes a hash value for the type `T` based on its fully qualified
 * name.
 * @tparam T The type to compute the hash for
 * @return The computed hash value
 */
template <typename T>
[[nodiscard]] consteval size_t ComputeTypeHash() noexcept {
  return Fnv1aHash(GetFullTypeName<T>());
}

/**
 * @brief Detects whether `T` is a lambda closure type.
 * @details Prefers C++26 reflection when available
 * (`std::meta::is_class && !std::meta::has_identifier`). Falls back to
 * compiler-specific type-name heuristics otherwise.
 * @tparam T Type to inspect
 * @return `true` when `T` is a lambda closure type
 */
template <typename T>
[[nodiscard]] consteval bool IsLambdaType() noexcept {
#if HELIOS_HAS_REFLECTION
  return std::meta::is_class(^^T) && !std::meta::has_identifier(^^T);
#elif defined(_MSC_VER)
  constexpr auto name = GetUnqualifiedTypeName<T>();
  return name.find("<lambda_") != std::string_view::npos;
#elif defined(__clang__)
  constexpr auto name = GetUnqualifiedTypeName<T>();
  return name.find("(lambda at") != std::string_view::npos;
#elif defined(__GNUC__)
  constexpr auto name = GetUnqualifiedTypeName<T>();
  return name.find("<lambda(") != std::string_view::npos ||
         name.find("<lambda>") != std::string_view::npos;
#else
  return false;
#endif
}

/**
 * @brief Detects whether `T` is a regular functor type (not a lambda).
 * @details Returns `true` for named structs/classes that are not lambda
 * closure types.
 * @tparam T Type to inspect
 * @return `true` when `T` is an object type that is not a lambda
 */
template <typename T>
[[nodiscard]] consteval bool IsFunctorType() noexcept {
  return std::is_class_v<T> && !IsLambdaType<T>();
}

}  // namespace details

class TypeId;

class TypeIndex {
public:
  /// @brief Constructs empty `TypeIndex`
  constexpr TypeIndex() noexcept = default;

  /**
   * @brief Constructs a `TypeIndex` from a `TypeId`.
   * @param type_id The `TypeId` to construct from
   */
  explicit constexpr TypeIndex(const TypeId& type_id) noexcept;
  constexpr TypeIndex(const TypeIndex&) noexcept = default;
  constexpr TypeIndex(TypeIndex&&) noexcept = default;
  constexpr ~TypeIndex() noexcept = default;

  /**
   * @brief Constructs a `TypeIndex` from a type `T`.
   * @tparam T The type to construct the `TypeIndex` for
   * @return TypeIndex representing the type `T`
   */
  template <typename T>
  [[nodiscard]] static constexpr TypeIndex From() noexcept {
    return TypeIndex(details::ComputeTypeHash<T>());
  }

  /**
   * @brief Constructs a `TypeIndex` from an instance of type `T`.
   * @tparam T The type of the instance
   * @param instance The instance to construct the `TypeIndex` from (unused)
   * @return TypeIndex representing the type of the instance
   */
  template <typename T>
  [[nodiscard]] static constexpr TypeIndex From(
      const T& /*instance*/) noexcept {
    return From<std::remove_cvref_t<T>>();
  }

  /**
   * @brief Constructs a `TypeIndex` from a precomputed hash value.
   * @param hash Type hash value
   * @return TypeIndex with the given hash
   */
  [[nodiscard]] static constexpr TypeIndex FromHash(size_t hash) noexcept {
    return TypeIndex(hash);
  }

  constexpr TypeIndex& operator=(const TypeIndex&) noexcept = default;
  constexpr TypeIndex& operator=(TypeIndex&&) noexcept = default;

  /**
   * @brief Checks if this `TypeIndex` is empty (i.e., has a hash value of 0).
   * @return true if this `TypeIndex` is empty, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return hash_ == 0; }

  /**
   * @brief Retrieves the hash value associated with this `TypeIndex`.
   * @return Hash value or 0 if this `TypeIndex` is empty
   */
  [[nodiscard]] constexpr size_t Hash() const noexcept { return hash_; }

  explicit constexpr operator size_t() const noexcept { return Hash(); }

  [[nodiscard]] constexpr std::strong_ordering operator<=>(
      const TypeIndex& other) const noexcept = default;
  [[nodiscard]] constexpr bool operator==(
      const TypeIndex& other) const noexcept = default;

private:
  explicit constexpr TypeIndex(size_t hash) noexcept : hash_(hash) {}

  size_t hash_ = 0;
};

class TypeId {
public:
  /// @brief Constructs empty `TypeId`
  constexpr TypeId() noexcept = default;
  constexpr TypeId(const TypeId&) noexcept = default;
  constexpr TypeId(TypeId&&) noexcept = default;
  constexpr ~TypeId() noexcept = default;

  /**
   * @brief Constructs a `TypeId` from a type `T`.
   * @tparam T The type to construct the `TypeId` for
   * @return TypeId representing the type `T`
   */
  template <typename T>
  [[nodiscard]] static constexpr TypeId From() noexcept {
    return {TypeIndex::From<T>(), details::GetFullTypeName<T>()};
  }

  /**
   * @brief Constructs a `TypeId` from an instance of type `T`.
   * @tparam T The type of the instance
   * @param instance The instance to construct the `TypeId` from (unused)
   * @return TypeId representing the type of the instance
   */
  template <typename T>
  [[nodiscard]] static constexpr TypeId From(const T& /*instance*/) noexcept {
    return From<std::remove_cvref_t<T>>();
  }

  /**
   * @brief Constructs a `TypeId` from a precomputed hash and optional name.
   * @param hash Type hash (`TypeIndex::Hash()` for the source type)
   * @param qualified_name Optional qualified type name
   * @return TypeId with the given identity
   */
  [[nodiscard]] static constexpr TypeId FromExported(
      uint64_t hash, std::string_view qualified_name = {}) noexcept {
    return {TypeIndex::FromHash(static_cast<size_t>(hash)), qualified_name};
  }

  constexpr TypeId& operator=(const TypeId&) noexcept = default;
  constexpr TypeId& operator=(TypeId&&) noexcept = default;

  /**
   * @brief Checks if this `TypeId` is empty (i.e., has a hash value of 0).
   * @return true if this `TypeId` is empty, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return type_index_.Empty();
  }

  /**
   * @brief Retrieves the unqualified name of the type represented by this
   * `TypeId`.
   * @return Unqualified type name or empty string if this `TypeId` is empty
   */
  [[nodiscard]] constexpr std::string_view Name() const noexcept {
    return details::ExtractUnqualifiedName(full_name_);
  }

  /**
   * @brief Retrieves the fully qualified name of the type represented by this
   * `TypeId`.
   * @return Fully qualified type name or empty string if this `TypeId` is empty
   */
  [[nodiscard]] constexpr std::string_view QualifiedName() const noexcept {
    return full_name_;
  }

  /**
   * @brief Retrieves the type index associated with this `TypeId`.
   * @return TypeIndex for stored type
   */
  [[nodiscard]] constexpr TypeIndex Index() const noexcept {
    return type_index_;
  }

  [[nodiscard]] constexpr std::strong_ordering operator<=>(
      const TypeId& other) const noexcept {
    return type_index_ <=> other.type_index_;
  }

  [[nodiscard]] constexpr bool operator==(const TypeId& other) const noexcept {
    return type_index_ == other.type_index_;
  }

private:
  constexpr TypeId(TypeIndex type_index, std::string_view full_name) noexcept
      : type_index_(type_index), full_name_(full_name) {}

  TypeIndex type_index_;
  std::string_view full_name_;
};

constexpr TypeIndex::TypeIndex(const TypeId& type_id) noexcept
    : TypeIndex(type_id.Index()) {}

/**
 * @brief Compares a `TypeId` with a `TypeIndex` for equality.
 * @param lhs The `TypeId` to compare
 * @param rhs The `TypeIndex` to compare
 * @return `true` if the `TypeId` and `TypeIndex` refer to the same type,
 * `false` otherwise
 */
constexpr bool operator==(const TypeId& lhs, TypeIndex rhs) noexcept {
  return lhs.Index() == rhs;
}

/**
 * @brief Compares a `TypeId` with a `TypeIndex` for ordering.
 * @param lhs The `TypeId` to compare
 * @param rhs The `TypeIndex` to compare
 * @return `std::strong_ordering` result of the comparison
 */
constexpr std::strong_ordering operator<=>(const TypeId& lhs,
                                           TypeIndex rhs) noexcept {
  return lhs.Index() <=> rhs;
}

/**
 * @brief Compares a `TypeIndex` with a `TypeId` for equality.
 * @param lhs The `TypeIndex` to compare
 * @param rhs The `TypeId` to compare
 * @return `true` if the `TypeIndex` and `TypeId` refer to the same type,
 * `false` otherwise
 */
constexpr bool operator==(TypeIndex lhs, const TypeId& rhs) noexcept {
  return lhs == rhs.Index();
}

/**
 * @brief Compares a `TypeIndex` with a `TypeId` for ordering.
 * @param lhs The `TypeIndex` to compare
 * @param rhs The `TypeId` to compare
 * @return `std::strong_ordering` result of the comparison
 */
constexpr std::strong_ordering operator<=>(TypeIndex lhs,
                                           const TypeId& rhs) noexcept {
  return lhs <=> rhs.Index();
}

/**
 * @brief Retrieves the unqualified type name of `T`.
 * @tparam T The type to retrieve the name for
 * @return Unqualified type name
 */
template <typename T>
[[nodiscard]] constexpr std::string_view TypeNameOf() noexcept {
  return details::GetUnqualifiedTypeName<T>();
}

/**
 * @brief Retrieves the unqualified type name of the type of the given instance.
 * @tparam T The type of the instance
 * @param instance The instance to retrieve the type name for (unused)
 * @return Unqualified type name
 */
template <typename T>
[[nodiscard]] constexpr std::string_view TypeNameOf(
    const T& /*instance*/) noexcept {
  return details::GetUnqualifiedTypeName<std::remove_cvref_t<T>>();
}

/**
 * @brief Retrieves the fully qualified type name of `T`.
 * @tparam T The type to retrieve the name for
 * @return Fully qualified type name
 */
template <typename T>
[[nodiscard]] constexpr std::string_view QualifiedTypeNameOf() noexcept {
  return details::GetFullTypeName<T>();
}

/**
 * @brief Retrieves the fully qualified type name of the type of the given
 * instance.
 * @tparam T The type of the instance
 * @param instance The instance to retrieve the type name for (unused)
 * @return Fully qualified type name
 */
template <typename T>
[[nodiscard]] constexpr std::string_view QualifiedTypeNameOf(
    const T& /*instance*/) noexcept {
  return details::GetFullTypeName<std::remove_cvref_t<T>>();
}

}  // namespace helios::utils

namespace std {

template <>
struct hash<helios::utils::TypeId> {
  [[nodiscard]] constexpr size_t operator()(
      const helios::utils::TypeId& type_id) const noexcept {
    return type_id.Index().Hash();
  }
};

template <>
struct hash<helios::utils::TypeIndex> {
  [[nodiscard]] constexpr size_t operator()(
      helios::utils::TypeIndex type_index) const noexcept {
    return type_index.Hash();
  }
};

}  // namespace std
