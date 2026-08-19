#pragma once

#include <format>
#include <iterator>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>

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
  unsigned long window = 0;
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

/// @brief Backend-agnostic native window handle.
/// @details Populated by whichever window backend plugin is active
/// (`glfw`, future `window_win32`, ...). `std::monostate` means no
/// native handle has been created yet.
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

#ifdef HELIOS_PLATFORM_WINDOWS
/**
 * @brief Formats a Win32 handle using an output iterator.
 * @tparam It Output iterator type
 * @param handle Win32 handle
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const Win32Handle& handle, It out) {
  return std::format_to(out, "Win32Handle{{hwnd={}, hinstance={}}}",
                        handle.hwnd, handle.hinstance);
}
#endif

#ifdef HELIOS_PLATFORM_LINUX_X11
/**
 * @brief Formats an Xlib handle using an output iterator.
 * @tparam It Output iterator type
 * @param handle Xlib handle
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const XlibHandle& handle, It out) {
  return std::format_to(out, "XlibHandle{{display={}, window={}}}",
                        handle.display, handle.window);
}
#endif

#ifdef HELIOS_PLATFORM_LINUX_WAYLAND
/**
 * @brief Formats a Wayland handle using an output iterator.
 * @tparam It Output iterator type
 * @param handle Wayland handle
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const WaylandHandle& handle, It out) {
  return std::format_to(out, "WaylandHandle{{display={}, surface={}}}",
                        handle.display, handle.surface);
}
#endif

#ifdef HELIOS_PLATFORM_MACOS
/**
 * @brief Formats a Cocoa handle using an output iterator.
 * @tparam It Output iterator type
 * @param handle Cocoa handle
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const CocoaHandle& handle, It out) {
  return std::format_to(out, "CocoaHandle{{ns_window={}, ns_view={}}}",
                        handle.ns_window, handle.ns_view);
}
#endif

/**
 * @brief Formats a native handle using an output iterator.
 * @tparam It Output iterator type
 * @param handle Native handle variant
 * @param out Output iterator to write the formatted string to
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(const NativeHandle& handle, It out) {
  return std::visit(
      [&out](const auto& value) -> It {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return std::format_to(out, "NativeHandle{{}}");
        } else {
          return ToString(value, out);
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
  ToString(handle, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a Win32Handle to an output stream.
 * @param os Output stream
 * @param handle Win32Handle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Win32Handle& handle) {
  ToString(handle, std::ostreambuf_iterator<char>(os));
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
  ToString(handle, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a XlibHandle to an output stream.
 * @param os Output stream
 * @param handle XlibHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const XlibHandle& handle) {
  ToString(handle, std::ostreambuf_iterator<char>(os));
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
  ToString(handle, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a WaylandHandle to an output stream.
 * @param os Output stream
 * @param handle WaylandHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const WaylandHandle& handle) {
  ToString(handle, std::ostreambuf_iterator<char>(os));
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
  ToString(handle, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a CocoaHandle to an output stream.
 * @param os Output stream
 * @param handle CocoaHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CocoaHandle& handle) {
  ToString(handle, std::ostreambuf_iterator<char>(os));
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
  ToString(handle, std::back_inserter(result));
  return result;
}

/**
 * @brief Outputs a NativeHandle to an output stream.
 * @param os Output stream
 * @param handle NativeHandle
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const NativeHandle& handle) {
  ToString(handle, std::ostreambuf_iterator<char>(os));
  return os;
}

}  // namespace helios::window

namespace std {

#ifdef HELIOS_PLATFORM_WINDOWS
template <>
struct formatter<helios::window::Win32Handle> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Win32Handle& handle,
                     format_context& ctx) {
    return helios::window::ToString(handle, ctx.out());
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
    return helios::window::ToString(handle, ctx.out());
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
    return helios::window::ToString(handle, ctx.out());
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
    return helios::window::ToString(handle, ctx.out());
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
    return helios::window::ToString(handle, ctx.out());
  }
};

}  // namespace std
