#pragma once

#include <helios/ecs/component/bundle.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace helios::window {

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
  constexpr void SetMonitorIndex(int32_t value) noexcept {
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

/// @brief Sparse-set component holding the native handle for a window entity.
struct NativeHandleComponent {
  static constexpr std::string_view kName =
      "helios::window::NativeHandleComponent";
  static constexpr auto kStorageType = ecs::ComponentStorageType::kArchetype;

  NativeHandle handle;
};

}  // namespace helios::window
