#pragma once

#include <cstddef>
#include <string_view>

namespace helios::utils {

using HashType = size_t;

#if SIZE_MAX == UINT64_MAX
constexpr HashType kFnvBasis = 14695981039346656037ULL;
constexpr HashType kFnvPrime = 1099511628211ULL;
#elif SIZE_MAX == UINT32_MAX
constexpr HashType kFnvBasis = 2166136261U;
constexpr HashType kFnvPrime = 16777619U;
#else
#error "Unsupported platform: size_t must be 32 or 64 bits"
#endif

/**
 * @brief Computes the FNV-1a hash of a string.
 * @param str The input string to hash.
 * @param hash The initial hash value (default is the FNV basis).
 * @return The computed FNV-1a hash of the input string.
 */
[[nodiscard]] constexpr HashType Fnv1aHash(std::string_view str,
                                           HashType hash = kFnvBasis) noexcept {
  for (const auto ch : str) {
    hash = (hash ^ static_cast<HashType>(ch)) * kFnvPrime;
  }
  return hash;
}

/**
 * @brief Computes the FNV-1a hash of a string literal.
 * @tparam N The size of the string literal (including null terminator).
 * @param array The string literal to hash.
 * @return The computed FNV-1a hash of the input string literal.
 */
template <size_t N>
[[nodiscard]] constexpr HashType Fnv1aHash(const char (&array)[N]) noexcept {
  return Fnv1aHash(std::string_view{array, N - 1});
}

}  // namespace helios::utils
