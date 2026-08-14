#pragma once

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace helios::window {

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
};

/// @brief Sent when a window content scale changes.
struct ContentScaleChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ContentScaleChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  ecs::Entity entity;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
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

/// @brief Sent when the OS clipboard text changes.
struct ClipboardChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::ClipboardChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  std::string text;
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

/// @brief Sent when the connected monitor layout changes.
struct MonitorsChangedMsg {
  static constexpr std::string_view kName =
      "helios::window::MonitorsChangedMsg";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  int32_t index = 0;
  MonitorEvent event = MonitorEvent::kConnected;
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

}  // namespace helios::window
