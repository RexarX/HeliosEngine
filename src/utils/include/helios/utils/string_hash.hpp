#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.utils;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::utils {

/**
 * @brief Transparent hash functor for string types.
 * @details Enables heterogeneous lookup in containers,  allowing lookups with
 * `std::string_view` without constructing temporary `std::string` objects.
 * Supports `std::string`, `std::string_view`, and `const  char*`.
 */
struct StringHash {
  using is_transparent = void;  ///< Enables heterogeneous lookup
  using hash_type = std::hash<std::string_view>;

  /**
   * @brief Hashes a string-like object.
   * @param str String to hash
   * @return Hash value
   */
  [[nodiscard]] size_t operator()(std::string_view str) const noexcept {
    return hash_type{}(str);
  }

  /**
   * @brief Hashes a std::string.
   * @param str String to hash
   * @return Hash value
   */
  [[nodiscard]] size_t operator()(const std::string& str) const noexcept {
    return hash_type{}(str);
  }

  /**
   * @brief Hashes a C-style string.
   * @param str String to hash
   * @return Hash value
   */
  [[nodiscard]] size_t operator()(const char* str) const noexcept {
    return hash_type{}(str);
  }
};

/**
 * @brief Transparent equality comparator for string types.
 * @details Enables heterogeneous lookup in containers, allowing lookups with
 * `std::string_view` without constructing temporary `std::string` objects.
 * Supports `std::string`, `std::string_view`, and `const char*`.
 */
struct StringEqual {
  using is_transparent = void;  ///< Enables heterogeneous lookup

  /**
   * @brief Compares two `std::string_view` objects for equality.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] constexpr bool operator()(std::string_view lhs,
                                          std::string_view rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares `std::string` with `std::string_view`.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side string view
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(const std::string& lhs,
                                std::string_view rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares `std::string_view` with `std::string`.
   * @param lhs Left-hand side string view
   * @param rhs Right-hand side string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(std::string_view lhs,
                                const std::string& rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares `const char*` string with `std::string_view`.
   * @param lhs Left-hand side C-style string
   * @param rhs Right-hand side string view
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] constexpr bool operator()(const char* lhs,
                                          std::string_view rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares `std::string_view` with `const char*`.
   * @param lhs Left-hand side string view
   * @param rhs Right-hand side C-style string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] constexpr bool operator()(std::string_view lhs,
                                          const char* rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares two `std::string` objects.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(const std::string& lhs,
                                const std::string& rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares `std::string` with `const char*`.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side C-style string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(const std::string& lhs,
                                const char* rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares `const char*` with `std::string`.
   * @param lhs Left-hand side C-style string
   * @param rhs Right-hand side string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(const char* lhs,
                                const std::string& rhs) const noexcept {
    return lhs == rhs;
  }

  /**
   * @brief Compares two `const char*` strings.
   * @param lhs Left-hand side C-style string
   * @param rhs Right-hand side C-style string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(const char* lhs,
                                const char* rhs) const noexcept {
    return std::string_view{lhs} == std::string_view{rhs};
  }
};

/**
 * @brief Transparent strict-weak-order comparator for string types.
 * @details Enables heterogeneous lookup in containers,  allowing lookups with
 * `std::string_view` without constructing temporary `std::string` objects.
 * Supports `std::string`, `std::string_view`, and `const  char*`.
 */
struct StringLess {
  using is_transparent = void;  ///< Enables heterogeneous lookup

  /**
   * @brief Compares `std::string_view` with `std::string_view`.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side string
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] constexpr bool operator()(std::string_view lhs,
                                          std::string_view rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares `std::string` with `std::string_view`.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side string view
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] bool operator()(const std::string& lhs,
                                std::string_view rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares `std::string_view` with `std::string`.
   * @param lhs Left-hand side string view
   * @param rhs Right-hand side string
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] bool operator()(std::string_view lhs,
                                const std::string& rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares `const char*` with `std::string_view`.
   * @param lhs Left-hand side C-style string
   * @param rhs Right-hand side string view
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] constexpr bool operator()(const char* lhs,
                                          std::string_view rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares std::string_view with C-style string.
   * @param lhs Left-hand side string view
   * @param rhs Right-hand side C-style string
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] constexpr bool operator()(std::string_view lhs,
                                          const char* rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares two `std::string` objects.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side string
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] bool operator()(const std::string& lhs,
                                const std::string& rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares `std::string` with `const char*`.
   * @param lhs Left-hand side string
   * @param rhs Right-hand side C-style string
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] bool operator()(const std::string& lhs,
                                const char* rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares `const char*` with `std::string`.
   * @param lhs Left-hand side C-style string
   * @param rhs Right-hand side string
   * @return True if `lhs` is less than `rhs`, false otherwise
   */
  [[nodiscard]] bool operator()(const char* lhs,
                                const std::string& rhs) const noexcept {
    return lhs < rhs;
  }

  /**
   * @brief Compares two `const char*` strings.
   * @param lhs Left-hand side C-style string
   * @param rhs Right-hand side C-style string
   * @return True if strings are equal, false otherwise
   */
  [[nodiscard]] bool operator()(const char* lhs,
                                const char* rhs) const noexcept {
    return std::string_view{lhs} < std::string_view{rhs};
  }
};

}  // namespace helios::utils
#endif  // HELIOS_MODULE_CONSUMER_SHIM
