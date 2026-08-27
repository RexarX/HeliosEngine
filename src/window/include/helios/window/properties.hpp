#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/component/bundle.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <algorithm>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#endif
#include <helios/window/ids.hpp>
#include <helios/window/monitor.hpp>

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Window presentation mode.
enum class Mode : uint8_t {
  kWindowed = 0,    ///< Standard windowed mode.
  kBorderless = 1,  ///< Borderless fullscreen window.
  kFullscreen = 2,  ///< Exclusive fullscreen mode.
};

/**
 * @brief Tests whether a mode covers an entire monitor surface.
 * @param mode Presentation mode
 * @return True for borderless or exclusive fullscreen
 */
[[nodiscard]] constexpr bool IsFullscreenPresentation(Mode mode) noexcept {
  return mode == Mode::kBorderless || mode == Mode::kFullscreen;
}

/// @brief Cursor visibility and capture mode.
enum class CursorMode : uint8_t {
  kVisible = 0,   ///< Cursor is visible and free.
  kHidden = 1,    ///< Cursor is hidden but not captured.
  kDisabled = 2,  ///< Cursor is hidden and captured (raw input).
  kCaptured = 3,  ///< Cursor is visible and confined to the window.
};

/// @brief Graphics client API requested at window creation.
enum class ClientApi : uint8_t {
  kNone = 0,  ///< No graphics client API; for Vulkan/DX/NRI/SDL-no-GL backends.
  kOpenGL = 1,  ///< OpenGL context at window creation.
};

/// @brief Bitmask of pending property changes for backend synchronization.
enum class DirtyFlag : uint32_t {
  kNone = 0,
  kTitle = 1U << 0U,
  kSize = 1U << 1U,
  kPos = 1U << 2U,
  kMode = 1U << 3U,
  kCursor = 1U << 4U,
  kVisible = 1U << 5U,
  kResizable = 1U << 6U,
  kDecorated = 1U << 7U,
  kMonitor = 1U << 8U,
  kIcon = 1U << 9U,
  kMaximized = 1U << 10U,
  kSizeLimits = 1U << 11U,
  kAspectRatio = 1U << 12U,
  kOpacity = 1U << 13U,
  kFloating = 1U << 14U,
  kAutoIconify = 1U << 15U,
  kFocusOnShow = 1U << 16U,
  kAttention = 1U << 17U,
  kFocus = 1U << 18U,
  kMousePassthrough = 1U << 19U,
  kRefreshRate = 1U << 20U,
};

/**
 * @brief Combines dirty flags.
 * @param lhs Left-hand side flag
 * @param rhs Right-hand side flag
 * @return Combined flag
 */
[[nodiscard]] constexpr DirtyFlag operator|(DirtyFlag lhs,
                                            DirtyFlag rhs) noexcept {
  return static_cast<DirtyFlag>(std::to_underlying(lhs) |
                                std::to_underlying(rhs));
}

/**
 * @brief Tests whether a dirty flag is set.
 * @param flags Combined dirty flags
 * @param flag Flag to test
 * @return True if the flag is set, false otherwise
 */
[[nodiscard]] constexpr bool HasFlag(DirtyFlag flags, DirtyFlag flag) noexcept {
  return (std::to_underlying(flags) & std::to_underlying(flag)) != 0U;
}

/// @brief RGBA8 window icon image.
struct IconImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> rgba;

  /**
   * @brief Returns the image size in pixels.
   * @return Width and height
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/// @brief Creation-time and mutable window properties.
struct Properties {
  std::string title = "Helios";
  std::vector<IconImage> icons;
  std::optional<uint32_t> width;
  std::optional<uint32_t> height;
  std::optional<uint32_t> client_width;
  std::optional<uint32_t> client_height;
  std::optional<int32_t> pos_x;
  std::optional<int32_t> pos_y;
  std::optional<MonitorId> monitor_index;
  std::optional<uint32_t> refresh_rate;
  std::optional<uint32_t> min_width;
  std::optional<uint32_t> min_height;
  std::optional<uint32_t> max_width;
  std::optional<uint32_t> max_height;
  std::optional<int32_t> aspect_numer;
  std::optional<int32_t> aspect_denom;
  float content_scale_x = 1.0F;
  float content_scale_y = 1.0F;
  float opacity = 1.0F;
  Mode mode = Mode::kWindowed;
  CursorMode cursor_mode = CursorMode::kVisible;
  ClientApi client_api = ClientApi::kNone;
  bool visible = true;
  bool focused = true;
  bool resizable = true;
  bool decorated = true;
  bool maximized = false;
  bool floating = false;
  bool auto_iconify = true;
  bool focus_on_show = true;
  bool hovered = false;
  bool transparent_framebuffer = false;
  bool scale_to_monitor = false;
  bool scale_framebuffer = true;
  bool mouse_passthrough = false;

  /**
   * @brief Returns the window size.
   * @return Width and height, if set
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<std::optional<uint32_t>, std::optional<uint32_t>> {
    return {width, height};
  }

  /**
   * @brief Returns the client-area size.
   * @return Client width and height, if set
   */
  [[nodiscard]] constexpr auto GetClientSize() const noexcept
      -> std::pair<std::optional<uint32_t>, std::optional<uint32_t>> {
    return {client_width, client_height};
  }

  /**
   * @brief Returns the window position.
   * @return X and y, if set
   */
  [[nodiscard]] constexpr auto GetPos() const noexcept
      -> std::pair<std::optional<int32_t>, std::optional<int32_t>> {
    return {pos_x, pos_y};
  }

  /**
   * @brief Returns the content scale.
   * @return Horizontal and vertical scale
   */
  [[nodiscard]] constexpr auto GetContentScale() const noexcept
      -> std::pair<float, float> {
    return {content_scale_x, content_scale_y};
  }

  /**
   * @brief Returns the minimum size limits.
   * @return Minimum width and height, if set
   */
  [[nodiscard]] constexpr auto GetMinSize() const noexcept
      -> std::pair<std::optional<uint32_t>, std::optional<uint32_t>> {
    return {min_width, min_height};
  }

  /**
   * @brief Returns the maximum size limits.
   * @return Maximum width and height, if set
   */
  [[nodiscard]] constexpr auto GetMaxSize() const noexcept
      -> std::pair<std::optional<uint32_t>, std::optional<uint32_t>> {
    return {max_width, max_height};
  }

  /**
   * @brief Returns the aspect-ratio constraint.
   * @return Numerator and denominator, if set
   */
  [[nodiscard]] constexpr auto GetAspectRatio() const noexcept
      -> std::pair<std::optional<int32_t>, std::optional<int32_t>> {
    return {aspect_numer, aspect_denom};
  }
};

/**
 * @brief Computes a default client size for a monitor resolution.
 * @details Uses most of the screen on portrait displays and a balanced fraction
 * on landscape displays so fixed 1280x720 is not assumed.
 * @param screen_width Monitor width in pixels
 * @param screen_height Monitor height in pixels
 * @return Default width and height in pixels
 */
[[nodiscard]] constexpr auto DefaultSizeForScreen(
    uint32_t screen_width, uint32_t screen_height) noexcept
    -> std::pair<uint32_t, uint32_t> {
  constexpr uint32_t kMinSize = 320;

  if (screen_width == 0 || screen_height == 0) {
    return {1280, 720};
  }

  if (screen_height > screen_width) {
    const uint32_t width =
        std::clamp((screen_width * 9U) / 10U, kMinSize, screen_width);
    const uint32_t height =
        std::clamp((width * 16U) / 10U, kMinSize, (screen_height * 9U) / 10U);
    return {width, height};
  }

  const uint32_t width =
      std::clamp((screen_width * 2U) / 3U, kMinSize, screen_width);
  const uint32_t height =
      std::clamp((screen_height * 2U) / 3U, kMinSize, screen_height);
  return {width, height};
}

/// @brief Exclusive-fullscreen width, height, and refresh rate.
struct ExclusiveVideoMode {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t refresh_rate = 0;

  /**
   * @brief Returns the video-mode size.
   * @return Width and height
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/**
 * @brief Resolves exclusive-fullscreen size and refresh rate.
 * @details Unset `width`, `height`, and `refresh_rate` fall back to the
 * current desktop video mode. Borderless fullscreen ignores this helper.
 * @param properties Window properties
 * @param desktop Current desktop video mode
 * @return Size and refresh rate to request from the backend
 */
[[nodiscard]] constexpr ExclusiveVideoMode ResolveExclusiveVideoMode(
    const Properties& properties, ExclusiveVideoMode desktop) noexcept {
  return {
      .width = properties.width.value_or(desktop.width),
      .height = properties.height.value_or(desktop.height),
      .refresh_rate = properties.refresh_rate.value_or(desktop.refresh_rate),
  };
}

/// @brief Describes a logical window entity.
struct Window {
  static constexpr std::string_view kName = "helios::window::Window";

  Properties properties;
  DirtyFlag dirty_flags = DirtyFlag::kNone;
  bool close_requested = false;

  /**
   * @brief Creates a window component from creation properties.
   * @param value Initial window properties
   * @return Window component
   */
  [[nodiscard]] static constexpr Window FromProperties(
      Properties value) noexcept {
    return {.properties = std::move(value)};
  }

  /// @brief Requests that the backend close the OS window.
  constexpr void RequestClose() noexcept { close_requested = true; }

  /**
   * @brief Marks one or more properties as pending backend synchronization.
   * @param flags Dirty flags to set
   */
  constexpr void MarkDirty(DirtyFlag flags) noexcept {
    dirty_flags = dirty_flags | flags;
  }

  /**
   * @brief Clears dirty flags after backend synchronization.
   * @param flags Dirty flags to clear
   */
  constexpr void ClearDirty(DirtyFlag flags) noexcept {
    dirty_flags = static_cast<DirtyFlag>(std::to_underlying(dirty_flags) &
                                         ~std::to_underlying(flags));
  }

  /// @brief Clears all dirty flags after backend synchronization.
  constexpr void ClearDirty() noexcept { dirty_flags = DirtyFlag::kNone; }

  /**
   * @brief Tests whether a dirty flag is set.
   * @param flag Dirty flag to test
   * @return True when the flag is set
   */
  [[nodiscard]] constexpr bool Dirty(DirtyFlag flag) const noexcept {
    return HasFlag(dirty_flags, flag);
  }

  /**
   * @brief Sets the window title and marks it dirty.
   * @param value New title
   */
  constexpr void SetTitle(std::string value) {
    properties.title = std::move(value);
    MarkDirty(DirtyFlag::kTitle);
  }

  /**
   * @brief Sets the window size and marks it dirty.
   * @param new_width Width in pixels
   * @param new_height Height in pixels
   */
  constexpr void SetSize(uint32_t new_width, uint32_t new_height) noexcept {
    properties.width = new_width;
    properties.height = new_height;
    MarkDirty(DirtyFlag::kSize);
  }

  /**
   * @brief Sets the window position and marks it dirty.
   * @param x Horizontal position in screen coordinates
   * @param y Vertical position in screen coordinates
   */
  constexpr void SetPos(int32_t x, int32_t y) noexcept {
    properties.pos_x = x;
    properties.pos_y = y;
    MarkDirty(DirtyFlag::kPos);
  }

  /// @brief Clears an explicit position so the backend chooses placement.
  constexpr void ClearPos() noexcept {
    properties.pos_x.reset();
    properties.pos_y.reset();
    MarkDirty(DirtyFlag::kPos);
  }

  /**
   * @brief Sets the target monitor index and marks it dirty.
   * @param value GLFW monitor index from `Monitors`
   */
  constexpr void SetMonitorIndex(MonitorId value) noexcept {
    properties.monitor_index = value;
    MarkDirty(DirtyFlag::kMonitor);
  }

  /// @brief Clears an explicit monitor index so the backend infers placement.
  constexpr void ClearMonitorIndex() noexcept {
    properties.monitor_index.reset();
    MarkDirty(DirtyFlag::kMonitor);
  }

  /**
   * @brief Sets the presentation mode and marks it dirty.
   * @param value New presentation mode
   */
  constexpr void SetMode(Mode value) noexcept {
    properties.mode = value;
    MarkDirty(DirtyFlag::kMode);
  }

  /**
   * @brief Sets the exclusive-fullscreen refresh rate and marks it dirty.
   * @param value Refresh rate in Hz. Pick a value from `Monitor::modes`.
   * @note Ignored in borderless fullscreen, which cannot change the display
   * mode.
   */
  constexpr void SetRefreshRate(uint32_t value) noexcept {
    properties.refresh_rate = value;
    MarkDirty(DirtyFlag::kRefreshRate);
  }

  /// @brief Clears an explicit refresh rate so exclusive fullscreen uses the
  /// desktop Hz.
  constexpr void ClearRefreshRate() noexcept {
    properties.refresh_rate.reset();
    MarkDirty(DirtyFlag::kRefreshRate);
  }

  /**
   * @brief Applies a monitor video mode's size and refresh rate.
   * @details Does not change presentation `Mode`. Call `SetMode` separately
   * when entering exclusive or borderless fullscreen.
   * @param mode Video mode from `Monitor::modes` or `Monitor::current`
   * @note Refresh rate is ignored in borderless fullscreen, which cannot
   * change the display mode.
   */
  constexpr void SetVideoMode(VideoMode mode) noexcept {
    SetSize(mode.width, mode.height);
    SetRefreshRate(mode.refresh_rate);
  }

  /**
   * @brief Sets the cursor mode and marks it dirty.
   * @param value New cursor mode
   */
  constexpr void SetCursorMode(CursorMode value) noexcept {
    properties.cursor_mode = value;
    MarkDirty(DirtyFlag::kCursor);
  }

  /**
   * @brief Sets visibility and marks it dirty.
   * @param value Whether the window should be visible
   */
  constexpr void SetVisible(bool value) noexcept {
    properties.visible = value;
    MarkDirty(DirtyFlag::kVisible);
  }

  /**
   * @brief Sets whether the window is resizable and marks it dirty.
   * @param value Whether the window is resizable
   */
  constexpr void SetResizable(bool value) noexcept {
    properties.resizable = value;
    MarkDirty(DirtyFlag::kResizable);
  }

  /**
   * @brief Sets whether the window has decorations and marks it dirty.
   * @param value Whether the window has decorations
   */
  constexpr void SetDecorated(bool value) noexcept {
    properties.decorated = value;
    MarkDirty(DirtyFlag::kDecorated);
  }

  /**
   * @brief Sets the window icon images and marks them dirty.
   * @param value Icon images in RGBA8 format
   */
  void SetIcons(std::vector<IconImage> value) {
    properties.icons = std::move(value);
    MarkDirty(DirtyFlag::kIcon);
  }

  /**
   * @brief Sets whether the window is maximized and marks it dirty.
   * @param value Whether the window should be maximized
   */
  constexpr void SetMaximized(bool value) noexcept {
    properties.maximized = value;
    MarkDirty(DirtyFlag::kMaximized);
  }

  /**
   * @brief Sets minimum window size limits and marks them dirty.
   * @param width Minimum width in pixels, or unset for no limit
   * @param height Minimum height in pixels, or unset for no limit
   */
  constexpr void SetMinSize(std::optional<uint32_t> width,
                            std::optional<uint32_t> height) noexcept {
    properties.min_width = width;
    properties.min_height = height;
    MarkDirty(DirtyFlag::kSizeLimits);
  }

  /**
   * @brief Sets maximum window size limits and marks them dirty.
   * @param width Maximum width in pixels, or unset for no limit
   * @param height Maximum height in pixels, or unset for no limit
   */
  constexpr void SetMaxSize(std::optional<uint32_t> width,
                            std::optional<uint32_t> height) noexcept {
    properties.max_width = width;
    properties.max_height = height;
    MarkDirty(DirtyFlag::kSizeLimits);
  }

  /**
   * @brief Sets the window aspect ratio and marks it dirty.
   * @param numer Aspect numerator, or unset to clear
   * @param denom Aspect denominator, or unset to clear
   */
  constexpr void SetAspectRatio(std::optional<int32_t> numer,
                                std::optional<int32_t> denom) noexcept {
    properties.aspect_numer = numer;
    properties.aspect_denom = denom;
    MarkDirty(DirtyFlag::kAspectRatio);
  }

  /// @brief Clears the aspect ratio constraint.
  constexpr void ClearAspectRatio() noexcept {
    properties.aspect_numer.reset();
    properties.aspect_denom.reset();
    MarkDirty(DirtyFlag::kAspectRatio);
  }

  /**
   * @brief Sets window opacity and marks it dirty.
   * @param value Opacity from 0 (transparent) to 1 (opaque)
   */
  constexpr void SetOpacity(float value) noexcept {
    properties.opacity = value;
    MarkDirty(DirtyFlag::kOpacity);
  }

  /**
   * @brief Sets whether the window is always on top and marks it dirty.
   * @param value Whether the window floats above others
   */
  constexpr void SetFloating(bool value) noexcept {
    properties.floating = value;
    MarkDirty(DirtyFlag::kFloating);
  }

  /**
   * @brief Sets whether fullscreen windows auto-minimize on focus loss.
   * @param value Whether auto-iconify is enabled
   */
  constexpr void SetAutoIconify(bool value) noexcept {
    properties.auto_iconify = value;
    MarkDirty(DirtyFlag::kAutoIconify);
  }

  /**
   * @brief Sets whether showing the window takes focus.
   * @param value Whether focus-on-show is enabled
   */
  constexpr void SetFocusOnShow(bool value) noexcept {
    properties.focus_on_show = value;
    MarkDirty(DirtyFlag::kFocusOnShow);
  }

  /// @brief Requests that the OS draw user attention to this window.
  constexpr void RequestAttention() noexcept {
    MarkDirty(DirtyFlag::kAttention);
  }

  /// @brief Requests that the OS window receive input focus.
  constexpr void RequestFocus() noexcept { MarkDirty(DirtyFlag::kFocus); }

  /**
   * @brief Sets whether mouse events pass through the window.
   * @param value Whether mouse passthrough is enabled
   */
  constexpr void SetMousePassthrough(bool value) noexcept {
    properties.mouse_passthrough = value;
    MarkDirty(DirtyFlag::kMousePassthrough);
  }

  /**
   * @brief Returns the window size.
   * @return Width and height, if set
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<std::optional<uint32_t>, std::optional<uint32_t>> {
    return properties.GetSize();
  }

  /**
   * @brief Returns the client-area size.
   * @return Client width and height, if set
   */
  [[nodiscard]] constexpr auto GetClientSize() const noexcept
      -> std::pair<std::optional<uint32_t>, std::optional<uint32_t>> {
    return properties.GetClientSize();
  }

  /**
   * @brief Returns the window position.
   * @return X and y, if set
   */
  [[nodiscard]] constexpr auto GetPos() const noexcept
      -> std::pair<std::optional<int32_t>, std::optional<int32_t>> {
    return properties.GetPos();
  }

  /**
   * @brief Returns the content scale.
   * @return Horizontal and vertical scale
   */
  [[nodiscard]] constexpr auto GetContentScale() const noexcept
      -> std::pair<float, float> {
    return properties.GetContentScale();
  }
};

/// @brief Marks the primary application window.
struct Primary {
  static constexpr std::string_view kName = "helios::window::Primary";
  static constexpr auto kStorageType = ecs::ComponentStorageType::kSparseSet;
};

/// @brief Marks a window entity whose OS window creation failed.
struct CreationFailed {
  static constexpr std::string_view kName = "helios::window::CreationFailed";
  static constexpr auto kStorageType = ecs::ComponentStorageType::kSparseSet;
};

struct PrimaryWindow {
  using ComponentTypes = ecs::ComponentBundleTypes<Window, Primary>;

  Window window;

  [[nodiscard]] constexpr ComponentTypes Build() noexcept {
    return {std::move(window), Primary{}};
  }
};

/// @brief Sent after a backend creates the OS window for an entity.
struct CreatedMsg {
  static constexpr std::string_view kName = "helios::window::CreatedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  Properties properties;
};

/// @brief Sent after a backend destroys the OS window for an entity.
struct ClosedMsg {
  static constexpr std::string_view kName = "helios::window::ClosedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
};

/// @brief Sent when a window framebuffer size changes.
struct ResizedMsg {
  static constexpr std::string_view kName = "helios::window::ResizedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  uint32_t width = 0;
  uint32_t height = 0;

  /**
   * @brief Returns the framebuffer size.
   * @return Width and height in pixels
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/// @brief Sent when a window client-area size changes.
struct ClientResizedMsg {
  static constexpr std::string_view kName = "helios::window::ClientResizedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  uint32_t width = 0;
  uint32_t height = 0;

  /**
   * @brief Returns the client-area size.
   * @return Width and height in pixels
   */
  [[nodiscard]] constexpr auto GetSize() const noexcept
      -> std::pair<uint32_t, uint32_t> {
    return {width, height};
  }
};

/// @brief Sent when a window content scale changes.
struct ContentScaleChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ContentScaleChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  float scale_x = 1.0F;
  float scale_y = 1.0F;

  /**
   * @brief Returns the content scale.
   * @return Horizontal and vertical scale
   */
  [[nodiscard]] constexpr auto GetScale() const noexcept
      -> std::pair<float, float> {
    return {scale_x, scale_y};
  }
};

/// @brief Sent when a window position changes.
struct PosChangedMsg {
  static constexpr std::string_view kName = "helios::window::PosChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  int32_t x = 0;
  int32_t y = 0;

  /**
   * @brief Returns the window position.
   * @return X and y in screen coordinates
   */
  [[nodiscard]] constexpr auto GetPos() const noexcept
      -> std::pair<int32_t, int32_t> {
    return {x, y};
  }
};

/// @brief Sent when a window presentation mode changes.
struct ModeChangedMsg {
  static constexpr std::string_view kName = "helios::window::ModeChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  Mode mode = Mode::kWindowed;
};

/// @brief Sent when a window cursor mode changes.
struct CursorModeChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::CursorModeChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  CursorMode cursor_mode = CursorMode::kVisible;
};

/// @brief Sent when a window visibility changes.
struct VisibilityChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::VisibilityChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool visible = true;
};

/// @brief Sent when a window gains or loses focus.
struct FocusChangedMsg {
  static constexpr std::string_view kName = "helios::window::FocusChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool focused = false;
};

/// @brief Sent when a window maximize state changes.
struct MaximizedChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::MaximizedChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool maximized = false;
};

/// @brief Sent when a window icon set changes.
struct IconChangedMsg {
  static constexpr std::string_view kName = "helios::window::IconChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  std::vector<IconImage> icons;
};

/// @brief Sent when a window resizable state changes.
struct ResizableChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ResizableChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool resizable = true;
};

/// @brief Sent when a window decorated state changes.
struct DecoratedChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::DecoratedChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool decorated = true;
};

/// @brief Sent when a window opacity changes.
struct OpacityChangedMsg {
  static constexpr std::string_view kName = "helios::window::OpacityChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  float opacity = 1.0F;
};

/// @brief Sent when a window floating state changes.
struct FloatingChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::FloatingChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool floating = false;
};

/// @brief Sent when the cursor enters or leaves a window.
struct HoverChangedMsg {
  static constexpr std::string_view kName = "helios::window::HoverChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool hovered = false;
};

/// @brief Sent when mouse passthrough state changes.
struct MousePassthroughChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::MousePassthroughChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  bool mouse_passthrough = false;
};

/// @brief Sent when files are dropped onto a window.
struct DroppedFilesMsg {
  static constexpr std::string_view kName = "helios::window::DroppedFilesMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  std::vector<std::string> paths;
};

/// @brief Sent when the user requests to close a window.
struct CloseRequestedMsg {
  static constexpr std::string_view kName = "helios::window::CloseRequestedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
};

/// @brief Sent when OS window creation fails for an entity.
struct CreationFailedMsg {
  static constexpr std::string_view kName = "helios::window::CreationFailedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  std::string reason;
};

/**
 * @brief Returns the string name of a window presentation mode.
 * @param mode Presentation mode
 * @return String name of the mode
 */
[[nodiscard]] constexpr std::string_view ToString(Mode mode) noexcept {
  switch (mode) {
    using enum Mode;
    case kWindowed:
      return "Windowed";
    case kBorderless:
      return "Borderless";
    case kFullscreen:
      return "Fullscreen";
  }
  return "unknown";
}

/**
 * @brief Outputs a window presentation mode to an output stream.
 * @param os Output stream
 * @param mode Presentation mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, Mode mode) {
  return os << "Mode::" << ToString(mode);
}

/**
 * @brief Returns the string name of a cursor mode.
 * @param mode Cursor mode
 * @return String name of the mode
 */
[[nodiscard]] constexpr std::string_view ToString(CursorMode mode) noexcept {
  switch (mode) {
    using enum CursorMode;
    case kVisible:
      return "Visible";
    case kHidden:
      return "Hidden";
    case kDisabled:
      return "Disabled";
    case kCaptured:
      return "Captured";
  }
  return "unknown";
}

/**
 * @brief Outputs a cursor mode to an output stream.
 * @param os Output stream
 * @param mode Cursor mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CursorMode mode) {
  return os << "CursorMode::" << ToString(mode);
}

/**
 * @brief Returns the string name of a client API.
 * @param api Client API
 * @return String name of the API
 */
[[nodiscard]] constexpr std::string_view ToString(ClientApi api) noexcept {
  switch (api) {
    using enum ClientApi;
    case kNone:
      return "None";
    case kOpenGL:
      return "OpenGL";
  }
  return "unknown";
}

/**
 * @brief Outputs a client API to an output stream.
 * @param os Output stream
 * @param api Client API
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ClientApi api) {
  return os << "ClientApi::" << ToString(api);
}

/**
 * @brief Formats dirty flags as a pipe-separated list and writes to an output
 * iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param flags Combined dirty flags
 * @param with_prefix Whether to include a "DirtyFlag::" prefix for each flag
 * @return Updated output iterator after writing the formatted string
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, DirtyFlag flags, bool with_prefix = false) {
  const std::string_view kNoneStr = with_prefix ? "DirtyFlag::None" : "None";

  if (flags == DirtyFlag::kNone) {
    return std::format_to(out, "{}", kNoneStr);
  }

  bool first = true;
  const auto append = [flags, &out, &first, with_prefix](std::string_view name,
                                                         DirtyFlag flag) {
    if (!HasFlag(flags, flag)) {
      return;
    }
    if (!first) {
      out = std::format_to(out, " | ");
    }
    first = false;
    out = with_prefix ? std::format_to(out, "DirtyFlag::{}", name)
                      : std::format_to(out, "{}", name);
  };

  append("Title", DirtyFlag::kTitle);
  append("Size", DirtyFlag::kSize);
  append("Pos", DirtyFlag::kPos);
  append("Mode", DirtyFlag::kMode);
  append("Cursor", DirtyFlag::kCursor);
  append("Visible", DirtyFlag::kVisible);
  append("Resizable", DirtyFlag::kResizable);
  append("Decorated", DirtyFlag::kDecorated);
  append("Monitor", DirtyFlag::kMonitor);
  append("Icon", DirtyFlag::kIcon);
  append("Maximized", DirtyFlag::kMaximized);
  append("SizeLimits", DirtyFlag::kSizeLimits);
  append("AspectRatio", DirtyFlag::kAspectRatio);
  append("Opacity", DirtyFlag::kOpacity);
  append("Floating", DirtyFlag::kFloating);
  append("AutoIconify", DirtyFlag::kAutoIconify);
  append("FocusOnShow", DirtyFlag::kFocusOnShow);
  append("Attention", DirtyFlag::kAttention);
  append("Focus", DirtyFlag::kFocus);
  append("MousePassthrough", DirtyFlag::kMousePassthrough);
  append("RefreshRate", DirtyFlag::kRefreshRate);
  return out;
}

/**
 * @brief Formats dirty flags as a pipe-separated list of flag names.
 * @param flags Combined dirty flags
 * @param with_prefix Whether to include a "DirtyFlag::" prefix for each flag
 * @return Formatted flag names
 */
[[nodiscard]] inline std::string ToString(DirtyFlag flags,
                                          bool with_prefix = false) {
  std::string result;
  result.reserve(32);
  ToString(std::back_inserter(result), flags, with_prefix);
  return result;
}

/**
 * @brief Formats dirty flags as a pipe-separated list of flag names using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param flags Combined dirty flags
 * @param with_prefix Whether to include a prefix for each flag
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(DirtyFlag flags,
                                                   bool with_prefix = false) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(32);
  ToString(std::back_inserter(result), flags, with_prefix);
  return result;
}

/**
 * @brief Outputs dirty flags to an output stream.
 * @param os Output stream
 * @param flags Combined dirty flags
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, DirtyFlag flags) {
  ToString(std::ostreambuf_iterator<char>(os), flags);
  return os;
}

/**
 * @brief Formats an icon image using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param image Icon image
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const IconImage& image) {
  return std::format_to(out, "IconImage{{width={}, height={}, rgba={}}}",
                        image.width, image.height, image.rgba.size());
}

/**
 * @brief Formats an icon image as a string.
 * @param image Icon image
 * @return Formatted icon image string
 */
[[nodiscard]] inline std::string ToString(const IconImage& image) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), image);
  return result;
}

/**
 * @brief Formats an icon image as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param image IconImage
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const IconImage& image) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), image);
  return result;
}

/**
 * @brief Outputs an icon image to an output stream.
 * @param os Output stream
 * @param image Icon image
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const IconImage& image) {
  ToString(std::ostreambuf_iterator<char>(os), image);
  return os;
}

/**
 * @brief Formats window properties using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param properties Window properties
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Properties& properties) {
  out = std::format_to(out, "Properties{{");
  out = std::format_to(out, "title=\"{}\"", properties.title);
  out = std::format_to(out, ", icons={}", properties.icons.size());

  if (properties.width.has_value()) [[likely]] {
    out = std::format_to(out, ", width={}", *properties.width);
  }
  if (properties.height.has_value()) [[likely]] {
    out = std::format_to(out, ", height={}", *properties.height);
  }
  if (properties.client_width.has_value()) [[likely]] {
    out = std::format_to(out, ", client_width={}", *properties.client_width);
  }
  if (properties.client_height.has_value()) [[likely]] {
    out = std::format_to(out, ", client_height={}", *properties.client_height);
  }
  if (properties.pos_x.has_value()) [[likely]] {
    out = std::format_to(out, ", pos_x={}", *properties.pos_x);
  }
  if (properties.pos_y.has_value()) [[likely]] {
    out = std::format_to(out, ", pos_y={}", *properties.pos_y);
  }
  if (properties.monitor_index.has_value()) [[likely]] {
    out = std::format_to(out, ", monitor_index={}", *properties.monitor_index);
  }
  if (properties.refresh_rate.has_value()) [[likely]] {
    out = std::format_to(out, ", refresh_rate={}", *properties.refresh_rate);
  }
  if (properties.min_width.has_value()) [[likely]] {
    out = std::format_to(out, ", min_width={}", *properties.min_width);
  }
  if (properties.min_height.has_value()) [[likely]] {
    out = std::format_to(out, ", min_height={}", *properties.min_height);
  }
  if (properties.max_width.has_value()) [[likely]] {
    out = std::format_to(out, ", max_width={}", *properties.max_width);
  }
  if (properties.max_height.has_value()) [[likely]] {
    out = std::format_to(out, ", max_height={}", *properties.max_height);
  }
  if (properties.aspect_numer.has_value()) [[likely]] {
    out = std::format_to(out, ", aspect_numer={}", *properties.aspect_numer);
  }
  if (properties.aspect_denom.has_value()) [[likely]] {
    out = std::format_to(out, ", aspect_denom={}", *properties.aspect_denom);
  }

  out = std::format_to(out, ", content_scale_x={}", properties.content_scale_x);
  out = std::format_to(out, ", content_scale_y={}", properties.content_scale_y);
  out = std::format_to(out, ", opacity={}", properties.opacity);
  out = std::format_to(out, ", mode={}", ToString(properties.mode));
  out =
      std::format_to(out, ", cursor_mode={}", ToString(properties.cursor_mode));
  out = std::format_to(out, ", client_api={}", ToString(properties.client_api));
  out = std::format_to(out, ", visible={}", properties.visible);
  out = std::format_to(out, ", focused={}", properties.focused);
  out = std::format_to(out, ", resizable={}", properties.resizable);
  out = std::format_to(out, ", decorated={}", properties.decorated);
  out = std::format_to(out, ", maximized={}", properties.maximized);
  out = std::format_to(out, ", floating={}", properties.floating);
  out = std::format_to(out, ", auto_iconify={}", properties.auto_iconify);
  out = std::format_to(out, ", focus_on_show={}", properties.focus_on_show);
  out = std::format_to(out, ", hovered={}", properties.hovered);
  out = std::format_to(out, ", transparent_framebuffer={}",
                       properties.transparent_framebuffer);
  out =
      std::format_to(out, ", scale_to_monitor={}", properties.scale_to_monitor);
  out = std::format_to(out, ", scale_framebuffer={}",
                       properties.scale_framebuffer);
  out = std::format_to(out, ", mouse_passthrough={}",
                       properties.mouse_passthrough);
  out = std::format_to(out, "}}");

  return out;
}

/**
 * @brief Formats window properties as a string.
 * @param properties Window properties
 * @return Formatted properties string
 */
[[nodiscard]] inline std::string ToString(const Properties& properties) {
  std::string result;
  result.reserve(256);  // Reserve reasonable space for typical properties
  ToString(std::back_inserter(result), properties);
  return result;
}

/**
 * @brief Formats window properties as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param properties Window properties
 * @return Formatted properties string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const Properties& properties) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), properties);
  return result;
}

/**
 * @brief Outputs window properties to an output stream.
 * @param os Output stream
 * @param properties Window properties
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const Properties& properties) {
  ToString(std::ostreambuf_iterator<char>(os), properties);
  return os;
}

/**
 * @brief Formats exclusive-fullscreen video mode using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param mode Exclusive video mode
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const ExclusiveVideoMode& mode) {
  return std::format_to(
      out, "ExclusiveVideoMode{{width={}, height={}, refresh_rate={}}}",
      mode.width, mode.height, mode.refresh_rate);
}

/**
 * @brief Formats exclusive-fullscreen video mode as a string.
 * @param mode Exclusive video mode
 * @return Formatted exclusive video mode string
 */
[[nodiscard]] inline std::string ToString(const ExclusiveVideoMode& mode) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), mode);
  return result;
}

/**
 * @brief Formats exclusive-fullscreen video mode as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param mode ExclusiveVideoMode
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const ExclusiveVideoMode& mode) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), mode);
  return result;
}

/**
 * @brief Outputs exclusive-fullscreen video mode to an output stream.
 * @param os Output stream
 * @param mode Exclusive video mode
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const ExclusiveVideoMode& mode) {
  ToString(std::ostreambuf_iterator<char>(os), mode);
  return os;
}

/**
 * @brief Formats a window component using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param window Window component
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const Window& window) {
  out =
      std::format_to(out, "Window{{close_requested={}", window.close_requested);
  out = std::format_to(out, ", dirty_flags=");
  out = ToString(out, window.dirty_flags);
  out = std::format_to(out, ", properties=");
  out = ToString(out, window.properties);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a window component as a string.
 * @param window Window component
 * @return Formatted window string
 */
[[nodiscard]] inline std::string ToString(const Window& window) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), window);
  return result;
}

/**
 * @brief Formats a window component as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param window Window
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const Window& window) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), window);
  return result;
}

/**
 * @brief Outputs a window component to an output stream.
 * @param os Output stream
 * @param window Window component
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Window& window) {
  ToString(std::ostreambuf_iterator<char>(os), window);
  return os;
}

/**
 * @brief Formats `CreatedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CreatedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CreatedMsg& msg) {
  out = std::format_to(out, "CreatedMsg{{entity={}, properties=", msg.entity);
  out = ToString(out, msg.properties);
  return std::format_to(out, "}}");
}

/**
 * @brief Formats a `CreatedMsg` message as a string.
 * @param msg `CreatedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CreatedMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CreatedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CreatedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const CreatedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CreatedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CreatedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const CreatedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ClosedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Closed message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ClosedMsg msg) {
  return std::format_to(out, "ClosedMsg{{entity={}}}", msg.entity);
}

/**
 * @brief Formats a `ClosedMsg` message as a string.
 * @param msg `ClosedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ClosedMsg msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ClosedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ClosedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ClosedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ClosedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ClosedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ClosedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ResizedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ResizedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ResizedMsg msg) {
  return std::format_to(out, "ResizedMsg{{entity={}, width={}, height={}}}",
                        msg.entity, msg.width, msg.height);
}

/**
 * @brief Formats a `ResizedMsg` message as a string.
 * @param msg `ResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ResizedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ResizedMsg` message as a string using `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ResizedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ResizedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ResizedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ResizedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ClientResizedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ClientResizedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ClientResizedMsg msg) {
  return std::format_to(out,
                        "ClientResizedMsg{{entity={}, width={}, height={}}}",
                        msg.entity, msg.width, msg.height);
}

/**
 * @brief Formats a `ClientResizedMsg` message as a string.
 * @param msg `ClientResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ClientResizedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ClientResizedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ClientResizedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ClientResizedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ClientResizedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ClientResizedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ClientResizedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ContentScaleChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ContentScaleChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ContentScaleChangedMsg msg) {
  return std::format_to(
      out, "ContentScaleChangedMsg{{entity={}, scale_x={}, scale_y={}}}",
      msg.entity, msg.scale_x, msg.scale_y);
}

/**
 * @brief Formats a `ContentScaleChangedMsg` message as a string.
 * @param msg `ContentScaleChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ContentScaleChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ContentScaleChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ContentScaleChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ContentScaleChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ContentScaleChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ContentScaleChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ContentScaleChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `PosChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `PosChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, PosChangedMsg msg) {
  return std::format_to(out, "PosChangedMsg{{entity={}, x={}, y={}}}",
                        msg.entity, msg.x, msg.y);
}

/**
 * @brief Formats a `PosChangedMsg` message as a string.
 * @param msg `PosChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(PosChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `PosChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `PosChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(PosChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `PosChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `PosChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, PosChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ModeChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ModeChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ModeChangedMsg msg) {
  return std::format_to(out, "ModeChangedMsg{{entity={}, mode={}}}", msg.entity,
                        ToString(msg.mode));
}

/**
 * @brief Formats a `ModeChangedMsg` message as a string.
 * @param msg `ModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ModeChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ModeChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ModeChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ModeChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ModeChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ModeChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `CursorModeChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CursorModeChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, CursorModeChangedMsg msg) {
  return std::format_to(out,
                        "CursorModeChangedMsg{{entity={}, cursor_mode={}}}",
                        msg.entity, ToString(msg.cursor_mode));
}

/**
 * @brief Formats a `CursorModeChangedMsg` message as a string.
 * @param msg `CursorModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(CursorModeChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CursorModeChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CursorModeChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(CursorModeChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CursorModeChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CursorModeChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CursorModeChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `VisibilityChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `VisibilityChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, VisibilityChangedMsg msg) {
  return std::format_to(out, "VisibilityChangedMsg{{entity={}, visible={}}}",
                        msg.entity, msg.visible);
}

/**
 * @brief Formats a `VisibilityChangedMsg` message as a string.
 * @param msg `VisibilityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(VisibilityChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `VisibilityChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `VisibilityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(VisibilityChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `VisibilityChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `VisibilityChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, VisibilityChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `FocusChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `FocusChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, FocusChangedMsg msg) {
  return std::format_to(out, "FocusChangedMsg{{entity={}, focused={}}}",
                        msg.entity, msg.focused);
}

/**
 * @brief Formats a `FocusChangedMsg` message as a string.
 * @param msg `FocusChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(FocusChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `FocusChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `FocusChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(FocusChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `FocusChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `FocusChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, FocusChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MaximizedChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MaximizedChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MaximizedChangedMsg msg) {
  return std::format_to(out, "MaximizedChangedMsg{{entity={}, maximized={}}}",
                        msg.entity, msg.maximized);
}

/**
 * @brief Formats a `MaximizedChangedMsg` message as a string.
 * @param msg `MaximizedChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MaximizedChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MaximizedChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MaximizedChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(MaximizedChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MaximizedChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MaximizedChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, MaximizedChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `IconChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `IconChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const IconChangedMsg& msg) {
  return std::format_to(out, "IconChangedMsg{{entity={}, icons={}}}",
                        msg.entity, msg.icons.size());
}

/**
 * @brief Formats a `IconChangedMsg` message as a string.
 * @param msg `IconChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const IconChangedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `IconChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `IconChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const IconChangedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `IconChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `IconChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const IconChangedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `ResizableChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `ResizableChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, ResizableChangedMsg msg) {
  return std::format_to(out, "ResizableChangedMsg{{entity={}, resizable={}}}",
                        msg.entity, msg.resizable);
}

/**
 * @brief Formats a `ResizableChangedMsg` message as a string.
 * @param msg `ResizableChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(ResizableChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `ResizableChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `ResizableChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(ResizableChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `ResizableChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `ResizableChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, ResizableChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `DecoratedChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `DecoratedChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, DecoratedChangedMsg msg) {
  return std::format_to(out, "DecoratedChangedMsg{{entity={}, decorated={}}}",
                        msg.entity, msg.decorated);
}

/**
 * @brief Formats a DecoratedChanged message as a string.
 * @param msg DecoratedChanged message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(DecoratedChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a DecoratedChanged message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `DecoratedChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(DecoratedChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a DecoratedChanged message to an output stream.
 * @param os Output stream
 * @param msg DecoratedChangedMsg
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, DecoratedChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `OpacityChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `OpacityChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, OpacityChangedMsg msg) {
  return std::format_to(out, "OpacityChangedMsg{{entity={}, opacity={}}}",
                        msg.entity, msg.opacity);
}

/**
 * @brief Formats a `OpacityChangedMsg` message as a string.
 * @param msg `OpacityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(OpacityChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `OpacityChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `OpacityChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(OpacityChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `OpacityChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `OpacityChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, OpacityChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `FloatingChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `FloatingChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, FloatingChangedMsg msg) {
  return std::format_to(out, "FloatingChangedMsg{{entity={}, floating={}}}",
                        msg.entity, msg.floating);
}

/**
 * @brief Formats a `FloatingChangedMsg` message as a string.
 * @param msg `FloatingChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(FloatingChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `FloatingChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `FloatingChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(FloatingChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `FloatingChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `FloatingChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, FloatingChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `HoverChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `HoverChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, HoverChangedMsg msg) {
  return std::format_to(out, "HoverChangedMsg{{entity={}, hovered={}}}",
                        msg.entity, msg.hovered);
}

/**
 * @brief Formats a `HoverChangedMsg` message as a string.
 * @param msg `HoverChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(HoverChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `HoverChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `HoverChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(HoverChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `HoverChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `HoverChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, HoverChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `MousePassthroughChangedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `MousePassthroughChangedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, MousePassthroughChangedMsg msg) {
  return std::format_to(
      out, "MousePassthroughChangedMsg{{entity={}, mouse_passthrough={}}}",
      msg.entity, msg.mouse_passthrough);
}

/**
 * @brief Formats a `MousePassthroughChangedMsg` message as a string.
 * @param msg `MousePassthroughChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(MousePassthroughChangedMsg msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `MousePassthroughChangedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `MousePassthroughChangedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    MousePassthroughChangedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `MousePassthroughChangedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `MousePassthroughChangedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                MousePassthroughChangedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `DroppedFilesMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `DroppedFilesMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const DroppedFilesMsg& msg) {
  out = std::format_to(out, "DroppedFilesMsg{{entity={}, paths=[", msg.entity);
  bool first = true;
  for (const std::string& path : msg.paths) {
    if (!first) {
      out = std::format_to(out, ", ");
    }
    first = false;
    out = std::format_to(out, "\"{}\"", path);
  }
  return std::format_to(out, "]}}");
}

/**
 * @brief Formats a `DroppedFilesMsg` message as a string.
 * @param msg `DroppedFilesMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const DroppedFilesMsg& msg) {
  std::string result;
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `DroppedFilesMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `DroppedFilesMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(const DroppedFilesMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(256);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `DroppedFilesMsg` message to an output stream.
 * @param os Output stream
 * @param msg `DroppedFilesMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const DroppedFilesMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `CloseRequestedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg Close-requested message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, CloseRequestedMsg msg) {
  return std::format_to(out, "CloseRequestedMsg{{entity={}}}", msg.entity);
}

/**
 * @brief Formats a `CloseRequestedMsg` message as a string.
 * @param msg `CloseRequestedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(CloseRequestedMsg msg) {
  std::string result;
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CloseRequestedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CloseRequestedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(CloseRequestedMsg msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(64);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CloseRequestedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CloseRequestedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, CloseRequestedMsg msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

/**
 * @brief Formats `CreationFailedMsg` message using an output iterator.
 * @tparam It Output iterator type
 * @param out Output iterator to write the formatted string to
 * @param msg `CreationFailedMsg` message
 * @return The output iterator after writing
 */
template <typename It>
  requires std::output_iterator<It, char>
inline It ToString(It out, const CreationFailedMsg& msg) {
  return std::format_to(out, "CreationFailedMsg{{entity={}, reason=\"{}\"}}",
                        msg.entity, msg.reason);
}

/**
 * @brief Formats a `CreationFailedMsg` message as a string.
 * @param msg `CreationFailedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::string ToString(const CreationFailedMsg& msg) {
  std::string result;
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Formats a `CreationFailedMsg` message as a string using
 * `TemporaryStorage`.
 * @warning The returned string is only valid until the next call to
 * `ResetTemporaryStorage()` on this thread.
 * @param msg `CreationFailedMsg` message
 * @return Formatted string
 */
[[nodiscard]] inline std::pmr::string TempToString(
    const CreationFailedMsg& msg) {
  std::pmr::string result{&mem::GetTemporaryStorage()};
  result.reserve(128);
  ToString(std::back_inserter(result), msg);
  return result;
}

/**
 * @brief Outputs a `CreationFailedMsg` message to an output stream.
 * @param os Output stream
 * @param msg `CreationFailedMsg` message
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os,
                                const CreationFailedMsg& msg) {
  ToString(std::ostreambuf_iterator<char>(os), msg);
  return os;
}

}  // namespace helios::window

HELIOS_MODULE_EXPORT
namespace std {

template <>
struct formatter<helios::window::Mode> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::Mode mode, format_context& ctx) {
    return format_to(ctx.out(), "Mode::{}", helios::window::ToString(mode));
  }
};

template <>
struct formatter<helios::window::CursorMode> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::CursorMode mode,
                               format_context& ctx) {
    return format_to(ctx.out(), "CursorMode::{}",
                     helios::window::ToString(mode));
  }
};

template <>
struct formatter<helios::window::ClientApi> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static constexpr auto format(helios::window::ClientApi api,
                               format_context& ctx) {
    return format_to(ctx.out(), "ClientApi::{}", helios::window::ToString(api));
  }
};

template <>
struct formatter<helios::window::DirtyFlag> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(helios::window::DirtyFlag flags, format_context& ctx) {
    return helios::window::ToString(ctx.out(), flags, /*with_prefix=*/true);
  }
};

template <>
struct formatter<helios::window::IconImage> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::IconImage& image,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), image);
  }
};

template <>
struct formatter<helios::window::Properties> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Properties& properties,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), properties);
  }
};

template <>
struct formatter<helios::window::ExclusiveVideoMode> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ExclusiveVideoMode& mode,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), mode);
  }
};

template <>
struct formatter<helios::window::Window> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::Window& window,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), window);
  }
};

template <>
struct formatter<helios::window::CreatedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CreatedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ClosedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ClosedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ResizedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ResizedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ClientResizedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ClientResizedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ContentScaleChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ContentScaleChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::PosChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::PosChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ModeChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ModeChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::CursorModeChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CursorModeChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::VisibilityChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::VisibilityChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::FocusChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::FocusChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MaximizedChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MaximizedChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::IconChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::IconChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::ResizableChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::ResizableChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::DecoratedChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::DecoratedChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::OpacityChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::OpacityChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::FloatingChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::FloatingChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::HoverChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::HoverChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::MousePassthroughChangedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::MousePassthroughChangedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::DroppedFilesMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::DroppedFilesMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::CloseRequestedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CloseRequestedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

template <>
struct formatter<helios::window::CreationFailedMsg> {
  static constexpr auto parse(format_parse_context& ctx) noexcept {
    return ctx.begin();
  }

  static auto format(const helios::window::CreationFailedMsg& msg,
                     format_context& ctx) {
    return helios::window::ToString(ctx.out(), msg);
  }
};

}  // namespace std
#endif  // HELIOS_MODULE_CONSUMER_SHIM
