#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/message/params.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/params.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/system/composite_param.hpp>
#include <helios/ecs/system/param.hpp>
#endif
#include <helios/window/clipboard.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/properties.hpp>

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Mutable query over all `Window` components.
struct Windows {
  ecs::Query<Window&> query;
};

/// @brief Read-only query over all `Window` components.
struct WindowsView {
  ecs::Query<const Window&> query;
};

/// @brief Mutable query over primary windows.
struct PrimaryWindows {
  ecs::Query<Window&, ecs::With<Primary>> query;
};

/// @brief Mutable query over primary windows.
struct PrimaryWindowsView {
  ecs::Query<const Window&, ecs::With<Primary>> query;
};

/// @brief Window create / close / failure message readers.
struct LifecycleMessages {
  ecs::MessageReader<CreatedMsg> created;
  ecs::MessageReader<ClosedMsg> closed;
  ecs::MessageReader<CloseRequestedMsg> close_requested;
  ecs::MessageReader<CreationFailedMsg> failed;
};

/// @brief Framebuffer, client-area, scale, and position message readers.
struct GeometryMessages {
  ecs::MessageReader<ResizedMsg> resized;
  ecs::MessageReader<ClientResizedMsg> client_resized;
  ecs::MessageReader<ContentScaleChangedMsg> content_scale;
  ecs::MessageReader<PosChangedMsg> pos;
};

/// @brief Chrome and presentation message readers.
struct AppearanceMessages {
  ecs::MessageReader<ModeChangedMsg> mode;
  ecs::MessageReader<CursorModeChangedMsg> cursor_mode;
  ecs::MessageReader<VisibilityChangedMsg> visibility;
  ecs::MessageReader<FocusChangedMsg> focus;
  ecs::MessageReader<MaximizedChangedMsg> maximized;
  ecs::MessageReader<IconChangedMsg> icon;
  ecs::MessageReader<ResizableChangedMsg> resizable;
  ecs::MessageReader<DecoratedChangedMsg> decorated;
  ecs::MessageReader<OpacityChangedMsg> opacity;
  ecs::MessageReader<FloatingChangedMsg> floating;
  ecs::MessageReader<HoverChangedMsg> hover;
  ecs::MessageReader<MousePassthroughChangedMsg> mouse_passthrough;
};

/// @brief Clipboard, file-drop, and monitor message readers.
struct PlatformMessages {
  ecs::MessageReader<ClipboardChangedMsg> clipboard;
  ecs::MessageReader<DroppedFilesMsg> dropped_files;
  ecs::MessageReader<MonitorConnectedMsg> monitor_connected;
  ecs::MessageReader<MonitorDisconnectedMsg> monitor_disconnected;
};

/// @brief All window message readers.
struct Messages {
  LifecycleMessages lifecycle;
  GeometryMessages geometry;
  AppearanceMessages appearance;
  PlatformMessages platform;
};

/// @brief Window create / close / failure message writers.
struct LifecycleWriters {
  ecs::MessageWriter<CreatedMsg> created;
  ecs::MessageWriter<ClosedMsg> closed;
  ecs::MessageWriter<CloseRequestedMsg> close_requested;
  ecs::MessageWriter<CreationFailedMsg> failed;
};

/// @brief Framebuffer, client-area, scale, and position message writers.
struct GeometryWriters {
  ecs::MessageWriter<ResizedMsg> resized;
  ecs::MessageWriter<ClientResizedMsg> client_resized;
  ecs::MessageWriter<ContentScaleChangedMsg> content_scale;
  ecs::MessageWriter<PosChangedMsg> pos;
};

/// @brief Chrome and presentation message writers.
struct AppearanceWriters {
  ecs::MessageWriter<ModeChangedMsg> mode;
  ecs::MessageWriter<CursorModeChangedMsg> cursor_mode;
  ecs::MessageWriter<VisibilityChangedMsg> visibility;
  ecs::MessageWriter<FocusChangedMsg> focus;
  ecs::MessageWriter<MaximizedChangedMsg> maximized;
  ecs::MessageWriter<IconChangedMsg> icon;
  ecs::MessageWriter<ResizableChangedMsg> resizable;
  ecs::MessageWriter<DecoratedChangedMsg> decorated;
  ecs::MessageWriter<OpacityChangedMsg> opacity;
  ecs::MessageWriter<FloatingChangedMsg> floating;
  ecs::MessageWriter<HoverChangedMsg> hover;
  ecs::MessageWriter<MousePassthroughChangedMsg> mouse_passthrough;
};

/// @brief Clipboard, file-drop, and monitor message writers.
struct PlatformWriters {
  ecs::MessageWriter<ClipboardChangedMsg> clipboard;
  ecs::MessageWriter<DroppedFilesMsg> dropped_files;
  ecs::MessageWriter<MonitorConnectedMsg> monitor_connected;
  ecs::MessageWriter<MonitorDisconnectedMsg> monitor_disconnected;
};

/// @brief All window message writers.
struct Writers {
  LifecycleWriters lifecycle;
  GeometryWriters geometry;
  AppearanceWriters appearance;
  PlatformWriters platform;
};

/// @brief Writers used when a backend creates a native window.
struct CreationWriters {
  ecs::MessageWriter<CreatedMsg> created;
  ecs::MessageWriter<ContentScaleChangedMsg> content_scale;
  ecs::MessageWriter<CreationFailedMsg> failed;
};

}  // namespace helios::window

namespace helios::ecs {

template <>
struct SystemParamTraits<window::Windows>
    : CompositeSystemParam<window::Windows, Query<window::Window&>> {};

template <>
struct SystemParamTraits<window::WindowsView>
    : CompositeSystemParam<window::WindowsView, Query<const window::Window&>> {
};

template <>
struct SystemParamTraits<window::PrimaryWindows>
    : CompositeSystemParam<window::PrimaryWindows,
                           Query<window::Window&, With<window::Primary>>> {};

template <>
struct SystemParamTraits<window::PrimaryWindowsView>
    : CompositeSystemParam<
          window::PrimaryWindowsView,
          Query<const window::Window&, With<window::Primary>>> {};

template <>
struct SystemParamTraits<window::LifecycleMessages>
    : CompositeSystemParam<window::LifecycleMessages,
                           MessageReader<window::CreatedMsg>,
                           MessageReader<window::ClosedMsg>,
                           MessageReader<window::CloseRequestedMsg>,
                           MessageReader<window::CreationFailedMsg>> {};

template <>
struct SystemParamTraits<window::GeometryMessages>
    : CompositeSystemParam<window::GeometryMessages,
                           MessageReader<window::ResizedMsg>,
                           MessageReader<window::ClientResizedMsg>,
                           MessageReader<window::ContentScaleChangedMsg>,
                           MessageReader<window::PosChangedMsg>> {};

template <>
struct SystemParamTraits<window::AppearanceMessages>
    : CompositeSystemParam<window::AppearanceMessages,
                           MessageReader<window::ModeChangedMsg>,
                           MessageReader<window::CursorModeChangedMsg>,
                           MessageReader<window::VisibilityChangedMsg>,
                           MessageReader<window::FocusChangedMsg>,
                           MessageReader<window::MaximizedChangedMsg>,
                           MessageReader<window::IconChangedMsg>,
                           MessageReader<window::ResizableChangedMsg>,
                           MessageReader<window::DecoratedChangedMsg>,
                           MessageReader<window::OpacityChangedMsg>,
                           MessageReader<window::FloatingChangedMsg>,
                           MessageReader<window::HoverChangedMsg>,
                           MessageReader<window::MousePassthroughChangedMsg>> {
};

template <>
struct SystemParamTraits<window::PlatformMessages>
    : CompositeSystemParam<window::PlatformMessages,
                           MessageReader<window::ClipboardChangedMsg>,
                           MessageReader<window::DroppedFilesMsg>,
                           MessageReader<window::MonitorConnectedMsg>,
                           MessageReader<window::MonitorDisconnectedMsg>> {};

template <>
struct SystemParamTraits<window::Messages>
    : CompositeSystemParam<window::Messages, window::LifecycleMessages,
                           window::GeometryMessages, window::AppearanceMessages,
                           window::PlatformMessages> {};

template <>
struct SystemParamTraits<window::LifecycleWriters>
    : CompositeSystemParam<window::LifecycleWriters,
                           MessageWriter<window::CreatedMsg>,
                           MessageWriter<window::ClosedMsg>,
                           MessageWriter<window::CloseRequestedMsg>,
                           MessageWriter<window::CreationFailedMsg>> {};

template <>
struct SystemParamTraits<window::GeometryWriters>
    : CompositeSystemParam<window::GeometryWriters,
                           MessageWriter<window::ResizedMsg>,
                           MessageWriter<window::ClientResizedMsg>,
                           MessageWriter<window::ContentScaleChangedMsg>,
                           MessageWriter<window::PosChangedMsg>> {};

template <>
struct SystemParamTraits<window::AppearanceWriters>
    : CompositeSystemParam<window::AppearanceWriters,
                           MessageWriter<window::ModeChangedMsg>,
                           MessageWriter<window::CursorModeChangedMsg>,
                           MessageWriter<window::VisibilityChangedMsg>,
                           MessageWriter<window::FocusChangedMsg>,
                           MessageWriter<window::MaximizedChangedMsg>,
                           MessageWriter<window::IconChangedMsg>,
                           MessageWriter<window::ResizableChangedMsg>,
                           MessageWriter<window::DecoratedChangedMsg>,
                           MessageWriter<window::OpacityChangedMsg>,
                           MessageWriter<window::FloatingChangedMsg>,
                           MessageWriter<window::HoverChangedMsg>,
                           MessageWriter<window::MousePassthroughChangedMsg>> {
};

template <>
struct SystemParamTraits<window::PlatformWriters>
    : CompositeSystemParam<window::PlatformWriters,
                           MessageWriter<window::ClipboardChangedMsg>,
                           MessageWriter<window::DroppedFilesMsg>,
                           MessageWriter<window::MonitorConnectedMsg>,
                           MessageWriter<window::MonitorDisconnectedMsg>> {};

template <>
struct SystemParamTraits<window::Writers>
    : CompositeSystemParam<window::Writers, window::LifecycleWriters,
                           window::GeometryWriters, window::AppearanceWriters,
                           window::PlatformWriters> {};

template <>
struct SystemParamTraits<window::CreationWriters>
    : CompositeSystemParam<window::CreationWriters,
                           MessageWriter<window::CreatedMsg>,
                           MessageWriter<window::ContentScaleChangedMsg>,
                           MessageWriter<window::CreationFailedMsg>> {};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
