#include <pch.hpp>

#include <helios/window/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/builtin/frame_limiter.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/ids.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/params.hpp>
#include <helios/window/properties.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace helios::window {

void SyncFrameLimiterRefreshRate::operator()(
    ecs::Res<app::FrameLimiter> limiter, ecs::Res<const Monitors> monitors,
    PrimaryWindowsView primaries) const {
  if (limiter->Mode() != app::FrameLimiterMode::kAuto) {
    return;
  }

  std::optional<MonitorId> window_monitor;
  primaries.query.ForEach([&window_monitor](const Window& window) {
    if (!window_monitor.has_value()) {
      window_monitor = window.properties.monitor_index;
    }
  });

  const auto& list = monitors->monitors;
  if (list.empty()) [[unlikely]] {
    limiter->SetRefreshRate(0);
    return;
  }

  const Monitor* chosen = nullptr;
  if (window_monitor.has_value()) {
    const MonitorId index = *window_monitor;
    const auto by_index = std::ranges::find_if(
        list,
        [index](const Monitor& monitor) { return monitor.index == index; });
    if (by_index != list.end()) {
      chosen = &*by_index;
    } else if (static_cast<size_t>(index) < list.size()) {
      chosen = &list[static_cast<size_t>(index)];
    }
  }

  if (chosen == nullptr) {
    const auto primary = std::ranges::find_if(
        list, [](const Monitor& monitor) { return monitor.primary; });
    if (primary != list.end()) {
      chosen = &*primary;
    } else {
      chosen = &list.front();
    }
  }

  limiter->SetRefreshRate(chosen->current.refresh_rate);
}

void Plugin::Build(app::App& app) {
  app.TryInsertResources(settings, Monitors{}, Clipboard{});
  app.AddMessages<CreatedMsg, ClosedMsg, ResizedMsg, ClientResizedMsg,
                  ContentScaleChangedMsg, PosChangedMsg, ModeChangedMsg,
                  CursorModeChangedMsg, VisibilityChangedMsg, FocusChangedMsg,
                  MaximizedChangedMsg, IconChangedMsg, ResizableChangedMsg,
                  DecoratedChangedMsg, CloseRequestedMsg, OpacityChangedMsg,
                  FloatingChangedMsg, HoverChangedMsg,
                  MousePassthroughChangedMsg, ClipboardChangedMsg,
                  DroppedFilesMsg, CreationFailedMsg, MonitorConnectedMsg,
                  MonitorDisconnectedMsg>();
  if (app.HasPlugins<app::FrameLimiterPlugin>()) {
    app.AddSystem(app::kFirst, SyncFrameLimiterRefreshRate{});
  }
}

}  // namespace helios::window
