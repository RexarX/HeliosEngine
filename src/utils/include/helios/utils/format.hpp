#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.utils;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <cstddef>
#include <format>
#include <memory_resource>
#include <string>
#include <utility>
#endif

HELIOS_MODULE_EXPORT
namespace helios::utils {

/**
 * @brief Formats a string and appends it to the given output string.
 * @tparam Traits The character traits of the output string
 * @tparam Allocator The allocator type of the output string
 * @tparam Args The types of the arguments to format
 * @param out The output string to append the formatted string to
 * @param fmt The format string
 * @param args The arguments to format
 * @return An iterator to the end of the output string after appending the
 * formatted string
 */
template <typename Traits, typename Allocator, typename... Args>
inline auto FormatTo(std::basic_string<char, Traits, Allocator>& out,
                     std::format_string<Args...> fmt, Args&&... args) ->
    typename std::basic_string<char, Traits, Allocator>::iterator {
  using difference_type =
      typename std::basic_string<char, Traits, Allocator>::difference_type;

  const size_t old_size = out.size();
  const size_t added = std::formatted_size(fmt, std::forward<Args>(args)...);
  out.resize(old_size + added);
  if (added == 0) [[unlikely]] {
    return out.begin() + static_cast<difference_type>(old_size);
  }
  return std::format_to(out.begin() + static_cast<difference_type>(old_size),
                        fmt, std::forward<Args>(args)...);
}

/**
 * @brief Formats a wide string and appends it to the given output string.
 * @tparam Traits The character traits of the output string
 * @tparam Allocator The allocator type of the output string
 * @tparam Args The types of the arguments to format
 * @param out The output string to append the formatted string to
 * @param fmt The format string
 * @param args The arguments to format
 * @return An iterator to the end of the output string after appending the
 * formatted string
 */
template <typename Traits, typename Allocator, typename... Args>
inline auto FormatTo(std::basic_string<wchar_t, Traits, Allocator>& out,
                     std::wformat_string<Args...> fmt, Args&&... args) ->
    typename std::basic_string<wchar_t, Traits, Allocator>::iterator {
  using difference_type =
      typename std::basic_string<wchar_t, Traits, Allocator>::difference_type;

  const size_t old_size = out.size();
  const size_t added = std::formatted_size(fmt, std::forward<Args>(args)...);
  out.resize(old_size + added);
  if (added == 0) [[unlikely]] {
    return out.begin() + static_cast<difference_type>(old_size);
  }
  return std::format_to(out.begin() + static_cast<difference_type>(old_size),
                        fmt, std::forward<Args>(args)...);
}

/**
 * @brief Formats a string using the provided memory resource and returns it
 * as a new string.
 * @tparam Args The types of the arguments to format
 * @param resource The memory resource to allocate the result from
 * @param fmt The format string
 * @param args The arguments to format
 * @return A new string containing the formatted string
 */
template <typename... Args>
inline auto FormatWith(std::pmr::memory_resource* resource,
                       std::format_string<Args...> fmt, Args&&... args)
    -> std::pmr::string {
  std::pmr::string result{resource};
  FormatTo(result, fmt, std::forward<Args>(args)...);
  return result;
}

/**
 * @brief Formats a wide string using the provided memory resource and
 * returns it as a new string.
 * @tparam Args The types of the arguments to format
 * @param resource The memory resource to allocate the result from
 * @param fmt The format string
 * @param args The arguments to format
 * @return A new wide string containing the formatted string
 */
template <typename... Args>
inline auto FormatWith(std::pmr::memory_resource* resource,
                       std::wformat_string<Args...> fmt, Args&&... args)
    -> std::pmr::wstring {
  std::pmr::wstring result{resource};
  FormatTo(result, fmt, std::forward<Args>(args)...);
  return result;
}

}  // namespace helios::utils
#endif  // HELIOS_MODULE_CONSUMER_SHIM
