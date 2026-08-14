#pragma once

#include <helios/app/application.hpp>
#include <helios/app/plugin.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::window {

/// @brief Registers window ECS types without creating OS windows.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::window::Plugin";

  /**
   * @brief Constructs a window plugin with the given settings.
   * @param settings Global window behavior to insert when absent
   */
  explicit Plugin(Settings settings = {}) : settings(settings) {}

  void Build(app::App& app) override {
    app.TryInsertResources(settings, Monitors{}, Clipboard{});
    app.AddMessages<CreatedMsg, ClosedMsg, ResizedMsg, ClientResizedMsg,
                    ContentScaleChangedMsg, PosChangedMsg, ModeChangedMsg,
                    CursorModeChangedMsg, VisibilityChangedMsg, FocusChangedMsg,
                    MaximizedChangedMsg, IconChangedMsg, ResizableChangedMsg,
                    DecoratedChangedMsg, MonitorsChangedMsg, CloseRequestedMsg,
                    OpacityChangedMsg, FloatingChangedMsg, HoverChangedMsg,
                    ClipboardChangedMsg, DroppedFilesMsg, CreationFailedMsg,
                    MousePassthroughChangedMsg>();
  }

  Settings settings;
};

}  // namespace helios::window
