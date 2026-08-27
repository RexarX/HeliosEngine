#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.memory;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/utils/format.hpp>

#include <format>
#include <string>
#include <utility>
#endif
#include <helios/memory/temporary_storage.hpp>

HELIOS_MODULE_EXPORT
namespace helios::utils {

/**
 * @brief Format a string into a temporary buffer allocated from the calling
 * thread's `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @tparam Args Argument types for the format string
 * @param fmt The format string
 * @param args The arguments to format into the string
 * @return A formatted string, allocated from the calling thread's
 * `TemporaryStorage`
 */
template <typename... Args>
inline auto TempFormat(std::format_string<Args...> fmt, Args&&... args)
    -> std::pmr::string {
  return FormatWith(&mem::GetTemporaryStorage(), fmt,
                    std::forward<Args>(args)...);
}

/**
 * @brief Format a wide string into a temporary buffer allocated from the
 * calling thread's `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @tparam Args Argument types for the format string
 * @param fmt The format string
 * @param args The arguments to format into the string
 * @return A formatted wide string, allocated from the calling thread's
 * `TemporaryStorage`
 */
template <typename... Args>
inline auto TempFormat(std::wformat_string<Args...> fmt, Args&&... args)
    -> std::pmr::wstring {
  return FormatWith(&mem::GetTemporaryStorage(), fmt,
                    std::forward<Args>(args)...);
}

}  // namespace helios::utils
#endif  // HELIOS_MODULE_CONSUMER_SHIM
