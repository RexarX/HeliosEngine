#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/component/component.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#endif
#include <helios/window/ids.hpp>

HELIOS_MODULE_EXPORT
namespace helios::window {

#ifdef HELIOS_PLATFORM_WINDOWS
/// @brief Win32 native window handle.
struct Win32Handle {
  void* hwnd = nullptr;
  void* hinstance = nullptr;
};
#endif

#ifdef HELIOS_PLATFORM_LINUX_X11
/// @brief X11 native window handle.
struct XlibHandle {
  void* display = nullptr;
  NativeXWindowId window = 0;
};
#endif

#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
/// @brief Wayland native window handle.
struct WaylandHandle {
  void* display = nullptr;
  void* surface = nullptr;
};
#endif

#ifdef HELIOS_PLATFORM_MACOS
/// @brief Cocoa native window handle.
struct CocoaHandle {
  void* ns_window = nullptr;
  void* ns_view = nullptr;
};
#endif

/**
 * @brief Backend-agnostic native window handle.
 * @details Populated by whichever window backend plugin is active.
 * Contains possible win32, x11, wayland, cocoa handles; `std::monostate` means
 * no native handle has been created yet.
 */
using NativeHandle = std::variant<std::monostate
#ifdef HELIOS_PLATFORM_WINDOWS
                                  ,
                                  Win32Handle
#endif
#ifdef HELIOS_PLATFORM_LINUX_X11
                                  ,
                                  XlibHandle
#endif
#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
                                  ,
                                  WaylandHandle
#endif
#ifdef HELIOS_PLATFORM_MACOS
                                  ,
                                  CocoaHandle
#endif
                                  >;

/// @brief Sparse-set component holding the native handle for a window entity.
struct NativeHandleComponent {
  static constexpr std::string_view kName =
      "helios::window::NativeHandleComponent";
  static constexpr auto kStorageType = ecs::ComponentStorageType::kArchetype;

  NativeHandle handle;
};

#ifdef HELIOS_PLATFORM_WINDOWS
/**
 * @brief Formats a Win32 handle using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param handle Win32 handle
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Win32Handle& handle) {
  return std::format_to(out, "Win32Handle{{hwnd={}, hinstance={}}}",
                        handle.hwnd, handle.hinstance);
}
#endif

#ifdef HELIOS_PLATFORM_LINUX_X11
/**
 * @brief Formats an Xlib handle using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param handle Xlib handle
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const XlibHandle& handle) {
  return std::format_to(out, "XlibHandle{{display={}, window={}}}",
                        handle.display, handle.window);
}
#endif

#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
/**
 * @brief Formats a Wayland handle using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param handle Wayland handle
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const WaylandHandle& handle) {
  return std::format_to(out, "WaylandHandle{{display={}, surface={}}}",
                        handle.display, handle.surface);
}
#endif

#ifdef HELIOS_PLATFORM_MACOS
/**
 * @brief Formats a Cocoa handle using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param handle Cocoa handle
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CocoaHandle& handle) {
  return std::format_to(out, "CocoaHandle{{ns_window={}, ns_view={}}}",
                        handle.ns_window, handle.ns_view);
}
#endif

/**
 * @brief Formats a native handle using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param handle Native handle variant
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const NativeHandle& handle) {
  return std::visit(
      [&out](const auto& value) -> It {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return std::format_to(out, "NativeHandle{{}}");
        } else {
          return ToString(out, value);
        }
      },
      handle);
}

#ifdef HELIOS_PLATFORM_WINDOWS
/**
 * @brief Formats a Win32Handle as a string.
 * @param handle Win32Handle
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const Win32Handle& handle) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Formats a Win32Handle as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param handle Win32Handle
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Win32Handle& handle) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Outputs a Win32Handle to an output stream.
 * @param os Output stream
 * @param handle Win32Handle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Win32Handle& handle) {
  ToString(std::ostreambuf_iterator<char>(os), handle);
  return os;
}
#endif
#ifdef HELIOS_PLATFORM_LINUX_X11
/**
 * @brief Formats a XlibHandle as a string.
 * @param handle XlibHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const XlibHandle& handle) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Formats a XlibHandle as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param handle XlibHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const XlibHandle& handle) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Outputs a XlibHandle to an output stream.
 * @param os Output stream
 * @param handle XlibHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const XlibHandle& handle) {
  ToString(std::ostreambuf_iterator<char>(os), handle);
  return os;
}
#endif
#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
/**
 * @brief Formats a WaylandHandle as a string.
 * @param handle WaylandHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const WaylandHandle& handle) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Formats a WaylandHandle as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param handle WaylandHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const WaylandHandle& handle) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Outputs a WaylandHandle to an output stream.
 * @param os Output stream
 * @param handle WaylandHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const WaylandHandle& handle) {
  ToString(std::ostreambuf_iterator<char>(os), handle);
  return os;
}
#endif
#ifdef HELIOS_PLATFORM_MACOS
/**
 * @brief Formats a CocoaHandle as a string.
 * @param handle CocoaHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CocoaHandle& handle) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Formats a CocoaHandle as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param handle CocoaHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const CocoaHandle& handle) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Outputs a CocoaHandle to an output stream.
 * @param os Output stream
 * @param handle CocoaHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CocoaHandle& handle) {
  ToString(std::ostreambuf_iterator<char>(os), handle);
  return os;
}
#endif
/**
 * @brief Formats a NativeHandle as a string.
 * @param handle NativeHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const NativeHandle& handle) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Formats a NativeHandle as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param handle NativeHandle
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const NativeHandle& handle) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), handle);
  return result;
}

/**
 * @brief Outputs a NativeHandle to an output stream.
 * @param os Output stream
 * @param handle NativeHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const NativeHandle& handle) {
  ToString(std::ostreambuf_iterator<char>(os), handle);
  return os;
}

/**
 * @brief Formats a native handle component using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param component Native handle component
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const NativeHandleComponent& component) {
  out = std::format_to(out, "NativeHandleComponent{{handle=");
  out = ToString(out, component.handle);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a native handle component as a string.
 * @param component Native handle component
 * @return Formatted native handle component string
 */
[[nodiscard]] inline std::string ToString(
    const NativeHandleComponent& component) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), component);
  return result;
}

/**
 * @brief Formats a native handle component as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param component NativeHandleComponent
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const NativeHandleComponent& component) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), component);
  return result;
}

/**
 * @brief Outputs a native handle component to an output stream.
 * @param os Output stream
 * @param component Native handle component
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const NativeHandleComponent& component) {
  ToString(std::ostreambuf_iterator<char>(os), component);
  return os;
}

}  // namespace helios::window

HELIOS_MODULE_EXPORT
namespace std {

#ifdef HELIOS_PLATFORM_WINDOWS
template <>
struct formatter<helios::window::Win32Handle> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Win32Handle& handle,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), handle);
  }
};
#endif
#ifdef HELIOS_PLATFORM_LINUX_X11
template <>
struct formatter<helios::window::XlibHandle> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::XlibHandle& handle,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), handle);
  }
};
#endif
#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
template <>
struct formatter<helios::window::WaylandHandle> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::WaylandHandle& handle,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), handle);
  }
};
#endif
#ifdef HELIOS_PLATFORM_MACOS
template <>
struct formatter<helios::window::CocoaHandle> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CocoaHandle& handle,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), handle);
  }
};
#endif
template <>
struct formatter<helios::window::NativeHandle> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::NativeHandle& handle,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), handle);
  }
};

template <>
struct formatter<helios::window::NativeHandleComponent> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::NativeHandleComponent& component,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), component);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
